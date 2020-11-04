/**
 * Mobidigm
 * CNU Smartdatalab Lee cheolju
 */

typedef struct _TdevChannel {
    int fd;  //file descriptor to tdev
    //char * filepath
    //int baudrate;
} TdevChannel;

void TdevChannel_init(TdevChannel * t, const char * filepath, int baudrate)
{
    int fd = open(name, O_RDWR)
}