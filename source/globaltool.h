/**
 * 모든 모듈에 공통하게 필요할 도구를 정리해놓은 곳
 */

/** 
 * 로깅(혹은 디버깅메시지)에 쓰입니다. 
 */
void logInfo(const char *fmt, ... );
/**
 * 버퍼 내 데이터를 바이트열로 표현해줍니다.
 * 여기서는 데이터의 송수신이 제대로 되는지 감시하기 위해 쓰입니다.
 */
void logDump(char prefix, const char *buffer, int length);

/**
 * 문자열(C-string) str의 후미 공백을 제거합니다. 
 * 여기서는 계정정보 입력 후미에 들어가는 공백(e.g.엔터)을 제거하는데 쓰입니다.
 * @return str를 가리킵니다.
 */
char * rtrim(char *str, const char *seps);

/** 
 * 사용자 패스워드 입력에 쓰입니다.
 * 일반적 scanf, gets와 달리 입력을 화면에서 가려줍니다. 
 * @returns 정상 처리시 1
 */
int getPasswordInto (char * buf, int nbytes);