#include <errno.h>  // Error number
#include <fcntl.h>   // File control
#include <termios.h>  // POSIX terminal control 
#include <unistd.h>  // Unic standard function 
#include <stdio.h>
#include <string.h>

// ref: https://stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c
// ref2: https://www.cmrr.umn.edu/~strupp/serial.html

int setInterfaceAttr(int fd)
{
    struct termios options;  // 인터페이스(시리얼포트) 설정을 지닐 객체
    /* tcgetattr를 통해 지금 연결된 인터페이스(fd)로부터 설정을 가져온다.
    그렇게 가져온 "구설정"에 변형을 가해 "신설정"으로 만든 후 fd에 적용해야지,
    options=0인 객체에 변형을 가해 신설정을 만들고 이를 fd에 적용하면
    "Unexplained intermittent failures"를 겪는다.
    */
    if( tcgetattr(fd, &options) != 0 )
    {
        fprintf(stderr, "Error from tcgetattr");
        return -1;
    }

    // Baud rate = 115200
    cfsetospeed(&options, B115200);
    cfsetispeed(&options, B115200);

    options.c_cflag = (options.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    options.c_iflag &= ~IGNBRK;         // disable break processing
    options.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
    options.c_oflag = 0;                // no remapping, no delays
    options.c_cc[VMIN]  = 0;            // read doesn't block
    options.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    options.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    options.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading


    // No parity (8N1)
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

    // 인터페이스에 적용
    if( tcsetattr(fd, TCSANOW, &options) != 0 )
    {
        fprintf(stderr, "Error from tcsetattr");
        return -1;
    }
}

int main() {
    printf("Target device와 통신하는 방법을 알기 위해 이 프로그램을 작성합니다.\n");
    
    // Opening the device interface
    char portname[] = "/dev/ttyACM0";
    int fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) { //Could not open the port.
        fprintf(stderr, "Error opening %s\n", portname);
        return 1;
    }
    
    // Setting the interface attributes
    // speed = 115200 baud rate, parity = no parity
    printf("setInterfaceAttr\n");
    setInterfaceAttr(fd);

    // Reading
    printf("Test: Reading\n");
    char buf[40];
    int n;
    //지금 생각해보건대
    //sizeof(buf)까지 읽어들이기 곤란하다.
    n = read(fd, buf, sizeof(buf) - 1);
    printf("Readed %d bytes : \n", n);
    printf("%s", buf);
    printf("\n");
    
    close(fd);
    return 0;
}


//결과
/*
[case1]
Readed 40 bytes : 
{'x':122, 'y':203}

{'x':456, 'y':283}


[case2]
Readed 26 bytes : 
166}

{'x':578, 'y':217}

5�sU

[case3]
Readed 36 bytes : 
Press 'n' to get to the next turn.

�
*/