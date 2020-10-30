/**
 * Target device interface (TDI); 
 * 시리얼 포트로 연결된 기기(예:MIMXRT1060-EVK)의
 * 시리얼 입출력을 구현하기 위한 모듈입니다. 
 */

typedef struct _tdi {
    int fd;  //file descriptor (기기를 가리킴)
} TDInterface;

/**
 * 시리얼포트로 연결된 기기에 접근하는 인터페이스 객체를 만들어줍니다. 
 * 그 객체를 가지고 TDI_read 등등의 입출력 기능을 이용할 수 있습니다. 
 * 더 이상 이용하지 않을 때 TDI_close로 마무리해줘야 합니다. 
 * @param tdi 초기화 대상 객체
 * @returns 성공시 0, 실패시 Non-zero
 */
int TDI_open(TDInterface * tdi);

int TDI_read(TDInterface * tdi, char * bigbuf);
