#include <math.h>
#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

typedef struct thread_estimation {
    pthread_t tid;
    int id;
    int ile;
    double estimation;
} thread_estimation;
/* * DLACZEGO rand_r() A NIE rand()?
 * * Standardowy rand() korzysta z ukrytego, globalnego stanu (seed). Aby zapewnic 
 * bezpieczenstwo watkowe, biblioteka C chroni ten stan wewnetrznym MUTEXEM.
 * * WASKIE GARDLO (BOTTLENECK):
 * Gdy wiele watkow jednoczesnie wola rand() w petli, dochodzi do zjawiska 
 * "Lock Contention" (rywalizacja o blokade). Watki zamiast pracowac rownolegle, 
 * musza czekac w kolejce na odblokowanie mutexa przez inny watek.
 * * Powoduje to SERIALIZACJE wykonywania kodu - w danej chwili losuje tylko jeden watek,
 * co sprawia, ze program dziala z predkoscia jednego rdzenia (lub wolniej przez narzut).
 * rand_r() uzywa lokalnego seeda na stosie, eliminujac blokady calkowicie.
 tzn jest 
 blokada < - tu jest klolejka 
 wywolanie rand 
 koniec blokazdy 
 my uzywamy rand_r ktory przyjmuje lokalny seed time(NULL) obecny czas linuxowy od startu linuxa
 */

void* estimate(void* arg){
    thread_estimation* argument = (thread_estimation*) arg; // rzutujemy sobie argument 
    unsigned int seed = time(NULL) ^ argument->id;
    double licznik = 0.0;
    for(int i=0;i<argument->ile;i++){
        double x = (double)(rand_r(&seed)) / (double)RAND_MAX; // rand max - maksymalna liczba do wylosowania przez rand_r 
        double y = (double)(rand_r(&seed)) / (double)RAND_MAX;
        if(x*x + y*y <= 1) licznik++;
    }
    argument->estimation = 4.0 * licznik / (double)argument->ile;
    return NULL;
}

int main(int argc, char ** argv) {
    if(argc != 3){
        ERR("zla liczba argumentow ");
    }
    int k = atoi(argv[1]);
    int n = atoi(argv[2]);

    /*
    plan na zadanie jest taki , tworzymy sobie tablice struct ( pid , double estymacja pi ) ktora bedzie na heapie , tam watki beda sobie wpisywaly 
    swoje obliczenia , a na koncu policzymy srednia arytmetyczna z wszyskich  
    */
    thread_estimation* tab = (thread_estimation*)malloc(k*sizeof(thread_estimation));
    for(int i=0;i<k;i++){
        tab[i].id = i;
        tab[i].ile = n;
        int err = pthread_create(&(tab[i].tid),NULL,estimate,&tab[i]);
        if(err != 0) ERR("blad przy tworzeniu watku ");
        
    }
    double suma = 0.0;
    for(int i=0;i<k;i++){
        int err = pthread_join(tab[i].tid,NULL);
        if(err!=0) ERR("join");
        suma += tab[i].estimation;
    }
    suma = suma / (double)k;
    free(tab);
    printf("%f\n",suma);
    return 0;
}