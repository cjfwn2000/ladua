#include "clientchannel.h"


//// ClientChannelList

void CCList_init(ClientChannelList * l) {
    // TODO Dummy를 헤드에 넣는 방법론 이용.
    l->head = dummy;
    l->tail = dummy;
}

ClientChannel * CCList_addNewFromSSH(ClientChannelList * l) {
    ClientChannel * 
}

// TODO: CCList_delete
// 1. [prev] -> [ccDel] -> [next] (선형탐색)
// 2. [prev] -> [next] / free [ccDel]
