#include "globaltool.h"
#include "clientchannel.h"
#include "tdevchannel.h"
#include <libssh/server.h>
#include <signal.h>
#include <pthread.h>

// TODO 데이터가 하드코딩 상태;
#define TDEV_FILE "/dev/ttyACM0"
#define TDEV_BAUD B115200
#define SSH_PORT 10001
#define RECVBUF_SIZE 256

/**
 * Global Variable - 메인 루틴이 종료해야 하는지 알려줌
 */
static int mainStopFlag = 0;

/**
 * SIGINT를 받으면 메인 루틴을 종료하게 함
 */
static void sigintHandlerToStop(int signum) {
    logInfo("--- Recieved Interruption. Exiting... ---");
    mainStopFlag = 1;
}


// tr = Thread Routine

/**
 * 클라이언트 SSH접속 등록을 담당하는 쓰레드 루틴
 * 기간: 메인 루틴이 가동중일 동안 항상
 * @param payload 메인쓰레드에서 운용하는 ClientChannelList를 가리킴,
 *  여기에 접속 완료된 ClientChannel을 등록(add). 
 */
static void * trSshAcceptor(void * payload)
{
    // Stop시: bind 정리
    // TODO ssh_bind_accept에서 여전히 blocking상태일텐데 Stop검사는 언제하나?
    // 해법?) 자식 쓰레드를 더 두자: mainStopFlag를 감시(polling)하여, 걸릴 때 부모의 bind를 free한다.
    // 추가정보) ssh_bind_free는 bind 내 소켓파일을 close하는 효과가 있다.
    // https://github.com/substack/libssh/blob/master/src/bind.c

    #define LOGPREFIX "[trSshAcceptor] "
    ClientChannelList * cclp = (ClientChannelList *) payload;
    
    logInfo(LOGPREFIX "trSshAcceptor Started.");
}

int main(int argc, char ** argv)
{   
    // Channels
    TdevChannel tdchan;
    ClientChannelList cclist;
    int recvBytes = 0;  //TdevChannel_recv
    char recvBuf[RECVBUF_SIZE] = {'\0'};  //TdevChannel_recv
    // A thread
    pthread_t tidSshAcceptor;

    // 여기는 메인쓰레드
    logInfo("%s : Starting...", argv[0]);

    // 초기화 스테이지
    if(pthread_create(&tidSshAcceptor, NULL, trSshAcceptor, NULL)) {
        logInfo("Failed in pthread_create tidSshAcceptor.");
        return 1;
    }
    TdevChannel_init(&tdchan, TDEV_FILE, TDEV_BAUD);
    CCList_init(&cclist);
    // 프로그램 중단 명령 핸들링
    signal(SIGINT, sigintHandlerToStop);

    // 메인 루틴 돌입
    while(!mainStopFlag) {
        // tdev->clients
        recvBytes = TdevChannel_recv(&tdchan, recvBuf, sizeof recvBuf);
        if (recvBytes > 0) { //보낼 데이터 있음
            CCList_batchSend(&cclist, recvBuf, recvBytes);
        } else if (recvBytes < 0) { //Error or EOF
            logInfo("[TdevChannel] Broken pipe. mainStopFlag");
            mainStopFlag = 1;
        }
        // clients->tdev
        CCList_batchRecv(&cclist, sendToTdev);
    }
    // Main program stops
    // TODO
    

    /*
    쓰레드시작_SshAcceptorThread(Main의clientChannelList); //생존: MainThread가 종료하라 전까지 //의무: ssh listen and accept
    // idea: clientChannelList는 MainThread와 AcceptorThread 둘 다 접근할 것이므로 이에 대한 mutex가 필요할 것

    // 메인 루틴 돌입
    while(!mainStopFlag) {
        //tdev->clients
        poll(tdev);
        if("got_output"){
            전달_clients에게(clientChannelList);
        } else if("broken") {
            break;
        }
        //clients->tdev
        for (clchan in clientChannelList) { //idea: 아무래도 clientChannelList는 Linked List로 만드는 게 좋겠다
            poll(clchan);
            if("got_output"){
                전달_tdev에게();
            } else if("broken") {
                clchan_종료();  //ssh_disconnect어쩌구...
            }
        }
    }
    // Main program stops
    join_thread(acceptor);
    종료_clients(clientChannelList);
    */

    return 0;
}
