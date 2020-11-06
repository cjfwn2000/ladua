/**
 * Mobidigm
 * CNU Smartdatalab Lee cheolju
 */

typedef struct _TdevChannel {
    int fd;  //file descriptor to tdev
    //char * filepath
    //char * filepath
    //int baudrate;
} TdevChannel;

/**
 * 유저가 baudrate를 제공할 수 있게 하기 위함
 */
#ifndef _TERMIOS_H
#define  B57600    0010001
#define  B115200   0010002
#define  B230400   0010003
#define  B460800   0010004
#define  B500000   0010005
#define  B576000   0010006
#define  B921600   0010007
#define  B1000000  0010010
#define  B1152000  0010011
#define  B1500000  0010012
#define  B2000000  0010013
#define  B2500000  0010014
#define  B3000000  0010015
#define  B3500000  0010016
#define  B4000000  0010017
#define __MAX_BAUD B4000000
#endif

/**
 * 
 * 정상 실행시 t가 목표기기에 연결되어 다른 문제 없는 한 추후 여러 실행이 가능하게 됩니다.
 * 
 * @param baudrate 목표기기의 전송 속도 (Baud rate); e.g. B115200
 * @returns 정상시 0, 에러시 Non-zero
 */
int TdevChannel_init(TdevChannel * t, const char * filepath, unsigned int baudrate);

/**
 * 
 * @param buf 읽을 데이터 들어가는 공간
 * @param nbytes 읽을 최대 바이트 길이
 * @returns 읽은 데이터 바이트 길이; 에러 혹은 EOF일 시 음수
 */
int TdevChannel_recv(TdevChannel * t, char * buf, int nbytes);

/**
 * 
 * @param buf 쓸 데이터
 * @param nbytes 쓸 데이터의 바이트 길이
 * @returns 쓴 데이터 바이트 길이; 정상적으로 쓰지 못했을 시 음수
 */
int TdevChannel_send(TdevChannel * t, char * buf, int nbytes);

/**
 * 
 * @returns 성공시 0; 실패시 Non-zero
 */
int TdevChannel_finish(TdevChannel * t);