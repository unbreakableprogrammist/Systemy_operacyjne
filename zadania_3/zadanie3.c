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

// Struktura ze wspólnymi zasobami (tor, mutexy)
typedef struct arg{
    int* tor;
    pthread_mutex_t* mutexy;
    int n; // dlugosc toru
    int k; // ilosc psow
} arg;

// Struktura dla konkretnego psa (ma swoje ID i wskaznik do wspolnych zasobow)
typedef struct dog{
    int id;
    arg* arguments;
} dog;
// Struktura dla argumentow watku sygnalowego
typedef struct signal_args{
    pthread_t* watki;
    int k;
} signal_args;

void* thread_work(void* ptr) {

    return NULL;
}

int main(int argc, char **argv) {
    if(argc != 3) ERR("zla ilosc argumentow");
    int n = atoi(argv[1]); // dlugosc toru
    int k = atoi(argv[2]); // ilosc psow 
    if(n <= 0 || k <= 0) ERR("argumenty musza byc dodatnie");

    // 1. Inicjalizacja wspolnych zasobow (arg)
    arg* arguments = malloc(sizeof(arg));
    if (!arguments) ERR("malloc arguments");

    // calloc zeruje pamiec od razu
    int* tor = calloc(n, sizeof(int));
    if (!tor) ERR("malloc tor");
    arguments->tor = tor;

    pthread_mutex_t* mutexy = malloc(n * sizeof(pthread_mutex_t));
    if (!mutexy) ERR("malloc mutexy");
    for(int i = 0; i < n; i++) {
        if(pthread_mutex_init(&mutexy[i], NULL) != 0) ERR("mutex init");
    }
    arguments->mutexy = mutexy;
    arguments->n = n;
    arguments->k = k;

    // 2. Przygotowanie tablicy struktur dla psow
    // Dzieki temu kazdy watek dostanie swoje wlasne "pudelko" z unikalnym ID
    dog* dogs = malloc(k * sizeof(dog));
    if (!dogs) ERR("malloc dogs");

    pthread_t* watki = malloc(k * sizeof(pthread_t)); // k wątków psów 
    if (!watki) ERR("malloc watki");

    // 3. Tworzenie watkow
    for(int i = 0; i < k; i++) {
        dogs[i].id = i;       // Nadajemy unikalne ID psu
        dogs[i].arguments = arguments; // Przypisujemy wskaznik do wspolnych danych
        
        // Przekazujemy adres KONKRETNEJ struktury dla danego psa (&dogs[i])
        if(pthread_create(&watki[i], NULL, thread_work, (void*)&dogs[i]) != 0) ERR("pthread create");
    }

    signal_args* sig_args = malloc(sizeof(signal_args));
    sig_args->watki = watki;
    sig_args->k = k;    

    // TODO: watek od sygnalu SIGINT...
    
    // TODO: Pętla glowna wypisujaca stan toru 
    

    // TODO: cleanup...

    return EXIT_SUCCESS;
}