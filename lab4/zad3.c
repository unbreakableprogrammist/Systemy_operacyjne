#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

#define PLAYER_COUNT 4
#define ROUNDS 10
/*
bardzo proste zadanko - stwórz 4 wątki reprezentujące graczy w grze.
gracz = watek, bariera pomaga w synchronizacji rund gry.
*/
typedef struct arg{ 
    int id;
    pthread_barrier_t* barrier; // jak nizej 
    int* scores; // tu bedziemy trzymac wskaznik na stos watku main
} arg;

void* thread_work(void* argument){
    arg* args = (arg*) argument;
    // teraz gramy 
    for(int i=0;i<ROUNDS;i++){
        unsigned int seed = time(NULL)^args->id;
        args->scores[args->id] = 1+ rand_r(&seed)&6;
        printf("Gracz %d wyrzucil %d w rundzie %d\n", args->id+1, args->scores[args->id], i+1);
        // czekamy na koiniec rundy 
        int wybraniec = pthread_barrier_wait(args->barrier); // tu sie zatrzymujemy az wszystkie watki nie skoncza pracy 
        if(wybraniec == PTHREAD_BARRIER_SERIAL_THREAD){ // jesli jestem wybrancem to wypisuje wyniki 
            int max = -1;
            int id_wygranego = -1;
            for(int i=0;i<PLAYER_COUNT;i++){
                if(args->scores[i]>max){
                    max = args->scores[i];
                    id_wygranego = i;
                }
            }
            printf("W rundzie %d wygrywa gracz %d z wynikiem %d\n", i+1, id_wygranego+1, max);
        }
        pthread_barrier_wait(args->barrier); // czekamy az wybraniec wypisze wyniki
    }
}

int main(){
    pthread_t watki[PLAYER_COUNT];
    pthread_barrier_t bariera; // ta zmienna bedzie na stosie tego watku 
    arg argument[PLAYER_COUNT];
    int scores[ROUNDS];
    pthread_barrier_init(&bariera, NULL, PLAYER_COUNT);
    for(int i=0; i<PLAYER_COUNT; i++){
        argument[i].barrier = &bariera;
        argument[i].id = i;
        argument[i].scores = scores;
        if(pthread_create(&watki[i],NULL,thread_work,&argument[i])!=0) ERR("pthread create");
    }

    for(int i=0;i<PLAYER_COUNT;i++){
        if(pthread_join(watki[i],NULL)!=0) ERR("pthread join");
    }
    pthread_barrier_destroy(&bariera);
    return EXIT_SUCCESS;
}