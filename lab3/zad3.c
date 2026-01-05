#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

/*
plan na to jest taki , nasz glowny watek stworzy sobie pthread_sigmask i zablokuje wszytskie sygnaly , stworzy sobie watek ktory bedzie bral 
te sygnaly za pomaca sigwaita , sigwait zasypia i budzi sie jak dojdzie jakis zablokowany sygnal 
*/

void* thread_for_signals(void* arg){
    
}

int main(int argc, char **argv) {

    if(argc != 2) ERR("zla liczba argumentow");
    int k = atoi(k);
    int* liczby = (int*)malloc((k+7)*sizeof(int));
    int* czy_wypisac= (int*)malloc((k+7)*sizeof(int));
    pthread_mutex_t* blok = (pthread_mutex_t*)malloc((k+7)*sizeof(pthread_mutex_t));
    for(int i=1;i<=k;i++){
        liczby[i]=i;
        czy_wypisac[i]=1;
        if(pthread_mutex_init(&blok[i],NULL)) ERR("pthread init");
    }
    sigset_t maska;
    sigemptyset(&maska);
    sigaddset(&maska,SIGINT);
    sigaddset(&maska,SIGQUIT);
    pthread_sigmask(SIG_BLOCK,&maska,NULL);  // dodajemy SIGINT i SIGQUIT do sygnalow blokowanych 

    pthread_t tid;
    if(pthread_create(&tid,NULL,thread_for_signals,NULL)!=0) ERR("pthread create");
    while(1){

    }

    free(liczby);
    free(czy_wypisac);
    free(blok);
    return EXIT_SUCCESS;
}