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

// Zmienna globalna do kontrolowania działania wątków
volatile sig_atomic_t running = 1;

typedef struct arg{
    int* tor; // tor ( tablica ktora jest mallocowana w main )
    pthread_mutex_t* mutexy;  // tablica mutexow do zabezpieczenia pozycji na torze
    int n; // dlugosc toru
    int k; // ilosc psow
    // miejsca na podium i mutexy do ich zabezpieczenia
    int first_place;   
    int second_place;   
    int third_place;  
    pthread_mutex_t first_place_mutex;  
    pthread_mutex_t second_place_mutex;
    pthread_mutex_t third_place_mutex;
    // licznik ile na mecie psow 
    int ile_na_mecie;
    pthread_mutex_t ile_na_mecie_mutex;
} arg;

// struktura przekazywana do wątku psa( ma unikalne id i wskaźnik do arg ( ktory jest ogolny dla wszystkich psów ) )
typedef struct dog{
    int id;
    arg* arguments;
} dog;

// struktura przekazywana do wątku sygnałowego dostaje wskaźnik do tablicy wątków psów i ich ilość
typedef struct signal_args{
    pthread_t* watki;
    int k;
} signal_args;


void przerwanie(void* arg){
    printf("Kocham Legię!!! \n");
}

void* thread_work(void* argument) {
    dog* doggo = (dog*)argument;
    int id = doggo->id;
    arg* args = doggo->arguments;
    int n = args->n;
    int* tor = args->tor;
    pthread_mutex_t* mutexy = args->mutexy;
    
    unsigned int seed = time(NULL) ^ id;
    
    int position = 0; 
    
    // Wejście na start
    int random_time = 200 + rand_r(&seed) % 1321;
    struct timespec ts= {0, random_time * 1000000};
    nanosleep(&ts, NULL);
    
    pthread_mutex_lock(&mutexy[0]);
    tor[0] += 1;
    pthread_mutex_unlock(&mutexy[0]);

    pthread_cleanup_push(przerwanie, NULL);
    
    while(running){
        random_time = 200 + rand_r(&seed) % 1321;
        ts.tv_nsec = random_time * 1000000;
        nanosleep(&ts, NULL);
        
        int skok = 1 + rand_r(&seed) % 5;
        int next_pos = position + skok;
        
        // Logika odbicia
        if (next_pos >= n) {
            next_pos = n - 1 - (next_pos - (n - 1));
        } 
        
        // jesli pies zmienil pozycje 
        if (next_pos != position) {
            pthread_mutex_lock(&mutexy[next_pos]);
            tor[next_pos] += 1;
            bool tlok = (tor[next_pos] > 1);
            pthread_mutex_unlock(&mutexy[next_pos]);
            
            if (tlok) {
                printf("waf waf waf , pies: %d, na pozycji: %d\n", id, next_pos);
            }
            
            pthread_mutex_lock(&mutexy[position]);
            tor[position] -= 1;
            pthread_mutex_unlock(&mutexy[position]);
            
            position = next_pos;
        }

        // Sprawdzenie czy pies dotarł do mety
        if (position == n - 1) {
            // Zwiększ licznik psów na mecie
            pthread_mutex_lock(&args->ile_na_mecie_mutex);
            args->ile_na_mecie += 1;
            pthread_mutex_unlock(&args->ile_na_mecie_mutex);

            bool czy_na_podium = false; // Flaga sterująca

            // Sprawdź 1. miejsce
            pthread_mutex_lock(&args->first_place_mutex);
            if(args->first_place == -1){
                args->first_place = id;
                printf("Pies %d zajął 1 miejsce!\n", id);
                czy_na_podium = true;
            }
            pthread_mutex_unlock(&args->first_place_mutex);

            // Sprawdź 2. miejsce (tylko jeśli nie ma podium)
            if (!czy_na_podium) {
                pthread_mutex_lock(&args->second_place_mutex);
                if(args->second_place == -1){
                    args->second_place = id;
                    printf("Pies %d zajął 2 miejsce!\n", id);
                    czy_na_podium = true;
                }
                pthread_mutex_unlock(&args->second_place_mutex);
            }

            // Sprawdź 3. miejsce (tylko jeśli nadal nie ma podium)
            if (!czy_na_podium) {
                pthread_mutex_lock(&args->third_place_mutex);
                if(args->third_place == -1){
                    args->third_place = id;
                    printf("Pies %d zajął 3 miejsce!\n", id);
                    czy_na_podium = true;
                }
                pthread_mutex_unlock(&args->third_place_mutex);
            }

            // Jeśli po sprawdzeniu wszystkich miejsc nadal nie ma podium
            if (!czy_na_podium) {
                printf("Pies %d na mecie (poza podium)\n", id);
            }

            // Wspólny kod sprzątający (dawniej pod etykietą finish:)
            pthread_mutex_lock(&mutexy[position]);
            tor[position] -= 1;
            pthread_mutex_unlock(&mutexy[position]);
            
            break; 
        }
    }

    pthread_cleanup_pop(0);
    return NULL;
}

