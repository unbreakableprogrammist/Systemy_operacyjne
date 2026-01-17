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
#define MAX_INPUT 120 // nie chce nam sie czekac wiecej niz 2 min

/*
pomysl na zadanie - semafor ustawiamy na 5 , za kazdym razem gdy przychodzi do nas nowe zadanie zegarkowe zmieniamy semafor , tak samo jak wraca , to zmieniamy semafor,
uzywamy trywait zeby sprawdzic czy mamy wypisac blad, 
handlowanie SIGINTA , albo mozemy zrobic oddzielny watek , albo mozemy na poczatku zrobic to sigprocmaska z napisanym handlerem 
*/

volatile sig_atomic_t work =  1;

void handler(int sig){
    work = 0;
}

int main(){
    struct sigaction sa;
    memset(&sa,0,sizeof(sa)); // zerujemy tablice 
    sa.sa_handler = handler;
    if(sigaction(SIGINT,&sa,NULL)<0)
        ERR("sigaction");

    return 0;
}