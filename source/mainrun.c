#include "globaltool.h"
#include "clientchannel.h"
#include "tdevchannel.h"
#include <libssh/server.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

// TODO 데이터가 하드코딩 상태;
#define TDEV_FILE "/dev/ttyACM0"
#define TDEV_BAUD B115200
#define SSH_PORT 10001
#define SSH_PATH_DSAKEY "/etc/ssh/ssh_host_dsa_key"
#define SSH_PATH_RSAKEY "/etc/ssh/ssh_host_rsa_key"
#define RECVBUF_SIZE 256

#define ACCOUNT_USER "mobidigm"
#define ACCOUNT_PASS "dj2020"

// Global variables
/** 메인 루틴이 종료해야 하는지 알려줌 */
static int mainStopFlag = 0;
/** SSH Server bind object */
ssh_bind sshBind;
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
 * sshBind를 세팅하여 SSH서버로서 ssh_bind_accept()가 가능하도록 함
 * @returns 성공시 0, 실패시 음수
 */
static int makeSshAcceptable() {
    const unsigned int bindport = SSH_PORT;

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
 * 더 이상 sshBind 필요없을 때; ssh_bind_accept block되어 있다면 그게 해제된다.
 */
static void finalizeSshAcception() {
    ssh_bind_free(sshBind);
}

/**
 * 클라이언트 SSH접속 등록(cclist)을 담당하는 쓰레드 루틴. 
 * sshBind 사용. 
 * 기간: 메인 루틴이 가동중일 동안 항상
 * @param payload 미사용
 */
static void * trSshAcceptor(void * payload)
{
    // 추가정보) ssh_bind_free는 bind 내 소켓파일을 close하는 효과가 있다.
    // 이때 ssh_bind_accept는 return SSH_ERROR를 한다.
    // https://github.com/substack/libssh/blob/master/src/bind.c

    #define LOGPREFIX "[trSshAcceptor] "
    int rc = 0;
    ssh_session sessionNewb = 0;  //매 턴마다 새로 배당됨
    ssh_message message = 0;
    ssh_channel chanNewb = 0;
    int authenticated = 0;
    int shellRequested = 0;

    // 선조건: ssh_bind_accept(sshBind)가 사용 가능하다
    logInfo(LOGPREFIX "Starting client-acception.");
    while(1) {
        sessionNewb = ssh_new();
        rc = ssh_bind_accept(sshBind, sessionNewb);
        if(rc == SSH_ERROR) {
            if(mainStopFlag) { //메인 중단 명령이 있었기에 accept 중단
                logInfo(LOGPREFIX "Got mainStopFlag, Stopping acception.");
                break;
            } else { //별개의 문제로 SSH 에러 발생
                logInfo(LOGPREFIX "Error accepting a connection: %s", ssh_get_error(sshBind));
                ssh_free(sessionNewb);
                continue;
            }
        }

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
            case SSH_REQUEST_CHANNEL:
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
    return NULL;
}

/**
 * CCList_batchRecv에 쓰일 function
 */
static void sendEachToTdev(const char * recvBuf, int nbytes) {
    // recvBuf[0..nbytes-1]
    TdevChannel_send(&tdchan, recvBuf, nbytes);
}

int main(int argc, char ** argv)
{   
    int recvBytes = 0;  //TdevChannel_recv
    char recvBuf[RECVBUF_SIZE] = {'\0'};  //TdevChannel_recv
    // A thread
    pthread_t tidSshAcceptor;

    logInfo("%s : Starting...", argv[0]);

    // 초기화 단계
    if(makeSshAcceptable() < 0) {
        logInfo("Failed in making SSH server acceptable.");
        logInfo("Please check if you don't have enough permission.");
        return 1;
    }
    logInfo("Initializing the target device channel...");
    if(TdevChannel_init(&tdchan, TDEV_FILE, TDEV_BAUD) < 0) {
        logInfo("Failed in initializing the target device channel.");
        logInfo("Please check if you don't have enough permission.");
    }
    CCList_init(&cclist);
    sem_init(&mutex_cclist, 0, 1);
    if(pthread_create(&tidSshAcceptor, NULL, trSshAcceptor, NULL)) {
        logInfo("Failed in pthread_create tidSshAcceptor.");
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

    logInfo("Stopping client-acception routine...");
    finalizeSshAcception();
    pthread_join(tidSshAcceptor, NULL); 
    //pthread_join(tidTelnetAcceptor, NULL);

    logInfo("Finished.");
    return 0;
}
