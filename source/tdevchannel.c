#include "tdevchannel.h"
#include <fcntl.h>  //file control (open)
#include <termios.h>  //Posix terminal control (struct termios)


int TdevChannel_init(TdevChannel * t, const char * filepath, int baudrate)
{
    int fd = open(filepath, O_RDWR);
    if(fd > 0)  //Failed to open
        return -1;

    struct termios t;
}