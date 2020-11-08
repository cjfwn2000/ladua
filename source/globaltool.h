/**
 * 모든 모듈에 공통하게 필요할 도구를 정리해놓은 곳
 */

//로깅(혹은 디버깅메시지)에 쓰임
void logInfo(const char *fmt, ... );


// 문자열데이터의 전달을 위한 Circular Decaying Queue
#define SCDQBUF_SIZE 256
typedef struct _StrdataCDQueue {
    char buf[SCDQBUF_SIZE];
    
} StrdataCDQueue;
