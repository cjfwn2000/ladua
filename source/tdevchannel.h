/**
 * @file tdevchannel.h
 * Mobidigm
 * CNU Smartdatalab Lee cheolju
 */

/**
 * 시리얼포트로 연결된 목표기기(Target Device)와 데이터를 주고받도록 하는 통로 역할을 합니다. 
 * 기능 활용 전에 반드시 초기화(TdevChannel_init)되어야 합니다. 
 */
typedef struct _TdevChannel {
    int fd;  //file descriptor to tdev
    //char * filepath
    //char * filepath
    //int baudrate;
} TdevChannel;

/*
 * 유저가 baudrate를 제공할 수 있게 하기 위합니다.
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
 * TdevChannel 객체를 초기화합니다.
 * t가 목표기기에 연결되어 다른 문제 없는 한 추후 여러 실행이 가능하게 됩니다.
 * @param filepath 목표기기 파일 주소
 * @param baudrate 목표기기의 전송 속도 (Baud rate); e.g. B115200
 * @returns 성공시 0, 에러시 Non-zero
 */
int TdevChannel_init(TdevChannel * t, const char * filepath, unsigned int baudrate);

/**
 * 목표기기로부터 데이터를 읽습니다.
 * Non-blocking이기 때문에 당장 들어온 것이 없다면 0이 return될 수 있습니다.
 * @param buf 읽을 데이터 들어가는 공간
 * @param nbytes 읽을 최대 바이트 길이
 * @returns 읽은 데이터 바이트 길이; 에러 혹은 EOF일 시 음수
 */
int TdevChannel_recv(TdevChannel * t, char * buf, int nbytes);

/**
 * 목표기기에게 데이터를 전송합니다.
 * @param buf 쓸 데이터
 * @param nbytes 쓸 데이터의 바이트 길이
 * @returns 쓴 데이터 바이트 길이; 정상적으로 쓰지 못했을 시 음수
 */
int TdevChannel_send(TdevChannel * t, const char * buf, int nbytes);

/**
 * 목표기기와의 데이터 송수신을 더 이상 하지 않을 때 호출합니다.
 * @returns 성공시 0; 실패시 음수
 */
int TdevChannel_finalize(TdevChannel * t);