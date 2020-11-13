#include "clientchannel.h"


//// ClientChannelList

void CCList_init(ClientChannelList * l) {
    l->head = NULL;
}

ClientChannel * CCList_addNewFromSSH(ClientChannelList * l, ssh_session sessOpened) {
    //TODO thread-safe, mutex
    ClientChannel * 
}

// TODO: CCList_delete
// 1. [prev] -> [ccDel] -> [next] (선형탐색)
// 2. [prev] -> [next] / free [ccDel]
