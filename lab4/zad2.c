#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

#define BUFFERSIZE 256
#define READCHUNKS 4
#define THREAD_NUM 3
volatile sig_atomic_t work = 1;
/*
pomysł na to zadanie jest taki, że mamy sobie zmienna ktora bedzie trzymac wolne watki , a same watki beda usypiac 
za pomoca ze jesli nie ma roboty do zrobienia to uspij sie, a jak bedzie to watek main bedzie wybudzal jakiegos jednego watka 
za pomoca cv signal , i wtedy on bedzie robil swoje zadanie , a potem znowu sie usypial 
*/

ssize_t bulk_read(int fd, char *buf, size_t count)
{
    int c;
    size_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;
        if (c == 0)
            return len;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

ssize_t bulk_write(int fd, char *buf, size_t count)
{
    int c;
    size_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return c;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

// struturka do przekazwania do watkow
typedef struct common_data{
    int id;
    int is_work;
    int free_threads;
    pthread_cond_t cv;
    pthread_mutex_t mutex;
}common_data;

typedef struct arg{
    int id;
    common_data* cd; // wskaznik do wspolnych zasobow
}arg;

void handler(int sig){
    work = 0;
}

void* thread_work(void* argument){
    // teraz jestesmy w watku i chcemy zaczac robi swoje zadanie
    arg* args = (arg*) argument;
    common_data* cd = args->cd;
    char buffer[BUFFERSIZE]; // bufor do czytania
    pthread

    free(argument); // zwalniamy pamiec po argumencie bo juz go nie potrzebujemy
}


int main(){
    // najpierw obsluga sygnalu 
    struct sigaction sa;
    memset(&sa,0,sizeof(sa)); // zerujemy tablice
    sa.sa_handler = handler;
    if(sigaction(SIGINT,&sa,NULL)<0)
        ERR("sigaction");
    
    //alokacja strukturkio
    common_data* argument = malloc(sizeof(arg));
    argument->is_work = 0;
    argument->free_threads = THREAD_NUM;
    if(pthread_cond_init(&argument->cv,NULL)!=0) ERR("pthread conditional variable init");
    if(pthread_mutex_init(&argument->mutex,NULL)!=0) ERR("mutex init");

    // teraz trzeba miec jakies watki najlepiej tzymac je w tablicy 
    pthread_t tid[THREAD_NUM];
    for(int i=0;i<THREAD_NUM;i++){
        arg* personalised_id = malloc(sizeof(arg));
        personalised_id->cd = argument;
        personalised_id->id = i;
        if(pthread_create(&tid[i],NULL,thread_work,personalised_id)!=0) ERR("pthread init");
    }
    char input_buffer[BUFFERSIZE];
    // teraz glowny watek bedzie czytal z terminala i budzil wolne watki
    while(work){
        printf("Nacisnij enter aby obudzic watek...\n");
        if(fgets(input_buffer,BUFFERSIZE,stdin)!=NULL){ // jesli dostaniemy jakikolwiek input
            pthread_mutex_lock(&argument->mutex);
            if(argument->free_threads>0){
                argument->free_threads--;
                argument->is_work = 1;
                pthread_mutex_unlock(&argument->mutex);
                pthread_cond_signal(&argument->cv); // budzimy jeden watek
            }else{
                printf("Brak wolnych watkow, czekam...\n");
            }
        }

    }
   
    free(argument);

}