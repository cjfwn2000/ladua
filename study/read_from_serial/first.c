#include <errno.h>  // Error number
#include <fcntl.h>   // File control
#include <termios.h>  // POSIX terminal control 
#include <unistd.h>  // Unic standard function 
#include <stdio.h>
#include <string.h>

// ref: https://stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c
// ref2: https://www.cmrr.umn.edu/~strupp/serial.html

int main() {
    printf("Target device와 통신하는 방법을 알기 위해 이 프로그램을 작성합니다.\n");
    
    char portname[] = "/dev/ttyACM0";
    int fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) { //Could not open the port.
        fprintf(stderr, "Error opening %s\n", portname);
        return 1;
    }
    
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

// FAIL: 시리얼 텍스트를 잘 못 읽는다.
// why? Baud rate 등등 초기 설정이 더 필요한 듯 하다.