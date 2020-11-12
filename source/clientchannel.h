/**
 * Mobidigm
 * CNU Smartdatalab Lee cheolju
 */

#include <libssh/server.h>

/**
 * 원격 셸 따위로 접속한 클라이언트와에 데이터를 주고받도록 하는 통로 역할을 합니다. 
 * 문자열 데이터를 받거나 보내거나 접속 종료하는 기능을 활용하는 데 쓰입니다. 
 * 구조체변수로 직접 선언했다면 기능 활용 전에 
 * 반드시 초기화(ClientChannel_init...)되어야 합니다.
 * 
 * ClientChannelList의 노드로도 쓰입니다. 따라서 신규 ClientChannel은 
 * CCList_addNewSSH 등등으로도 만들어질 수 있습니다.
 */
typedef struct _ClientChannel {
    ClientChannel * next;
} ClientChannel;

/**
 * ClientChannel 객체를 초기화합니다. (SSH) 
 * @param sessOpened SSH 세션 객체를 가리키지만 클라이언트와 채널이 열려있는 상태여야 합니다.
 */
int ClientChannel_initFromSsh(ClientChannel * c, ssh_session sessOpened);
ClientChannel * ClientChannel_next(ClientChannel * c);


/**
 * ClientChannel 관련 기능을 일괄처리하도록 도와주는 연결리스트(Linked List)입니다.
 */
typedef struct _ClientChannelList {
    ClientChannel * head; //처음
    ClientChannel * tail; //끝
} ClientChannelList;

void CCList_init(ClientChannelList * l);
/** 신규 ClientChannel(SSH접속)을 만들어 추가하고, 그 포인터를 return합니다. */
ClientChannel * CCList_addNewFromSSH(ClientChannelList * l);
void CCList_delete(ClientChannelList * l, ClientChannel * ccDel);
ClientChannel * CCList_first(ClientChannelList * l);  //List head
void CCList_clear(ClientChannelList * l);
