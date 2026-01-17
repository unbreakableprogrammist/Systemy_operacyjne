#define _GNU_SOURCE
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define MAX_INPUT 120 

/*
pomysl na zadanie - semafor ustawiamy na 5 , za kazdym razem gdy przychodzi do nas nowe zadanie zegarkowe zmieniamy semafor , tak samo jak wraca , to zmieniamy semafor,
uzywamy trywait zeby sprawdzic czy mamy wypisac blad, 
handlowanie SIGINTA , albo mozemy zrobic oddzielny watek , albo mozemy na poczatku zrobic to sigprocmaska z napisanym handlerem 
*/

volatile sig_atomic_t work =  1;

void handler(int sig){
    work = 0;
}

typedef struct arguments{
    int time;
    sem_t *semaphore;
}arguments;

void* thread_work(void* arg){
    arguments* argument = (arguments*)arg;
    int time = argument->time;
    sleep(time);
    fprintf(stderr, "Koniec alarmu , mineło %d sekund\n",time);
    if(sem_post(argument->semaphore)<0) ERR("semafor post");
    free(arg);
    return NULL;
}

int main(){
    struct sigaction sa;
    memset(&sa,0,sizeof(sa)); // zerujemy tablice 
    sa.sa_handler = handler;
    if(sigaction(SIGINT,&sa,NULL)<0)
        ERR("sigaction");

    char line [MAX_INPUT];
    sem_t semafor;
    if(sem_init(&semafor,0,5)<0)
        ERR("sem_init");
    pthread_attr_t atrybuty;
    if(pthread_attr_init(&atrybuty)!=0) ERR("pthread attr init ");
    if (pthread_attr_setdetachstate(&atrybuty, PTHREAD_CREATE_DETACHED) != 0) ERR("attr_setdetachstate");
    while(work){
        printf("Please set the alarm :  ");
        if(fgets(&line,MAX_INPUT,stdin) == NULL){
            if(errno == EINTR) continue;
            ERR("fgets");
        }
        int time = atoi(line);
        if(time <= 0) {
            fputs("Incorrect time specified", stderr);
            continue;
        }
        // trywait jak moze to waituje
        if(sem_trywait(&semafor)<0){  // trywait - nie blokuje procesu, tylko sprawdza czy mozna zmniejszyc semafor, >0 jesli mozna , -1 jesli nie mozna/blad
            if(errno == EAGAIN){  // skoro wartosc semafora <0 to albo blad , albo nie mozna zmniejszyc semafora bo jest 0 , EAGAIN - nie mozna zmniejszyc semafora bo jest 0
                fputs("Too many alarms set\n",stderr);
                continue;
            } // w przeciwnym wypadku to jest blad
            ERR("sem_trywait");
        }

        // teraz inicjiujemy watek
        arguments* arg = malloc(sizeof(arguments));
        
        arg->time = time;
        arg->semaphore = &semafor;
        pthread_t tid;
        if(pthread_create(&tid,&atrybuty,thread_work,arg)!=0) ERR("pthread create");
        
      

    }
    return 0;
}