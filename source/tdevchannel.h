/**
 * Mobidigm
 * CNU Smartdatalab Lee cheolju
 */

typedef struct _TdevChannel {
    int fd;  //file descriptor to tdev
    //char * filepath
    //int baudrate;
} TdevChannel;

/**
 * 정상 실행시 t가 목표기기에 연결되어 다른 문제 없는 한 추후 여러 실행이 가능하게 됩니다.
 * @returns 정상시 0, 에러시 Non-zero
 */
int TdevChannel_init(TdevChannel * t, const char * filepath, int baudrate);