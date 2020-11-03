/**
 * 모든 모듈에 공통하게 필요할 도구를 정리해놓은 곳
 */

#include <stdio.h>
#include <stdarg.h>

void logInfo(const char *fmt, ... )
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}
