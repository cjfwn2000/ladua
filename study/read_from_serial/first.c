#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>

// ref: https://stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c

int main() {
    printf("Target device와 통신하는 방법을 알기 위해 이 프로그램을 작성합니다.\n");
    
    char portname[] = "/dev/ttyACM0";
    int fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
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
