#include "globaltool.h"

#define SS_TDEV_FILE "/dev/ttyACM0"
#define SS_TDEV_BAUD B115200


int main(int argc, char ** argv)
{
    // 여기는 메인쓰레드
    logInfo("%s : Starting...", argv[0]);

    // TODO 내부흐름 시나리오를 다시 설계해야할 듯 싶다.
    // 이유? "각 연결당 쓰레드 위임 방식"은 너무 메모리에 부담이 가게 된다. 특히 메시지큐...
    // 예정 쓰레드 목록: MainThread(여기), SshAcceptorThread, AuthCallbackThread(콜백용; 즉 잠시만 쓰임; 각 클라이언트들의 처음 단계)

    쓰레드시작_SshAcceptorThread(Main의clientChannelList); //생존: MainThread가 종료하라 전까지 //의무: ssh listen and accept
    // idea: clientChannelList는 MainThread와 AcceptorThread 둘 다 접근할 것이므로 이에 대한 mutex가 필요할 것

    while(1) {
        //tdev->client
        poll(tdev);
        if("got_output"){
            전달_clients에게(clientChannelList);
        } else if("broken") {
            break;
        }
        //client->tdev
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
    종료_clients(clientChannelList);

    return 0;
}
