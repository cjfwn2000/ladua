#include "globaltool.h"
#include "clientchannel.h"
#include "tdevchannel.h"
#include "libtelnet.h"
#include <libssh/server.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>  //getopt
#include <stdlib.h>  //atoi
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#define DEFAULT_TDEV_FILE "/dev/ttyACM0"
#define DEFAULT_TDEV_BAUD B115200
#define DEFAULT_SSH_PORT 10001
#define SSH_PATH_DSAKEY "/etc/ssh/ssh_host_dsa_key"
#define SSH_PATH_RSAKEY "/etc/ssh/ssh_host_rsa_key"
#define RECVBUF_SIZE 256
#define DEFAULT_TELNET_PORT 10002
#define ACCOUNT_USER "mobidigm"
#define ACCOUNT_PASS "dj2020"

// Global variables
/** 메인 루틴이 종료해야 하는지 알려줌 */
static int mainStopFlag = 0;
/** SSH Server Bind object */
static ssh_bind sshBind;
/** Telnet Server Listening socket */
static int telnetListenSock;
static struct sockaddr_in telnetListenAddr = {0};
/** Target Device Channel */
static TdevChannel tdchan;
/** Client Channel List */
static ClientChannelList cclist;
static sem_t mutex_cclist;  //cclist를 main, acceptor 쓰레드 둘 다 접근할 수 있기 때문

/**
 * SIGINT를 받으면 메인 루틴을 종료하게 함
 */
static void sigintHandlerToStop(int signum) {
    logInfo("--- Recieved Interruption. ---");
    mainStopFlag = 1;
}

/**
 * 주어진 user, pass가 올바른 계정인지 확인
 * @returns 올바를 시 1(True), 아닐 시 0(False)
 */
static int isAccountValid(const char * user, const char * pass) {
    return (strcmp(user, ACCOUNT_USER) == 0) && (strcmp(pass, ACCOUNT_PASS) == 0);
}

/**
 * sshBind를 세팅하여 SSH서버로서 ssh_bind_accept가 가능하도록 함
 * @returns 성공시 0, 실패시 음수
 */
static int makeSshAcceptable(unsigned int bindport) {
    sshBind = ssh_bind_new();
    ssh_bind_options_set(sshBind, SSH_BIND_OPTIONS_DSAKEY, SSH_PATH_DSAKEY);
    ssh_bind_options_set(sshBind, SSH_BIND_OPTIONS_RSAKEY, SSH_PATH_RSAKEY);
    ssh_bind_options_set(sshBind, SSH_BIND_OPTIONS_BINDPORT, &bindport);
    
    if(ssh_bind_listen(sshBind) < 0) {
        logInfo("Error listening to socket: %s", ssh_get_error(sshBind));
        return -1;
    }
    return 0;
}

/**
 * telnetListenSock를 세팅하여 Telnet서버로서 accept가 가능하도록 함
 * @returns 성공시 0, 실패시 음수
 */
