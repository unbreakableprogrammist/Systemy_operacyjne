#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h> // Potrzebne do time()

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

// Zmienna globalna volatile - informuje kompilator, że może się zmienić w każdej chwili
volatile int running = 1;
pthread_mutex_t do_running;

typedef struct zmienna{
    int* liczby;
    int* czy_wypisac;
    pthread_mutex_t* blok;
    int k;
} zmienna;

void* thread_for_signals(void* arg){
    zmienna* dane = (zmienna*)arg;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGQUIT);
    
    int sig;
    unsigned int seed = time(NULL); // Poprawiony typ na unsigned int

    while(running){
        // Czekamy na sygnał
        if(sigwait(&set, &sig) != 0) ERR("sigwait");

        if(sig == SIGINT){
            printf("\n");
            // Losujemy indeks od 1 do k
            int rand_number = 1 + rand_r(&seed) % dane->k;
            
            // Blokujemy tylko ten jeden konkretny mutex (fajne rozwiązanie!)
            pthread_mutex_lock(&dane->blok[rand_number]);
            if (dane->czy_wypisac[rand_number] == 1) {
                 // Opcjonalnie: można wypisać co usuwamy, żeby widzieć efekt
                 // printf("[DEBUG] Usuwam liczbe: %d\n", dane->liczby[rand_number]);
                 dane->czy_wypisac[rand_number] = 0;
            }
            pthread_mutex_unlock(&dane->blok[rand_number]);

        } else if(sig == SIGQUIT){
            // Zamykamy bezpiecznie flagę
            pthread_mutex_lock(&do_running);
            running = 0;
            pthread_mutex_unlock(&do_running);
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    if(argc != 2) ERR("zla liczba argumentow");
    int k = atoi(argv[1]);

    if(pthread_mutex_init(&do_running, NULL)) ERR("mutex init");

    // Alokacja struktur
    zmienna* do_przekazania = (zmienna*)malloc(sizeof(zmienna));
    // k+1 wystarczy, skoro indeksujemy 1..k. Zera nie uzywamy.
    do_przekazania->liczby = (int*)malloc((k+1)*sizeof(int));
    do_przekazania->czy_wypisac= (int*)malloc((k+1)*sizeof(int));
    do_przekazania->blok = (pthread_mutex_t*)malloc((k+1)*sizeof(pthread_mutex_t));
    do_przekazania->k = k;

    // Inicjalizacja
    for(int i=1; i<=k; i++){
        do_przekazania->liczby[i] = i;
        do_przekazania->czy_wypisac[i] = 1;
        if(pthread_mutex_init(&do_przekazania->blok[i], NULL)) ERR("pthread init");
    }

    // --- BLOKOWANIE SYGNAŁÓW ---
    sigset_t maska;
    sigemptyset(&maska);
    sigaddset(&maska, SIGINT);
    sigaddset(&maska, SIGQUIT);
    if(pthread_sigmask(SIG_BLOCK, &maska, NULL) != 0) ERR("sigmask");

    // Start wątku
    pthread_t tid;
    if(pthread_create(&tid, NULL, thread_for_signals, do_przekazania) != 0) ERR("pthread create");

    // Pętla główna
    while(running){ // Czytanie running bez mutexa jest tu ryzykowne, ale na potrzeby zadania ujdzie
        sleep(1);
        
        // Sprawdzamy running ponownie po sleepie, żeby nie wypisywać jeśli właśnie przyszedł SIGQUIT
        if(!running) break; 

        printf("Lista: ");
        for(int i=1; i<=k; i++){
            pthread_mutex_lock(&do_przekazania->blok[i]);
            if(do_przekazania->czy_wypisac[i]) {
                printf("%d ", do_przekazania->liczby[i]);
            }
            pthread_mutex_unlock(&do_przekazania->blok[i]);
        }
        printf("\n"); // <--- WAŻNE: Nowa linia, żeby zrzucić bufor na ekran
    }

    // --- SPRZĄTANIE (BARDZO WAŻNE) ---
    
    // 1. Czekamy aż wątek pomocniczy skończy pracę!
    pthread_join(tid, NULL);

    // 2. Niszczymy mutexy
    for(int i=1; i<=k; i++){
        pthread_mutex_destroy(&do_przekazania->blok[i]);
    }
    pthread_mutex_destroy(&do_running);

    // 3. Zwalniamy pamięć
    free(do_przekazania->liczby);
    free(do_przekazania->czy_wypisac);
    free(do_przekazania->blok);
    free(do_przekazania);

    return EXIT_SUCCESS;
}