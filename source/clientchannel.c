#include "clientchannel.h"


//// ClientChannelList

void CCList_init(ClientChannelList * l) {
    l->head = NULL;
}

// TODO: CCList_delete
// 1. [prev] -> [ccDel] -> [next] (선형탐색)
// 2. [prev] -> [next] / free [ccDel]
