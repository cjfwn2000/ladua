#include "globaltool.h"
#include "tdevchannel.h"

#include <unistd.h> //FOR UNIT TEST usleep

#define SS_TDEV_FILE "/dev/ttyACM0"
#define SS_TDEV_BAUD B115200

int main(int argc, char ** argv)
{
    int rc;
    TdevChannel tdchan;
    //ClientChannel clchan;

    // 여기는 메인
    logInfo("%s on line.", argv[0]);
    
    // UNIT TEST BED
    char buf_td[256] = {'\0', };
    int i, n;
    rc = TdevChannel_init(&tdchan, SS_TDEV_FILE, SS_TDEV_BAUD);
    if(rc != 0)
    {
        logInfo("Error on init tdev (%d)", rc);
        logInfo("Please check if the target device is on-line or you have root previlege.");
        return 1;
    }
    
    for(i=0; i<7; i++)
    {
        logInfo("Recv %d", i);
        n = TdevChannel_recv(&tdchan, buf_td, sizeof buf_td - 1);
        if(n >= 0)
            logInfo("(%d) %.*s", n, n, buf_td);
        else { //negative
            logInfo("(%d) ==Broken==", n);
            break;
        }   
        usleep(1300 * 1000);
    }
    logInfo("TdevChannel_send");
    char samplebuf_cl[256] = "";
    int n_cl = 0;
    n = TdevChannel_send(&tdchan, samplebuf_cl, n_cl);
    logInfo("Writed = %d", n);
    for(i=0; i<7; i++)
    {
        logInfo("Recv %d", i);
        n = TdevChannel_recv(&tdchan, buf_td, sizeof buf_td - 1);
        if(n >= 0)
            logInfo("(%d) %.*s", n, n, buf_td);
        else { //negative
            logInfo("(%d) ==Broken==", n);
            break;
        }   
        usleep(1300 * 1000);
    }

    rc = TdevChannel_finish(&tdchan);
    logInfo("finish code of %d", rc);
    ////

    /*
    // 지금부터는 accept하는 쓰레드
    logInfo("Thread start: accepting");
    logInfo("Thread end: accepting");
    
    // 지금부터는 클라이언트1과 대화하는 쓰레드
    logInfo("Thread start: ClientChannel1");
    char buf_td[256];  //buffer for TdevChannel_recv
    int n_td; //return from TdevChannel_recv
    char buf_cl[256];  //buffer for ClientChannel_recv
    int n_cl; //return from ClientChannel_recv
    while(1) {
        // Polling: TDev
        //TODO if(TdevMsgQ_len(tdmq) > 0) ...
        //본래 TdevChannel은 '중앙측' 쓰레드만 다루는 것이지만 여기서는 임시로 직접 다룸
        n_td = TdevChannel_recv(&tdchan, buf_td, sizeof buf_td);  //should be non-block
        if(n_td > 0) { //sane
            // tdev -> ss -> client
            logInfo("TDev recv (%d bytes)", n_td);
            ClientChannel_send(&clchan, buf_td, n_td);
        } else if (n_td == 0) { //sane, 기기출력 아직 없음
            //대기
        } else { //n_td < 0; 기기 EOF
            logInfo("[!] TDev channel broken.");
            TdevChannel_sendStr(&clchan, "[!] Target device channel has been broken.");
            break;
        }

        // Polling: ClientChannel
        n_cl = ClientChannel_recv(&clchan, buf_cl, sizeof buf_cl);
        if(n_cl > 0) { //sane
            // tdev <- ss <- client
            TdevChannel_send(&tdchan, buf_cl, n_cl);
        } else if (n_cl == 0) { //sane, 클라이언트출력 아직 없음
            //대기
        } else { //n_cl < 0; 클라이언트 EOF
            logInfo("[!] Client channel broken.");
            break;
        }
    }
    TdevChannel_close(); //TODO 이것도 중앙측 쓰레드가 해야...
    ClientChannel_close();
    logInfo("Thread end: ClientChannel1");
    */

    return 0;
}
