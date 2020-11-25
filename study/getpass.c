#include <stdio.h>
#include <termios.h>

// 참고: https://www.gnu.org/savannah-checkouts/gnu/libc/manual/html_node/getpass.html
int my_getpass (char * buf, int nbytes)
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
    if( fgets(buf, nbytes, stream) != NULL )
        successful = 1;

    /* Restore terminal. */
    (void) tcsetattr (fileno (stream), TCSAFLUSH, &old);

    return successful;
}

int main() {

    char password[64] = "";

    printf("Your password?: ");
    if( ! my_getpass(password, sizeof password) )
        printf("ERROR\n");
    printf("password = %s\n", password);

    return 0;
}