static int makeTelnetAcceptable(unsigned int bindport) {
    int rs = 1; //reuse address option

    if( (telnetListenSock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        logInfo("Error creating a new socket: %s", strerror(errno));
        return -1;
    }
    setsockopt(telnetListenSock, SOL_SOCKET, SO_REUSEADDR, (char*)&rs, sizeof rs);
    
    // Binding
    telnetListenAddr.sin_family = AF_INET;
    telnetListenAddr.sin_addr.s_addr = INADDR_ANY;
    telnetListenAddr.sin_port = htons(bindport);
    if((bind(telnetListenSock, (struct sockaddr *)&telnetListenAddr,
            sizeof telnetListenAddr)) < 0) {
        logInfo("Error invoking posix bind: %s", strerror(errno));
        close(telnetListenSock);
        return -1;
    }

    // Listening
    if ( listen(telnetListenSock, 5) == -1 ) {
        logInfo("Error invoking posix listen: %s", strerror(errno));
        close(telnetListenSock);
        return -1;
    }
    return 0;
}

/**
 * 더 이상 sshBind 필요없을 때
 */
static void finalizeSshAcception() {
    ssh_bind_free(sshBind);
}

/**
 * 더 이상 telnetListenSock 필요없을 때
 */
static void finalizeTelnetAcception() {
    close(telnetListenSock);
}

// --------------------------

/**
 * trSshAcceptor가 cancel되었을 때 메모리를 청소하기 위한 function.
 * @param payload sessionNewb (청소대상)
 */
static void trSshAcceptor_cleanup(void * payload)
{
    ssh_session sessionNewb = (ssh_session) payload;
    ssh_free(sessionNewb);
    logInfo("[trSshAcceptor_cleanup]");
}

/**
 * 클라이언트 SSH접속 등록(cclist)을 담당하는 쓰레드 루틴. 
 * makeSshAcceptable이 사전에 실행되어야 함. 
 * 기간: 메인 루틴이 가동중일 동안 항상
 * @param payload 미사용
 */
static void * trSshAcceptor(void * payload)
{
    #define LOGPREFIX "[trSshAcceptor] "
    int rc = 0;
    ssh_session sessionNewb = 0;  //매 턴마다 새로 배당됨
    ssh_message message = 0;
    ssh_channel chanNewb = 0;
    int authenticated = 0;
    int shellRequested = 0;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    logInfo(LOGPREFIX "Starting SSH client-acception.");
    while(1) {
        sessionNewb = ssh_new();
        pthread_cleanup_push(trSshAcceptor_cleanup, (void*)sessionNewb);
        //여기서 pthread_cleanup_pop 사이에 thread cancel이 들어와도 sessionNewb을 정리할 수 있게 한다.
        rc = ssh_bind_accept(sshBind, sessionNewb);
        if(rc == SSH_ERROR) {
            logInfo(LOGPREFIX "Error accepting a connection: %s", ssh_get_error(sshBind));
            ssh_free(sessionNewb);
            continue;
        }
        pthread_cleanup_pop(0);

        logInfo(LOGPREFIX "New client connected. checking...");
        // 1. Key exchange
        if( ssh_handle_key_exchange(sessionNewb) ) {
            logInfo(LOGPREFIX "Error ssh_handle_key_exchange: %s", ssh_get_error(sessionNewb));
            ssh_disconnect(sessionNewb);
            ssh_free(sessionNewb);
            continue;
        }
        // 2. Authentication
        authenticated = 0;
        do {
            message = ssh_message_get(sessionNewb);
            if(!message)
                break;
            switch(ssh_message_type(message)) {
            case SSH_REQUEST_AUTH:
                switch(ssh_message_subtype(message)){
                case SSH_AUTH_METHOD_PASSWORD:
                    logInfo(LOGPREFIX "User %s tries to auth.", ssh_message_auth_user(message));
                    if( isAccountValid(ssh_message_auth_user(message), ssh_message_auth_password(message)) ) {
                        authenticated = 1;
                        ssh_message_auth_reply_success(message, 0);
                        break;  // 성공시 아래 case ...로 내려가지 마라
                    }
                case SSH_AUTH_METHOD_NONE: 
                default:
                    ssh_message_auth_set_methods(message, SSH_AUTH_METHOD_PASSWORD);
                    ssh_message_reply_default(message);
                }
                break;
            default:
                ssh_message_reply_default(message);
            }
            ssh_message_free(message);
        } while (!authenticated);
        if(!authenticated) {
            logInfo(LOGPREFIX "Error Authentication", ssh_get_error(sessionNewb));
            ssh_disconnect(sessionNewb);
            ssh_free(sessionNewb);
            continue;
        }
        // 3. Wait for a channel session (여기서 ssh_channel객체 준비됨)
        chanNewb = 0;
        do {
            message = ssh_message_get(sessionNewb);
            if(message) {
                switch(ssh_message_type(message)) {
                case SSH_REQUEST_CHANNEL_OPEN:
                    if(ssh_message_subtype(message) == SSH_CHANNEL_SESSION) {
                        chanNewb = ssh_message_channel_request_open_reply_accept(message);
                        break;
                    }
                default:
                    ssh_message_reply_default(message);
                }
                ssh_message_free(message);
            }
        } while(message && !chanNewb);
        if(!chanNewb) {
            logInfo(LOGPREFIX "Error: Client did not ask for a channel session (%s)", ssh_get_error(sessionNewb));
            ssh_disconnect(sessionNewb);
            ssh_free(sessionNewb);
            continue;
        }
        // 4. Wait for a shell
        shellRequested = 0;
        do {
            message = ssh_message_get(sessionNewb);
            if(message != NULL) {
                if(ssh_message_type(message) == SSH_REQUEST_CHANNEL &&
                   ssh_message_subtype(message) == SSH_CHANNEL_REQUEST_SHELL) {
                    shellRequested = 1;
                    ssh_message_channel_request_reply_success(message);
                    ssh_message_free(message);
                    break;
                }
                ssh_message_reply_default(message);
                ssh_message_free(message);
            } else {
                break;
            }
        } while(!shellRequested);
        if(!shellRequested) {
            logInfo(LOGPREFIX "Error: No shell requested (%s)", ssh_get_error(sessionNewb));
            ssh_channel_free(chanNewb);
            ssh_disconnect(sessionNewb);
            ssh_free(sessionNewb);
            continue;
        }

        logInfo(LOGPREFIX "Success: Shell-opened session");
        // 클라이언트 리스트에 추가한다.
        sem_wait(&mutex_cclist);
        if(!mainStopFlag)
            CCList_addNewFromSSH(&cclist, sessionNewb, chanNewb);
        else {
            logInfo(LOGPREFIX "Because of mainStopFlag, discarding the very last client session.");
            ssh_channel_free(chanNewb);
            ssh_disconnect(sessionNewb);
            ssh_free(sessionNewb);
        }
        sem_post(&mutex_cclist);
    }

    logInfo(LOGPREFIX "Finished.");
}

/**
 * 클라이언트 Telnet접속 등록(cclist)을 담당하는 쓰레드 루틴. 
 * makeTelnetAcceptable이 사전에 실행되어야 함. 
 * 기간: 메인 루틴이 가동중일 동안 항상
 * @param payload 미사용
 */
static void * trTelnetAcceptor (void * payload) {
    #define LOGPREFIX "[trTelnetAcceptor] "
    static const telnet_telopt_t telopts[] = {
        { TELNET_TELOPT_COMPRESS2,	TELNET_WILL, TELNET_DONT },
        { -1, 0, 0 }
    };
    int rc = 0;
    socklen_t addrlen;
    int clientsockNewb = 0;
    telnet_t * telnettrackerNewb;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    logInfo(LOGPREFIX "Starting Telnet client-acception.");
    while(1) {
        addrlen = sizeof(telnetListenAddr);
        clientsockNewb = accept( telnetListenSock,
            (struct sockaddr *)&telnetListenAddr,
            &addrlen );  //Blocking
        if( clientsockNewb < 0 ) {
            logInfo(LOGPREFIX "Error accepting a connection: %s", strerror(errno));
            continue;
        }

        logInfo(LOGPREFIX "New client connected. checking...");
        // Authentication
        //telnettrackerNewb = telnet_init(opts, eh, 0, )
        telnettrackerNewb = ClientChannel_newTempTelnett();
        telnet_negotiate(telnettrackerNewb, TELNET_WILL, TELNET_TELOPT_COMPRESS2);
        telnet_printf(telnettrackerNewb, "Entern name: ");
        telnet_negotiate(telnettrackerNewb, TELNET_WILL, TELNET_TELOPT_ECHO);

        // TODO CCList 등록할 적에 넘겨줘야 할 물건: clientsockNewb, telnettrackerNewb
    }

    logInfo(LOGPREFIX "Finished.");
}


// -------------------------------------------------

/**
 * CCList_batchRecv에 쓰일 function
 */
static void sendEachToTdev(const char * recvBuf, int nbytes)
{
    // recvBuf[0..nbytes-1]
    logInfo("< A client sends...");
    logDump('<', recvBuf, nbytes);
    TdevChannel_send(&tdchan, recvBuf, nbytes);
}

// -------------------------------------------------

int main(int argc, char ** argv)
{   
    int recvBytes = 0;  //TdevChannel_recv
    char recvBuf[RECVBUF_SIZE] = {'\0'};  //TdevChannel_recv
    // threads
    pthread_t tidSshAcceptor;
    pthread_t tidTelnetAcceptor;
    // 초기화 설정값; 사용자로부터 가져올 수도 있음.
    char tdevFile[128] = DEFAULT_TDEV_FILE;
    unsigned int tdevBaud = DEFAULT_TDEV_BAUD;
    unsigned int bindportSsh = DEFAULT_SSH_PORT;
    unsigned int bindportTelnet = DEFAULT_TELNET_PORT;

    // 사용자로부터 설정값 가져오기
    // Usage: serialserver -f /dev/ttyACM0 -b 115200 -s 10001 -t 10002
    int opt;
    while( (opt=getopt(argc, argv, "f:b:s:t:")) != -1 ) {
        switch(opt) {
        case 'f': //File path dev
            strncpy(tdevFile, optarg, sizeof tdevFile - 1);
            break;
        case 'b': //Baud rate
            logInfo("Argument '-b' : Sorry, not implemented.");
            break;
        case 's': //port SSH
            bindportSsh = atoi(optarg);
            break;
        case 't': //port Telnet
            bindportTelnet = atoi(optarg);
            break;
        }
    }

    // 초기화 단계
    logInfo("Initializing the SSH server...");
    if(makeSshAcceptable(bindportSsh) < 0) {
        logInfo("Failed in making SSH server acceptable.");
        logInfo("Please check if you don't have enough permission.");
        return 1;
    }
    logInfo("Initializing the telnet server...");
    if(makeTelnetAcceptable(bindportTelnet) < 0) {
        logInfo("Failed in making the telnet server acceptable.");
        logInfo("Please check if you don't have enough permission.")
    }
    logInfo("Initializing the target device channel...");
    if(TdevChannel_init(&tdchan, tdevFile, tdevBaud) < 0) {
        logInfo("Failed in initializing the target device channel.");
        logInfo("Please check if the device isn't connected");
        logInfo("or you don't have enough permission.");
        return 1;
    }
    CCList_init(&cclist);
    sem_init(&mutex_cclist, 0, 1);
    if(pthread_create(&tidSshAcceptor, NULL, trSshAcceptor, NULL)) {
        logInfo("Failed in pthread_create tidSshAcceptor.");
        return 1;
    }
    if(pthread_create(&tidTelnetAcceptor, NULL, trTelnetAcceptor, NULL)) {
        logInfo("Failed in pthread_create tidTelnetAcceptor.");
        return 1;
    }
    // 프로그램 중단 명령 핸들링
    signal(SIGINT, sigintHandlerToStop);

    // 메인 쓰레드 루틴 돌입
    logInfo("Entering the main routine...");
    while(!mainStopFlag) {
        // tdev->clients
        recvBytes = TdevChannel_recv(&tdchan, recvBuf, sizeof recvBuf);
        if (recvBytes > 0) { //보낼 데이터 있음
            logDump('>', recvBuf, recvBytes);
            sem_wait(&mutex_cclist);
            CCList_batchSend(&cclist, recvBuf, recvBytes);
            sem_post(&mutex_cclist);
        } else if (recvBytes < 0) { //Error or EOF
            logInfo("[TdevChannel] Disconnected. mainStopFlag");
            mainStopFlag = 1;
            break;
        }
        // clients->tdev
        sem_wait(&mutex_cclist);
        CCList_batchRecv(&cclist, sendEachToTdev);
        sem_post(&mutex_cclist);
    }
    // Main program stops

    logInfo("Stopping the device...");
    TdevChannel_finalize(&tdchan);
    
    logInfo("Closing all the client channels...");
    sem_wait(&mutex_cclist);
    CCList_finalize(&cclist);
    sem_post(&mutex_cclist);

    logInfo("Stopping client-acception routine (SSH)...");
    finalizeSshAcception();
    finalizeTelnetAcception();
    pthread_cancel(tidSshAcceptor); //accept에 블럭돼있으면 빠져나오게 해준다.
    pthread_join(tidSshAcceptor, NULL);
    pthread_cancel(tidTelnetAcceptor);
    pthread_join(tidTelnetAcceptor, NULL);

    logInfo("Finished.");
    return 0;
}
