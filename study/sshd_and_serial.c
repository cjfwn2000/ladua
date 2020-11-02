/**
 * 2020.10.30.
 * Mobidigm, CNU SmartDataLab
 * 시리얼 서버 (혹은 기기출력 송신 모듈)
 */

#include <libssh/libssh.h>
#include <libssh/server.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <fcntl.h>
#include <getopt.h>
#include <poll.h>
//#include <stropts.h>
#include <unistd.h>
#include <termios.h>  // POSIX terminal control 
#include <netinet/in.h>

#define KEYS_FOLDER "/etc/ssh/"

/**
 * SSH접속에 필요한 계정 인증
 */
static int auth_password(const char *user, const char *password){
    if(strcmp(user,"mobidigm"))
        return 0;
    if(strcmp(password,"dj2020"))
        return 0;
    return 1; // authenticated
}

/**
 * 시리얼포트로 연결된 Target Device의 File Descriptor(Integer)를 줄 뿐 아니라 
 * 그것의 여러 설정(예: Baud rate)까지 해줌
 */
static int serialsetup(const char *name, int speed) {
    int fd = open(name, O_RDWR);
    if(fd < 0)
        return -1;

    struct termios t;
    if( tcgetattr(fd, &t) != 0 )
    {
        fprintf(stderr, "Error from tcgetattr");
        return -1;
    }

    // Baud rate = 115200
    cfsetospeed(&t, B115200);
    cfsetispeed(&t, B115200);
    //
    t.c_cflag = (t.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    t.c_iflag &= ~IGNBRK;         // disable break processing
    t.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
    t.c_oflag = 0;                // no remapping, no delays
    t.c_cc[VMIN]  = 0;            // read doesn't block
    t.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    t.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    t.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
    // No parity (8N1)
    t.c_cflag &= ~PARENB;
    t.c_cflag &= ~CSTOPB;
    t.c_cflag &= ~CSIZE;
    t.c_cflag |= CS8;
    cfmakeraw(&t);
    
    if( tcsetattr(fd, TCSANOW, &t) != 0 )
    {
        fprintf(stderr, "Error from tcsetattr");
        return -1;
    }
    return fd;
}

/**
 * SSH출력을 미리 여기 콘솔에 보여줄 용도
 */
static void dump16(char prefix, const char *buffer, int length) {
  putchar(prefix);
  for (int i = 0; i < length; i++)
    printf(" %02x", (uint8_t) buffer[i]);
  for (int i = length; i < 16; i++)
    printf("   ");
  printf(" %c ", prefix);
  for (int i = 0; i < length; i++) {
    uint8_t c = buffer[i];
    putchar(c >= 32 && c <= 127 ? c : '.');
  }
  putchar('\n');
}
static void dump(char prefix, const char *buffer, int length) {
  for (; length >= 0; length -= 16, buffer += 16)
    dump16(prefix, buffer, length > 16 ? 16 : length);
}

////


int main(int argc, char **argv){
    ssh_session session;
    ssh_bind sshbind;
    ssh_message message;
    ssh_channel chan=0;
    char buf[2048];
    int auth=0;
    int sftp=0;
    int i;
    int r;

    unsigned int myport = 10001;

    sshbind=ssh_bind_new();
    session=ssh_new();

    printf("Entering: ssh_bind_options_set\n");
    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_DSAKEY, KEYS_FOLDER "ssh_host_dsa_key");
    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_RSAKEY, KEYS_FOLDER "ssh_host_rsa_key");
    // SSH_BIND_OPTIONS_BINDPORT: Set the port to bind (unsigned int *).
    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDPORT, &myport);

    (void) argc;
    (void) argv;

    printf("ssh_bind_listen\n");
    if(ssh_bind_listen(sshbind)<0){
        printf("Error listening to socket: %s\n",ssh_get_error(sshbind));
        return 1;
    }
    printf("ssh_bind_accept\n");
    r=ssh_bind_accept(sshbind,session);
    if(r==SSH_ERROR){
      printf("error accepting a connection : %s\n",ssh_get_error(sshbind));
      return 1;
    }
    printf("ssh_handle_key_exchange\n");
    if (ssh_handle_key_exchange(session)) {
        printf("ssh_handle_key_exchange: %s\n", ssh_get_error(session));
        return 1;
    }
    printf("Entering the loop.\n");
    do {
        message=ssh_message_get(session);
        if(!message)
            break;
        switch(ssh_message_type(message)){
            case SSH_REQUEST_AUTH:
                switch(ssh_message_subtype(message)){
                    case SSH_AUTH_METHOD_PASSWORD:
                        printf("User %s wants to auth with pass %s\n",
                               ssh_message_auth_user(message),
                               ssh_message_auth_password(message));
                        if(auth_password(ssh_message_auth_user(message),
                           ssh_message_auth_password(message))){
                               auth=1;
                               ssh_message_auth_reply_success(message,0);
                               break;
                           }
                        // not authenticated, send default message
                    case SSH_AUTH_METHOD_NONE:
                    default:
                        ssh_message_auth_set_methods(message,SSH_AUTH_METHOD_PASSWORD);
                        ssh_message_reply_default(message);
                        break;
                }
                break;
            default:
                ssh_message_reply_default(message);
        }
        ssh_message_free(message);
    } while (!auth);
    if(!auth){
        printf("auth error: %s\n",ssh_get_error(session));
        ssh_disconnect(session);
        return 1;
    }
    do {
        message=ssh_message_get(session);
        if(message){
            switch(ssh_message_type(message)){
                case SSH_REQUEST_CHANNEL_OPEN:
                    if(ssh_message_subtype(message)==SSH_CHANNEL_SESSION){
                        chan=ssh_message_channel_request_open_reply_accept(message);
                        break;
                    }
                default:
                ssh_message_reply_default(message);
            }
            ssh_message_free(message);
        }
    } while(message && !chan);
    if(!chan){
        printf("error : %s\n",ssh_get_error(session));
        ssh_finalize();
        return 1;
    }
    do {
        message=ssh_message_get(session);
        if(message && ssh_message_type(message)==SSH_REQUEST_CHANNEL &&
           ssh_message_subtype(message)==SSH_CHANNEL_REQUEST_SHELL){
//            if(!strcmp(ssh_message_channel_request_subsystem(message),"sftp")){
                sftp=1;
                ssh_message_channel_request_reply_success(message);
                break;
 //           }
           }
        if(!sftp){
            ssh_message_reply_default(message);
        }
        ssh_message_free(message);
    } while (message && !sftp);
    if(!sftp){
        printf("error : %s\n",ssh_get_error(session));
        return 1;
    }
    printf("it works !\n");

    // The Serial Port Interface
    int speed = 115200;
    char devfilename[] = "/dev/ttyACM0";
    int fdDev = serialsetup(devfilename, speed);

    //doMyBridge(fdDev);
    char buffer[256];
    int n = 0;
    struct pollfd fds[] = {
        {fdDev, POLLIN, 0}
    };
    //fds[0]: for fdDev (Serial port)
    //fds[1]: for fdSshch (SSH Channel) //TODO 여기서 아직 구현 안함
    while(1) {
        // Mission: poll을 대체하라... 훗날 정식 버전에서...
        poll(fds, 1, -1);
        if(fds[0].revents & POLLIN) {
            // relay
            n = read(fdDev, buffer, sizeof buffer);
            if (n>0) {
                // write to the channel
                if( ssh_channel_write(chan, buffer, n) < 0) {
                    printf("Error on ssh channel. Has the client broken?\n");
                    return 1;
                }
                dump('<', buffer, n);
            } else {
                printf("Error, serial port broken?\n");
                break;
            }
        }
    }
    ////

    if(close(fdDev))
    {
        printf("Error on close()\n");
    }
    ssh_disconnect(session);
    ssh_bind_free(sshbind);

    ssh_finalize();
    return 0;
}