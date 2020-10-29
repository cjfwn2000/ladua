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
    n = read(fd, buf, sizeof buf);
    printf("Readed %d bytes : \n", n);
    printf("%s", buf);
    printf("\n");
    
    close(fd);
    return 0;
}

// 결과
/*
[case1]
Readed 19 bytes : 
{'x':568, 'y':147}
�lU

[case2]
Readed 19 bytes : 
{'x':568, 'y':147}
˒U

[의문]
sizeof buf = 40인데도 겨우 18-19바이트를 읽는 것은 왜인가?
알 수 없는 문자가 끝에 붙어있음은 왜인가?
*/
