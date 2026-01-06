#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>


#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

/* 
pomysl na to zadanie :
tworzymy n watkow , kazdy tam sobie wylosuje to M , L to bedie liczba globalna chroniona mutexem , petla dziala while running , gdzie running bedzie 
zmienial sie jak przyjdzie sigint ( obsluga sygnalu poprzez dodatkowy watek do czekania na sygnal , on zmienia running na false i laczymy watki 
dodatkowo bedziemy sprawdzac zmienna globalna czy wszyskie watki juz sobie sprawdzily 
*/

volatile sig_atomic_t running = 1;
pthread_mutex_t do_running;
int L;
pthread_mutex_t do_L;
int czy_wszytskie;
pthread_mutex_t do_czy_wszystkie;

void* signal_handler(void* arg){
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set,SIGINT);
    int sig;
    while(1){
        if(sigwait(&set, &sig) != 0) ERR("sigwait");
        if(sig == SIGINT){
            //zmieniamy running na false 
            pthread_mutex_lock(&do_running);
            running = 0;
            pthread_mutex_unlock(&do_running);
            break;
        }
    }
    return NULL;
}

void* thread_work(void* arg){

}

int main(int argc, char **argv) {
    if (argc != 2) ERR("Wrong number of arguments");
    int n = atoi(argv[1]);
    
    // bedziemy ignorowac SIGINT poza watkiem do tego odpowiedzialnym 
    sigset_t maska;
    sigemptyset(&maska);
    sigaddset(&maska,SIGINT);
    if(pthread_sigmask(SIG_BLOCK, &maska, NULL) != 0) ERR("sigmask");
    // iniciujemy mutexy 
    if(pthread_mutex_init(&do_running, NULL)) ERR("pthread init");
    if(pthread_mutex_init(&do_L, NULL)) ERR("pthread init");
    if(pthread_mutex_init(&do_czy_wszystkie, NULL)) ERR("pthread init");

    pthread_t threads[n+1]; // jeden dodatkowy watek do obslugi sygnalow
    
    //teraz tworzymy watek od handlowania sygnalow 
    if(pthread_create(&threads[n],NULL,signal_handler,NULL)!=0) ERR("pthread create");
    
    //teraz tworzymy n watkow roboczych
    for(int i=0;i<n;i++){
        if(pthread_create(&threads[i],NULL,thread_work,NULL)!=0) ERR("pthread create");
    }
    srand(time(NULL));
    // teraz glowna czesc 
    while(running){
        
    }

    // destroy mutex
    return EXIT_SUCCESS;
}