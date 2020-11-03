#include "globaltool.h"

int main(int argc, char ** argv)
{
    TdevChannel tdchan;
    ClientChannel clchan;

    // 여기는 메인
    logInfo("%s on line.", argv[0]);
    
    // 지금부터는 accept하는 쓰레드
    logInfo("Thread start: accepting");
    logInfo("Thread end: accepting");
    
    // 지금부터는 클라이언트1과 대화하는 쓰레드
    logInfo("Thread start: ClientChannel1");
    char buf_td[256];  //buffer for TdevChannel_read
    int n_td; //return from TdevChannel_read
    char buf_cl[256];  //buffer for ClientChannel_read
    int n_cl; //return from ClientChannel_read
    while(1) {
        // Polling: TDev
        //if(TdevMsgQ_len(tdmq) > 0) ...
        //본래 TdevChannel은 '중앙측' 쓰레드만 다루는 것이지만 여기서는 임시로 직접 다룸
        n_td = TdevChannel_read(&tdchan, buf_td, sizeof buf_td);  //should be non-block
        if(n_td > 0) { //sane
            
        } else if (n_td == 0) { //sane, 기기출력 아직 없음

        } else { //n_td < 0; 기기 EOF

        }

        // Polling: ClientChannel
        n_cl = ClientChannel_read(&clchan, buf_cl, sizeof buf_cl);
        if(n_cl > 0) { //sane

        } else if (n_cl == 0) { //sane, 클라이언트출력 아직 없음
        
        } else { //n_cl < 0; 클라이언트 EOF
            
        }
    }
    logInfo("Thread end: ClientChannel1");

    return 0;
}
