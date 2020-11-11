#include "tdevchannel.h"
#include "globaltool.h"
#include <fcntl.h>  //file control (e.g. O_RDWR)
#include <unistd.h>  //Posix IO interfaces (e.g. read)
#include <termios.h>  //Posix terminal control (e.g. struct termios)

struct _TdevChannel {
    int fd;  //file descriptor to tdev
    //char * filepath
    //char * filepath
    //int baudrate;
};

int TdevChannel_init(TdevChannel * t, const char * filepath, unsigned int baudrate)
{
    int fd = open(filepath, O_RDWR | O_SYNC | O_NOCTTY);
    if(fd < 0)  //Failed to open
        return -1;

    // 설정을 쓰기 위해 기기에서 복사해온다.
    struct termios opts;
    if(tcgetattr(fd, &opts) != 0)
        return -2;

    // Baud rate
    cfsetispeed(&opts, baudrate);
    cfsetospeed(&opts, baudrate);
    // 기타 이것저것
    // 참고: https://stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c
    opts.c_cflag = (opts.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    opts.c_iflag &= ~IGNBRK;         // disable break processing
    opts.c_lflag = 0;                // no signaling chars, no echo,
                                     // no canonical processing
    opts.c_oflag = 0;                // no remapping, no delays
    opts.c_cc[VMIN]  = 0;            // read doesn't block
    opts.c_cc[VTIME] = 1;            // 0.1 seconds read timeout

    opts.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    opts.c_cflag |= (CLOCAL | CREAD); // ignore modem controls,
                                      // enable reading
    // No parity (8N1)
    opts.c_cflag &= ~PARENB;
    opts.c_cflag &= ~CSTOPB;
    opts.c_cflag &= ~CSIZE;
    opts.c_cflag |= CS8;

    // 설정 opts를 다시 기기에 반영해준다.
    if(tcsetattr(fd, TCSANOW, &opts) != 0)
        return -3;

    // 정상
    t->fd = fd;
    return 0;
}

int TdevChannel_recv(TdevChannel * t, char * buf, int nbytes)
{
    int readed;
    static char zerobuf[1] = "";

    readed = read(t->fd, buf, nbytes);
    // 혹시 연결이 끊어졌는지 확인
    if(readed==0 && write(t->fd, zerobuf, 0) < 0) 
        return -1;
    return readed;
}

int TdevChannel_send(TdevChannel * t, char * buf, int nbytes)
{
    int writed;
    
    writed = write(t->fd, buf, nbytes);
    return writed;
}

int TdevChannel_finish(TdevChannel * t)
{
    return close(t->fd);
}
