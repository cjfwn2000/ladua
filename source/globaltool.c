#include "globaltool.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <termios.h>

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

char * rtrim(char *str, const char *seps)
{
    int i;
    if (seps == NULL) {
        seps = "\t\n\v\f\r ";
    }
    i = strlen(str) - 1;
    while (i >= 0 && strchr(seps, str[i]) != NULL) {
        str[i] = '\0';
        i--;
    }
    return str;
}

// 참고: https://www.gnu.org/savannah-checkouts/gnu/libc/manual/html_node/getpass.html
int getPasswordInto (char * buf, int nbytes)
{
    struct termios old, new;
    int successful = 0;
    FILE * stream = stdin;

    /* Turn echoing off and fail if we can’t. */
    if (tcgetattr (fileno (stream), &old) != 0)
        return -1;
    new = old;
    new.c_lflag &= ~ECHO;
    if (tcsetattr (fileno (stream), TCSAFLUSH, &new) != 0)
        return -1;

    /* Read the passphrase */
    if( fgets(buf, nbytes, stream) != NULL ) {
        rtrim(buf, NULL);
        successful = 1;
    }

    /* Restore terminal. */
    (void) tcsetattr (fileno (stream), TCSAFLUSH, &old);

    return successful;
}
