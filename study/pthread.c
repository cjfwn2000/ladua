#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <semaphore.h>

void logln(const char *fmt, ... )
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

sem_t mutex;

struct SomeStruct {
    int a;
    int b;
    char buf[25];
};

void * thread1Routine(void * payload)
{
    struct SomeStruct * p = (struct SomeStruct *) payload;
    int i;

    for(i=0; i<7; i++) {
        sem_wait(&mutex);
        logln("th1: ))Entered((");
        usleep(1400*1000);
        logln("th1: (i=%d) p->b = %d", i, p->b);
        p->b += 3;
        logln("th1: ))Exiting((");
        sem_post(&mutex);
    }
}
void * thread2Routine(void * payload)
{
    struct SomeStruct * p = (struct SomeStruct *) payload;
    int i;

    for(i=0; i<7; i++) {
        sem_wait(&mutex);
        logln("th2: ))Entered((");
        usleep(1200*1000);
        logln("th2: (i=%d) p->b = %d", i, p->b);
        p->b += 4;
        logln("th2: ))Exiting((");
        sem_post(&mutex);
    }
}
void * thread3Routine(void * payload)
{
    struct SomeStruct * p = (struct SomeStruct *) payload;
    int i;

    for(i=0; i<7; i++) {
        sem_wait(&mutex);
        logln("th3: ))Entered((");
        usleep(1000*1000);
        logln("th3: (i=%d) p->b = %d", i, p->b);
        p->b += 5;
        logln("th3: ))Exiting((");
        sem_post(&mutex);
    }
}

int main()
{
    sem_init(&mutex, 0, 1);
    pthread_t thread1ID, thread2ID, thread3ID;
    struct SomeStruct ss = {123, 65, "Banya sutra"};

    logln("Creating threads.");
    pthread_create(&thread1ID, NULL, thread1Routine, &ss);
    pthread_create(&thread2ID, NULL, thread2Routine, &ss);
    pthread_create(&thread3ID, NULL, thread3Routine, &ss);

    logln("Wait for threads to join before exiting.");
    pthread_join(thread1ID, NULL);
    pthread_join(thread2ID, NULL);
    pthread_join(thread3ID, NULL);

    return 0;
}