#include "clientchannel.h"
#include "globaltool.h"
#include <stdlib.h>
#include <libssh/server.h>
#include "libtelnet.h"

#define RECVBUF_SIZE 256

//// Private functions

static void _telnetEvtHandler(telnet_t * tt, telnet_event_t * ev, void * userData)
{
    int sock = (intptr_t) userData;

    switch (ev->type) {
    // 받을 데이터 생김
    case TELNET_EV_DATA:
        logInfo("_telnetEvtHandler] Warning: Not implemented TELNET_EV_DATA.");
        telnet_negotiate(tt, TELNET_WONT, TELNET_TELOPT_ECHO);
        telnet_negotiate(tt, TELNET_WILL, TELNET_TELOPT_ECHO);
        break;
    // 보낼 데이터 생김
    case TELNET_EV_SEND:
        // _global_recvBytes_forTelnet = send(blahblah);
        logInfo("_telnetEvtHandler] Warning: Still not returned into static global variables.");
        send(sock, ev->data.buffer, (int)ev->data.size, 0);
        break;
        //TODO static global variable을 통한 return 구현이니까 그 variable에 mutex를 걸어야겠지.
        //물론 mutex 진입 주체는 여기가 아니라 ClientChannel_recv다.
        break;
    // enable compress2 if accepted by client
    case TELNET_EV_DO:
        if(ev->neg.telopt == TELNET_TELOPT_COMPRESS2)
            telnet_begin_compress2(tt);
        break;
    // Error
    case TELNET_EV_ERROR:
        logInfo("[_telnetEvtHandler] TELNET_EV_ERROR");
        close(sock);
        break;
    // 그 외 이벤트 무시
    default:
        break;
    }
}

//// ClientChannel

int ClientChannel_initFromSsh(ClientChannel * c, ssh_session sessOpened, ssh_channel chan)
{
    c->type = SSH;
    c->sshSess = sessOpened;
    c->sshChan = chan;

    c->next = NULL;
    return 0;
}

int ClientChannel_recv(ClientChannel * c, char * buf, int nbytes)
{
    int readed;

    switch (c->type)
    {
    case SSH:
        readed = ssh_channel_read_nonblocking(c->sshChan, buf, nbytes, 0);
        // 혹시 연결이 끊어졌는지 확인
        if(readed == 0 && ssh_channel_is_eof(c->sshChan))
            return -1;
        return readed;
        break;
        
    //case TELNET:
    /* TODO 
    소켓에서 raw하게 데이터를 recv한다.
    telnet_recv : 정해진 핸들러(telnet_init때 정해줌)로 recv데이터를 보낸다.
    이때 user_data는 우리가 받고자 하는 변수 장소 포인터, 즉 char* buf와 int nbytes로 하자.
    (그렇다. 소켓에서 바로 받지 않고 일단 telnet_recv->핸들러로 보내줘야 한다.)
    그 핸들러: char* buf에게 int nbytes만큼 쓰기. 그렇다, 그 핸들러가 ClientChannel_recv가 하던 일을 대신한다.

    그 핸들러는 아마 이 파일(clientchannel.c)에서 정의해줘야할 것이다. 그걸 mainrun.c에서 가리키게 해주고.
    이렇게 해도 되는 이유? 핸들러는 새로운 쓰레드로 파서 실행되는 것은 아닌 걸로 보인다. 그러므로 실행흐름은 쭉 이어지는 것으로 볼 수 있다.
    */
    default:
        logInfo("[ClientChannel] Warning: Unexpected type for recv.");
        return -1;
        break;
    }
}

int ClientChannel_send(ClientChannel * c, const char * buf, int nbytes)
{
    int writed;

    switch (c->type)
    {
    case SSH:
        writed = ssh_channel_write(c->sshChan, buf, nbytes);
        return writed;
        break;
        
    //case TELNET:
    default:
        logInfo("[ClientChannel] Warning: Unexpected type for send.");
        return -1;
        break;
    }
}

void ClientChannel_close(ClientChannel * c)
{
    if(ssh_channel_is_open(c->sshChan)) {
        ssh_channel_send_eof(c->sshChan);
        ssh_channel_close(c->sshChan);
    }
    ssh_channel_free(c->sshChan);
    ssh_disconnect(c->sshSess);
}

telnet_t * ClientChannel_newTempTelnettWithAuth(int sock)
{
    telnet_t * newTT;
    static const telnet_telopt_t telopts[] = {
        { TELNET_TELOPT_COMPRESS2,	TELNET_WILL, TELNET_DONT },
        { -1, 0, 0 }
    };

    newTT = telnet_init(telopts, _telnetEvtHandler, 0, (void*)(intptr_t)sock);

    // TODO 참조 : https://github.com/seanmiddleditch/libtelnet/blob/develop/util/telnet-test.c

    telnet_negotiate(telnettrackerNewb, TELNET_WILL, TELNET_TELOPT_COMPRESS2);
    telnet_negotiate(telnettrackerNewb, TELNET_WILL, TELNET_TELOPT_ECHO);
    

    return NULL;
}


//// ClientChannelList

/** 비어있는 객체 생성; Caller가 따로 init필요 */
static ClientChannel * _createNode()
{
    ClientChannel * newNode = malloc(sizeof(ClientChannel));
    if(!newNode) {
        logInfo("[ClientChannelList] Warning: Failed to malloc new ClientChannel");
        return NULL;
    }
    newNode->next = NULL;
    return newNode;
}

void CCList_init(ClientChannelList * l)
{
    l->head = NULL;
}

ClientChannel * CCList_addNewFromSSH(ClientChannelList * l, ssh_session sessOpened, ssh_channel chan)
{
    ClientChannel * current = NULL;
    // 선형탐색으로 빈자리 찾기
    if(l->head == NULL) {
        l->head = _createNode();
        current = l->head;
    } else {
        current = l->head;
        while(current->next != NULL) {
            current = current->next;
        }
        current->next = _createNode();
        current = current->next;
    }
    ClientChannel_initFromSsh(current, sessOpened, chan);
    current->next = NULL;

    return current;
}

void CCList_batchRecv(ClientChannelList * l, void (*fnRecvData)(const char *, int))
{
    char recvBuf[RECVBUF_SIZE];
    int recvBytes;
    ClientChannel * current;
    ClientChannel * previous;  //For deleting operation

    //for(; current != NULL; current = current->next) {
    current = l->head;
    previous = NULL;
    while(current != NULL) {
        recvBytes = ClientChannel_recv(current, recvBuf, sizeof recvBuf);
        if(recvBytes > 0) { //보낼 데이터 있음
            fnRecvData(recvBuf, recvBytes);
        } else if (recvBytes < 0) {  //Error or EOF
            logInfo("[CCList] Broken client channel, disconnecting.");
            ClientChannel_close(current);
            // Delete current node
            if(current == l->head)  //extreme case: 첫번째 노드였을 때
                l->head = current->next;
            else
                previous->next = current->next;
            free(current);
        }
        previous = current;
        current = current->next;
    }
}

void CCList_batchSend(ClientChannelList * l, const char * sendBuf, int sendNBytes)
{
    ClientChannel * current;

    current = l->head;
    while(current != NULL) {
        ClientChannel_send(current, sendBuf, sendNBytes);
        current = current->next;
    }
}

void CCList_finalize(ClientChannelList * l)
{
    ClientChannel * current;
    ClientChannel * next;
    
    current = l->head;
    while(current != NULL) {
        next = current->next;
        ClientChannel_close(current);
        free(current);
        current = next;
    }
    l->head = NULL;
}