void* signal_handler_threat(void* arg){
    // Ten wątek tylko czeka na sygnał
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    int sig;
    
    // sigwait jest punktem anulowania, więc cancel na nim zadziała
    if(sigwait(&set, &sig) != 0) ERR("sigwait");
    
    if(sig == SIGINT){
        printf("\nOdebrano SIGINT, zatrzymywanie wyscigu...\n");
        running = 0;
        // Nie musimy tu anulować wątków pętlą for,
        // main to zrobi w sekcji sprzątania po wyjściu z while(running)
    }
    return NULL;
}

int main(int argc, char **argv) {
    if(argc != 3) ERR("zla ilosc argumentow");
    int n = atoi(argv[1]); 
    int k = atoi(argv[2]); 
    if(n <= 0 || k <= 0) ERR("argumenty musza byc dodatnie");

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    if(pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) ERR("pthread_sigmask");

    arg* arguments = malloc(sizeof(arg));
    if (!arguments) ERR("malloc arguments");
    arguments->first_place = -1;
    arguments->second_place = -1;
    arguments->third_place = -1;
    arguments->ile_na_mecie = 0;
    
    if(pthread_mutex_init(&arguments->ile_na_mecie_mutex, NULL) != 0) ERR("mutex init");    
    if(pthread_mutex_init(&arguments->first_place_mutex, NULL) != 0) ERR("mutex init");
    if(pthread_mutex_init(&arguments->second_place_mutex, NULL) != 0) ERR("mutex init");
    if(pthread_mutex_init(&arguments->third_place_mutex, NULL) != 0) ERR("mutex init");

    int* tor = calloc(n, sizeof(int));
    if (!tor) ERR("malloc tor");
    arguments->tor = tor;

    pthread_mutex_t* mutexy = malloc(n * sizeof(pthread_mutex_t));
    if (!mutexy) ERR("malloc mutexy");
    for(int i = 0; i < n; i++) pthread_mutex_init(&mutexy[i], NULL);
    
    arguments->mutexy = mutexy;
    arguments->n = n;
    arguments->k = k;

    dog* dogs = malloc(k * sizeof(dog));
    pthread_t* watki = malloc(k * sizeof(pthread_t));

    for(int i = 0; i < k; i++) {
        dogs[i].id = i;
        dogs[i].arguments = arguments;
        if(pthread_create(&watki[i], NULL, thread_work, (void*)&dogs[i]) != 0) ERR("pthread create");
    }

    // Wątek sygnałowy nie potrzebuje 'sig_args', wystarczy mu NULL, bo tylko zmienia running
    pthread_t signal_thread;
    if(pthread_create(&signal_thread, NULL, signal_handler_threat, NULL) != 0) ERR("pthread create");
    
    // --- PĘTLA GŁÓWNA ---
    while(running){
        struct timespec ts = {0, 100000000}; // 100ms (zmieniłem z 1s dla płynności, ale jak wolisz 1s to {1,0})
        nanosleep(&ts, NULL);
        
        printf("Stan toru: ");
        for(int i = 0; i < n; i++){
            pthread_mutex_lock(&mutexy[i]);
            printf("%d ", tor[i]); // Uproszczony widok
            pthread_mutex_unlock(&mutexy[i]);
        }
        printf("\n");
        
        pthread_mutex_lock(&arguments->ile_na_mecie_mutex);
        if(arguments->ile_na_mecie == k){
            printf("Wszystkie psy dotarly do mety!\n");
            running = 0; // Wyjście z pętli
        }
        pthread_mutex_unlock(&arguments->ile_na_mecie_mutex);
    }
    
    // --- SPRZĄTANIE (POPRAWIONA KOLEJNOŚĆ!) ---

    printf("Konczenie watkow...\n");

    // 1. Jeśli wyścig skończył się normalnie, wątek sygnałowy wciąż wisi na sigwait.
    // Musimy go anulować. Jeśli przerwano SIGINTem, to on już się skończył, ale cancel nie zaszkodzi.
    pthread_cancel(signal_thread);
    pthread_join(signal_thread, NULL);

    // 2. Anulujemy psy (dla pewności, jeśli przerwano w połowie) i czekamy na nie
    for(int i = 0; i < k; i++) {
        pthread_cancel(watki[i]); 
        pthread_join(watki[i], NULL);
    }

    printf("Wyniki:\n");
    if(arguments->first_place != -1) printf("1 miejsce: Pies %d\n", arguments->first_place);
    if(arguments->second_place != -1) printf("2 miejsce: Pies %d\n", arguments->second_place);
    if(arguments->third_place != -1) printf("3 miejsce: Pies %d\n", arguments->third_place);

    // 3. Dopiero TERAZ niszczymy mutexy
    pthread_mutex_destroy(&arguments->first_place_mutex);
    pthread_mutex_destroy(&arguments->second_place_mutex);
    pthread_mutex_destroy(&arguments->third_place_mutex);
    pthread_mutex_destroy(&arguments->ile_na_mecie_mutex);
    for (int i = 0; i < n; i++) pthread_mutex_destroy(&mutexy[i]);

    // 4. Na samym końcu zwalniamy pamięć
    free(dogs);
    free(tor);
    free(mutexy);
    free(arguments);
    free(watki);
    
    
    return EXIT_SUCCESS;
}