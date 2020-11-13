#include "clientchannel.h"
#include "globaltool.h"
#include <stdlib.h>

#define RECVBUF_SIZE 256

//// ClientChannelList

/** 비어있는 객체 생성; Caller가 구조체 내 데이터 채우기 필요 */
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
    current->type = SSH;
    current->sshSess = sessOpened;
    current->sshChan = chan;
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