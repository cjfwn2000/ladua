/**
 * Mobidigm
 * CNU Smartdatalab Lee cheolju
 */

/**
 * 원격 셸 따위로 접속한 클라이언트와에 데이터를 주고받도록 하는 통로 역할을 합니다. 
 * 문자열 데이터를 받거나 보내거나 접속 종료하는 기능을 활용하는 데 쓰입니다. 
 * 기능 활용 전에 반드시 초기화(ClientChannel_init)되어야 합니다.
 * ClientChannelList의 노드로도 쓰입니다. 따라서 ClientChannel은 
 * CCList_addNew로도 생성될 수 있습니다.
 */
typedef struct _ClientChannel ClientChannel;
ClientChannel * ClientChannel_next(ClientChannel * c);

/**
 * ClientChannel 관련 기능을 일괄처리하도록 도와주는 연결리스트(Linked List)입니다.
 */
typedef struct _ClientChannelList ClientChannelList;
void CCList_init(ClientChannelList * l);
ClientChannel * CCList_addNew(ClientChannelList * l);
void CCList_delete(ClientChannelList * l, ClientChannel * ccDel);
ClientChannel * CCList_first(ClientChannelList * l);  //List head
void CCList_clear(ClientChannelList * l);

// TODO: CCList_delete
// 1. [prev] -> [ccDel] -> [next] (선형탐색)
// 2. [prev] -> [next] / free [ccDel]
// or
// 1. head -> [ccDel] -> [next] (선형탐색, but no prev)
// 2. head -> [next]
// https://github.com/skorks/c-linked-list/blob/master/linkedlist.c#L59
