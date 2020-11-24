/**
 * Mobidigm
 * CNU Smartdatalab Lee cheolju
 */

#include <libssh/server.h>
#include "libtelnet.h"

typedef enum _ClientType {
    SSH,
    TELNET,
} ClientType;

/**
 * 텔넷 접속 입출력을 제대로 다루기 위해 존재하는 구조체입니다.
 * CCList_addNewFromTelnet에 넘겨줘야 할 것이 이것입니다.
 */
typedef struct _TelnetBackpack {
    int sock;
    telnet_t * tracker;
    
    //temp** : 텔넷 프로그래밍을 위해 임시로 쓰이는 메모리공간
    //libtelnet이 독특한 핸들러방식을 요구하기에 어쩔 수 없이 이렇게 쓰게 되었습니다.
    char * tempRecvBuf;  //입력 파라메터: 받을 곳 포인터
    int tempRecvBufSize;  //입력 파라메터: 받을 곳의 최대 양
    int tempRecvBytes;  //출력 파라메터: 받은 양
    int tempDataTypeFlag;  //출력 파라메터: 받은 데이터가 실제 ClientChannel에 부합한 데이터인가?
    
} TelnetBackpack;

/**
 * ClientChannel을 만들기 전 임시로 주어지는 TelnetBackpack 객체입니다. 
 * 클라이언트가 인증절차를 밟도록 (계정정보 입력 요구) 합니다. 
 * 이후 return된 객체는 세션이 확보된 상태이므로, 
 * 이 객체를 CCList_addNewFromTelnet에 넘길 수 있습니다. 
 * @param sock 클라이언트와 연결된 소켓 FD; 후술하는 return 객체에 들어감
 * @param isAccountValid 인증에 필요한 function; (username, password) -> (TRUE/FALSE)
 * @returns 신규 TelnetBackpack 객체를 가리키는 포인터, 세션 확보 상태; 인증 실패시 NULL
 * @warning sock을 close하는 의무는 아직 ClientChannel 모듈로 넘어오지 않았습니다. 따라서 인증 실패 시 더 이상 쓸모가 없어진 sock은 호출자가 직접 닫아야 합니다.
 */
TelnetBackpack * TelnetBackpack_newWithAuth(int sock, int (*isAccountValid)(const char * username, const char * pass));


/**
 * 원격 셸 따위로 접속한 클라이언트와에 데이터를 주고받도록 하는 통로 역할을 합니다. 
 * 문자열 데이터를 받거나 보내거나 접속 종료하는 기능을 활용하는 데 쓰입니다. 
 * 스택변수로 직접 선언했다면 기능 활용 전에 반드시 초기화(ClientChannel_init...)되어야 합니다.
 * 
 * ClientChannelList의 노드로도 쓰입니다. 따라서 신규 ClientChannel은 
 * CCList_addNewFromSSH 등등으로도 만들어질 수 있습니다.
 */
typedef struct _ClientChannel {
    ClientType type; //SSH or TELNET
    ssh_session sshSess;  //ssh접속일 때만
    ssh_channel sshChan;  //ssh접속일 때만
    TelnetBackpack * telnetBackpack;  //텔넷접속일 때만
    struct _ClientChannel * next;  //다음노드
} ClientChannel;

/**
 * ClientChannel 객체를 초기화합니다. (SSH) 
 * @param sessOpened SSH 세션 객체를 가리키지만 해당 클라이언트가 셸에 접속된 상태여야 합니다. 
 * @param chan sessOpened에서 파생된 ssh_channel 객체여야 합니다. 데이터 송수신을 위해 필요합니다.
 */
int ClientChannel_initFromSsh(ClientChannel * c, ssh_session sessOpened, ssh_channel chan);
//TODO documentations....
int ClientChannel_initFromTelnet(ClientChannel * c, TelnetBackpack * tbp);
int ClientChannel_recv(ClientChannel * c, char * buf, int nbytes);
int ClientChannel_send(ClientChannel * c, const char * buf, int nbytes);
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
 * @param sessOpened SSH 세션 객체를 가리키지만 해당 클라이언트가 셸에 접속된 상태여야 합니다. 
 * @param chan sessOpened에서 파생된 ssh_channel 객체여야 합니다. 데이터 송수신을 위해 필요합니다.
 * @returns 그 추가된 객체로의 포인터
 */
ClientChannel * CCList_addNewFromSSH(ClientChannelList * l, ssh_session sessOpened, ssh_channel chan);
/**
 * 신규 ClientChannel(텔넷접속)을 만들어 리스트에 추가합니다. 
 * @param tbp TelnetBackpack 객체를 가리키지만 해당 클라이언트가 셸에 접속된 상티여야 합니다.
 * @returns 그 추가된 객체로의 포인터
 */
ClientChannel * CCList_addNewFromTelnet(ClientChannelList * l, TelnetBackpack * tbp);
/**
 * 일괄처리입니다. 각 ClientChannel마다: 
 * - 받을 데이터가 있으면 그것을 인자로 삼는 fnRecvData을 호출합니다.
 * - 연결이 끊어져있다면 종료하고 제거합니다.
 * @param fnRecvData 각 클라이언트로부터 받은 데이터 처리 function (받은데이터, 그 길이)
 */
void CCList_batchRecv(ClientChannelList * l, void (*fnRecvData)(const char *, int));
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
