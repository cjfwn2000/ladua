/**
 * 모든 모듈에 공통하게 필요할 도구를 정리해놓은 곳
 */

//로깅(혹은 디버깅메시지)에 쓰임
void logInfo(const char *fmt, ... );
void logDump(char prefix, const char *buffer, int length);

//문자열 후미 공백 제거; 텔넷 계정정보를 제대로 받는데 쓰임
//출처: http://www.martinbroadhurst.com/trim-a-string-in-c.html (2016.09.08.)
char * rtrim(char *str, const char *seps);
