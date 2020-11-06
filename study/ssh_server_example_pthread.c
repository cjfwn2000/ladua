#include <libssh/libssh.h>
#include <libssh/server.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#ifndef KEYS_FOLDER
#ifdef _WIN32
#define KEYS_FOLDER
#else
#define KEYS_FOLDER "/etc/ssh/"
#endif
#endif

typedef struct _Packpack {
    pthread_t threadID;
    ssh_session sshSess;
    char bufbuf[256];
} Packpack;


static int auth_password(const char *user, const char *password){
    if(strcmp(user,"aris"))
        return 0;
    if(strcmp(password,"lala"))
        return 0;
    return 1; // authenticated
}

static void * eachSessionRoutine(void * pack)
{
    /* ref
    https://api.libssh.org/master/libssh_tutor_threads.html
    */
    Packpack * p = (Packpack *) pack;
    ssh_session sess = p->sshSess;
    ssh_message sshMsg;

    printf("Hello new thread id = %u\n", p->threadID);
    // assert sess == (부모쓰레드가 만든 ssh_session)

    // 1. Key exchange
    if( ssh_handle_key_exchange(sess) ) {
        printf("Error ssh_handle_key_exchange: %s\n", ssh_get_error(sess));
        return NULL;
    }

    // 2. Authentication
    // TODO Rather try to functionize this step for readability. @_@
    int auth = 0;
    do {
        sshMsg = ssh_message_get(sess);
        if(!sshMsg)
            break;
        switch(ssh_message_type(sshMsg)) {
            case SSH_REQUEST_AUTH:
            switch(ssh_message_subtype(sshMsg)) {
                case SSH_AUTH_METHOD_PASSWORD:
                printf("User %s wants to auth with pass %s\n",
                    ssh_message_auth_user(sshMsg),
                    ssh_message_auth_password(sshMsg));
                if( auth_password(
                        ssh_message_auth_user(sshMsg),
                        ssh_message_auth_password(sshMsg)
                    )
                ) {
                    auth = 1;
                    ssh_message_auth_reply_success(sshMsg, 0);
                    break;  //Go down if failed.
                }

                // Else If auth_password failed, it is not authenticated.
                case SSH_AUTH_METHOD_NONE:
                default:
                ssh_message_auth_set_methods(sshMsg, SSH_AUTH_METHOD_PASSWORD);
                ssh_message_reply_default(sshMsg);
                break;
            }
            break;

            default:
            ssh_message_reply_default(sshMsg);
        }
    } while(!auth);
    if(!auth) {
        printf("Auth error: %s\n", ssh_get_error(sess));
        return NULL;
    }
}

int main(int argc, char **argv){
    ssh_bind sshbind;
    int rc;
    unsigned int myport = 10001;

    sshbind=ssh_bind_new();

    printf("Entering: ssh_bind_options_set\n");
    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_DSAKEY, KEYS_FOLDER "ssh_host_dsa_key");
    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_RSAKEY, KEYS_FOLDER "ssh_host_rsa_key");
    // SSH_BIND_OPTIONS_BINDPORT: Set the port to bind (unsigned int *).
    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDPORT, &myport);


    printf("ssh_bind_listen\n");
    if(ssh_bind_listen(sshbind)<0){
        printf("Error listening to socket: %s\n",ssh_get_error(sshbind));
        return 1;
    }

    Packpack packs[32];  //a pack for each session
    unsigned int i_pack;
    i_pack = 0;

    ssh_session sshSessNewb;

    while(1)
    { // In our project: MAIN THREAD = ACCEPTOR THREAD
        // Max pack limitation; i_pack should be in [0..31]
        while(i_pack >= sizeof(packs));
        
        // One new session
        sshSessNewb = ssh_new();
        printf("sshSessNewb = %p\n", sshSessNewb); //TODO
        printf("ssh_bind_accept\n");
        ssh_bind_accept(sshbind, sshSessNewb); //blocking
        if(rc == SSH_ERROR) {
            printf("error accepting a connection : %s\n",ssh_get_error(sshbind));
            break;
        }
        printf("New client accepted\n");
        
        // accepted then -> One new thread
        // with a new pack
        packs[i_pack].sshSess = sshSessNewb;
        snprintf(packs[i_pack].bufbuf, sizeof(packs[i_pack].bufbuf), "Tutmonda SSH!");
        if( pthread_create(
            &(packs[i_pack].threadID), NULL,
            eachSessionRoutine, (void*) &(packs[i_pack])
            ) != 0 ) {
            printf("Error creating thread\n");
            break;
        }
        i_pack++;
        printf("Then new thread created\n");
    }
    // TODO join the threads to safely exit. but how?
    return 0;
}