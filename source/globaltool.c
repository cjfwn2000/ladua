#include "globaltool.h"
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
