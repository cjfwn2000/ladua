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

static void dump16(char prefix, const char *buffer, int length) {
    putchar(prefix);
    for (int i = 0; i < length; i++)
        printf(" %02x", (unsigned char) buffer[i]);
    for (int i = length; i < 16; i++)
        printf("   ");
    printf(" %c ", prefix);
    for (int i = 0; i < length; i++) {
        unsigned char c = buffer[i];
        putchar(c >= 32 && c <= 127 ? c : '.');
    }
    putchar('\n');
}

void logDump(char prefix, const char *buffer, int length) {
    for (; length >= 0; length -= 16, buffer += 16)
        dump16(prefix, buffer, length > 16 ? 16 : length);
}
