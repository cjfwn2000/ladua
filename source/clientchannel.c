#include "clientchannel.h"
#include "globaltool.h"
#include <stdlib.h>
#include <libssh/server.h>
#include "libtelnet.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#define RECVBUF_SIZE 256

//// Private functions

// 흔히 생각하길 메시지를 recv(소켓, ...)으로 받으려 하는데
// 소켓에 들어오는 것이 클라이언트가 실제 타이핑한 메시지 뿐 아니라 IAC DO 메시지도 포함되므로
// 그 클라이언트 실제 메시지를 받고자 하면 recv가 아닌 _TBP_recv을 이용해야 함
/** @returns 읽은 데이터 바이트 길이; 에러 혹은 읽을데이터 없었을 시 음수; 연결 끊어질 시 0
  * @warning libssh와 달리 C Socket fd에 대한 recv는 "읽을데이터없음"일 때 음수(-1) */
static int _TBP_recv(TelnetBackpack * tbp, char * buf, int nbytes)
{
    int sockRecieved = 0;
    // 입력 파라메터
    tbp->tempRecvBuf = buf;
    tbp->tempRecvBufSize = nbytes;
    // _telnetEvtHandler는 tempDataTypeFlag을 1로 설정해주는 것만 함. 0으로 설정해주는 것은 우리 책임
    tbp->tempDataTypeFlag = 0;

    do {
        // 소켓 수신
        sockRecieved = recv(tbp->sock, tbp->tempRecvBuf, tbp->tempRecvBufSize, 0);
        // 현재 상황: buf에 raw received data 저장 [0..sockRecieved-1]
        if(sockRecieved > 0) {
            // 받은 것이 클라이언트가 직접 보내준 것(예:타이핑)인지 알 수 없으므로
            // 아래 호출을 통해 _telnetEvtHandler가 이를 판별해주도록 한다.
            telnet_recv(tbp->tracker, tbp->tempRecvBuf, sockRecieved);
            // 판별 결과 기대한 게 맞다면 그것으로 종결한다.
            // 판별 결과 다른 거였다면 다시 소켓 수신을 시도한다.
            if(tbp->tempDataTypeFlag) {
                tbp->tempDataTypeFlag = 0;  //궅이 다시 =0 하는 이유: defensive programming
                return tbp->tempRecvBytes;
                // 최종 상황: buf에 재분석된 received data 저장[0..tbp->tbp->tempRecvBytes-1]
                // , return값은 tbp->tempRecvBytes
            } else {
                continue;
            }
        } else {
            // 받은게 없거나 에러이거나 연결 끊어짐
            break;
            // 최종상황: buf은 무효함, return값은 0 혹은 음수
        }
    } while(1);

    return sockRecieved;
}
        

static void _telnetEvtHandler(telnet_t * tt, telnet_event_t * ev, void * userData)
{
    //int sock = (intptr_t) userData;
    TelnetBackpack * tbp = (TelnetBackpack *) userData;
    int actualRecvByte; //case TELNET_EV_DATA에 쓰임

    switch (ev->type) {
    // 받을 데이터 생김
    case TELNET_EV_DATA:
        tbp->tempDataTypeFlag = 1;
        //actualRecvByte = min(ev->data.size, tbp->tempRecvBufSize);
        actualRecvByte = (ev->data.size > tbp->tempRecvBufSize) ? tbp->tempRecvBufSize : ev->data.size;
        memmove(tbp->tempRecvBuf, ev->data.buffer, actualRecvByte);
        tbp->tempRecvBytes = actualRecvByte;
        break;
    // 보낼 데이터 생김
    case TELNET_EV_SEND:
        // _global_recvBytes_forTelnet = send(blahblah);
        send(tbp->sock, ev->data.buffer, (int)ev->data.size, 0);
        break;
    // enable compress2 if accepted by client
    case TELNET_EV_DO:
        if(ev->neg.telopt == TELNET_TELOPT_COMPRESS2)
            telnet_begin_compress2(tt);
        break;
    // Error
    case TELNET_EV_ERROR:
        close(tbp->sock);
        break;
    // 그 외 이벤트 무시
    default:
        break;
    }
}

//// TelnetBackpack

