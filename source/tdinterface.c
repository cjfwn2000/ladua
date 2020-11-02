#include "tdinterface.h"

#include <errno.h>  // Error number
#include <fcntl.h>   // File control
#include <termios.h>  // POSIX terminal control 
#include <unistd.h>  // Unic standard function 
#include <stdio.h>
#include <string.h>

// -- Private functions ----------------



// -- Public functions -----------------

int TDI_open(TDInterface * tdi, const char * devfile)
{
    int fd;  //file descriptor to the serial port

    // Opening
    fd = open(devfile, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {  //could not open the serial port
        fprintf(stderr, "Error opening the serial port.\n");
        return -1;
    }
    // Setting the interface attributes (e.g. baud rate)
    if(_setInterfaceAttr(fd)) {
        fprintf(stderr, "Failed setInterfaceAttr.\n");
    }
}

int TDI_read(TDInterface * tdi, char * bigbuf);
