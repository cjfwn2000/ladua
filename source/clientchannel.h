/**
 * Mobidigm
 * CNU Smartdatalab Lee cheolju
 */

#include <libssh/server.h>

typedef enum _ClientType {
    SSH,
    TELNET,
} ClientType;

/**
 * 원격 셸 따위로 접속한 클라이언트와에 데이터를 주고받도록 하는 통로 역할을 합니다. 
 * 문자열 데이터를 받거나 보내거나 접속 종료하는 기능을 활용하는 데 쓰입니다. 
 * 스택변수로 직접 선언했다면 기능 활용 전에 반드시 초기화(ClientChannel_init...)되어야 합니다.
 * 
 * ClientChannelList의 노드로도 쓰입니다. 따라서 신규 ClientChannel은 
 * CCList_addNewSSH 등등으로도 만들어질 수 있습니다.
 */
typedef struct _ClientChannel {
    ClientType type; //SSH or TELNET
    ssh_session sshSess;  //ssh접속일 때만
    ssh_channel ssdChan;  //ssh접속일 때만
    ClientChannel * next;  //다음노드
} ClientChannel;

/**
 * ClientChannel 객체를 초기화합니다. (SSH) 
 * @param sessOpened SSH 세션 객체를 가리키지만 클라이언트와 채널이 열려있는 상태여야 합니다. 
 * 예컨데 ssh_channel_read(chan,buf,...)가 가능한 상태여야 합니다.
 */
int ClientChannel_initFromSsh(ClientChannel * c, ssh_session sessOpened);
ClientChannel * ClientChannel_next(ClientChannel * c);
void ClientChannel_close(ClientChannel * c);

/**
 * ClientChannel 관련 기능을 일괄처리하도록 도와주는 연결리스트(Linked List)입니다.
 */
typedef struct _ClientChannelList {
    ClientChannel * head; //처음노드, 초기에 NULL
} ClientChannelList;

/**
 * 리스트를 사용가능하도록 초기화합니다. 
 */
void CCList_init(ClientChannelList * l);
/**
 * 신규 ClientChannel(SSH접속)을 만들어 리스트에 추가합니다. 
 * @param sessOpened SSH 세션 객체를 가리키지만 클라이언트와 채널이 열려있는 상태여야 합니다.
 * @returns 그 추가된 객체로의 포인터
 */
ClientChannel * CCList_addNewFromSSH(ClientChannelList * l, ssh_session sessOpened);
/**
 * 일괄처리입니다. 각 ClientChannel마다
 * - 받을 데이터가 있으면 그것을 인자로 삼는 fnRecvData을 호출합니다.
 * - 연결이 끊어져있다면 종료하고 제거합니다.
 * @param fnRecvData 각 클라이언트로부터 받은 데이터 처리 function (받은데이터, 그 길이)
 */
void CCList_batchRecv(ClientChannelList * l, void (*fnRecvData)(const char *, int *));
/**
 * 일괄처리입니다. 각 ClientChannel에게 특정 데이터를 보냅니다.
 * @param sendBuf 각 클라이언트에게 보낼 데이터 버퍼 위치
 * @param sendNbytes 그 데이터의 바이트 양
 */
void CCList_batchSend(ClientChannelList * l, const char * sendBuf, int sendNBytes);
/**
 * 더 이상 작업을 하지 않을 때 호출합니다. 
 * 리스트 내 모든 ClientChannel을 종료하고 정리합니다.
 */
void CCList_finalize(ClientChannelList * l);