TelnetBackpack * TelnetBackpack_newWithAuth(int sock, int (*isAccountValid)(const char * username, const char * pass))
{
    TelnetBackpack * newTBP;
    telnet_t * newTT;
    static const telnet_telopt_t telopts[] = {
        { TELNET_TELOPT_COMPRESS2,	TELNET_WILL, TELNET_DONT },
        { -1, 0, 0 }
    };
    int try = 0; //입력시도횟수
    static const int MAX_TRY = 5; //최대 입력시도횟수
    char recvBuf[64];  //expected: login user name
    int recvBytes = 0; //length
    int authenticated = 0;
    char loginName[64] = "";
    char loginPass[64] = "";
    int sockFL = 0;  // 소켓이 셸 접속상태일때부터는 Non-blocking으로 만들기 위함

    newTBP = (TelnetBackpack *) malloc( sizeof(TelnetBackpack) );
    newTBP->sock = sock;
    newTT = telnet_init(telopts, _telnetEvtHandler, 0, (void*)newTBP);
    newTBP->tracker = newTT;
    // 나머지 newTBP->temp...는 _TBP_recv 등등이 다루는 것이므로 따로 설정할 필요 없음
    
    telnet_negotiate(newTT, TELNET_WILL, TELNET_TELOPT_COMPRESS2);
    telnet_negotiate(newTT, TELNET_WILL, TELNET_TELOPT_ECHO);
    
    for(try = 0; try < MAX_TRY; try++) {
        // 입력받기: loginName
        telnet_printf(newTT, "Login as: ");
        telnet_negotiate(newTT, TELNET_WONT, TELNET_TELOPT_ECHO);
        recvBytes = _TBP_recv(newTBP, recvBuf, sizeof recvBuf);
        if(recvBytes <= 0) // 입력이 끊어진 상황. 종료
            break;
        strncpy(loginName, recvBuf, recvBytes);
        loginName[recvBytes] = '\0'; //C-string으로 만들기 (Null-terminated)
        rtrim(loginName, NULL);
        if(strlen(loginName) == 0) {
            telnet_printf(newTT, "\n");
            continue;
        }

        // 입력받기: loginPass
        telnet_negotiate(newTT, TELNET_WILL, TELNET_TELOPT_ECHO);
        telnet_printf(newTT, "\n%s's password: ", loginName);
        recvBytes = _TBP_recv(newTBP, recvBuf, sizeof recvBuf);
        if(recvBytes <= 0) // 입력이 끊어진 상황. 종료
            break;
        strncpy(loginPass, recvBuf, recvBytes);
        rtrim(loginPass, NULL);
        loginPass[recvBytes] = '\0';
        
        authenticated = isAccountValid(loginName, loginPass);
        if(authenticated) {
            telnet_printf(newTT, "\n");
            telnet_negotiate(newTT, TELNET_WONT, TELNET_TELOPT_ECHO);
            break;
        } else if(try+1 < MAX_TRY)
            telnet_printf(newTT, "\nSorry, try again.\n");
    }
    if(authenticated) {
        sockFL = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, sockFL | O_NONBLOCK);
        return newTBP;
    } else {
        telnet_free(newTT);
        return NULL;
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

int ClientChannel_initFromTelnet(ClientChannel * c, TelnetBackpack * tbp)
{
    c->type = TELNET;
    c->telnetBackpack = tbp;

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
        
    case TELNET:
        readed = _TBP_recv(c->telnetBackpack, buf, nbytes);
        // Note: _TBP_recv의 return은 sys/socket.h recv의 return과 같다.
        // 혹시 연결이 끊어졌는지 확인
        if(readed == 0)
            return -1;
        // _TBP_recv가 음수를 return했지만 아직 끊어진 게 아닌 경우, 우리는 0를 return해야 한다.
        if(readed < 0 && errno == EAGAIN)
            return 0;
        return readed;
        break;
        
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
        
    case TELNET:
        writed = send(c->telnetBackpack->sock, buf, nbytes, 0);
        return writed;
        break;

    default:
        logInfo("[ClientChannel] Warning: Unexpected type for send.");
        return -1;
        break;
    }
}

void ClientChannel_close(ClientChannel * c)
{
    switch(c->type)
    {
    case SSH:
        if(ssh_channel_is_open(c->sshChan)) {
            ssh_channel_send_eof(c->sshChan);
            ssh_channel_close(c->sshChan);
        }
        ssh_channel_free(c->sshChan);
        ssh_disconnect(c->sshSess);
        break;

    case TELNET:
        close(c->telnetBackpack->sock);
        telnet_free(c->telnetBackpack->tracker);
        free(c->telnetBackpack);
        break;

    default:
        logInfo("[ClientChannel] Warning: Unexpected type for close.");
    }
}


//// ClientChannelList

/** 비어있는 객체 생성 */
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
/** 비어있는 객체 생성하여 리스트에 추가; 
 * Caller가 따로 init필요 */
static ClientChannel * _createNodeAndAdd(ClientChannelList * l)
{
    ClientChannel * current = NULL;
    
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
    current->next = NULL;

    return current;
}

void CCList_init(ClientChannelList * l)
{
    l->head = NULL;
}

ClientChannel * CCList_addNewFromSSH(ClientChannelList * l, ssh_session sessOpened, ssh_channel chan)
{
    ClientChannel * current = _createNodeAndAdd(l);
    ClientChannel_initFromSsh(current, sessOpened, chan);
    return current;
}

ClientChannel * CCList_addNewFromTelnet(ClientChannelList * l, TelnetBackpack * tbp)
{
    ClientChannel * current = _createNodeAndAdd(l);
    ClientChannel_initFromTelnet(current, tbp);
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