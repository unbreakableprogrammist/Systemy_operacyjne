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

volatile sig_atomic_t running = 1; // Flaga do kontrolowania dzialania watkow
// Struktura ze wspólnymi zasobami (tor, mutexy)
typedef struct arg{
    int* tor;
    pthread_mutex_t* mutexy;
    int n; // dlugosc toru
    int k; // ilosc psow
    int first_place;
    int second_place;
    int third_place;
    pthread_mutex_t first_place_mutex;
    pthread_mutex_t second_place_mutex;
    pthread_mutex_t third_place_mutex;
    int ile_na_mecie;
    pthread_mutex_t ile_na_mecie_mutex;
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

void przerwanie(void* arg){

}

void* thread_work(void* argument) {
    dog* doggo = (dog*)argument;
    int id = doggo->id;
    arg* args = doggo->arguments;
    int n = args->n;
    int* tor = args->tor;
    pthread_mutex_t* mutexy = args->mutexy;
    unsigned int seed = time(NULL) ^ id; // unikalny seed dla kazdego psa
    int position = -1; // poczatkowa pozycja psa (poza torem)
    while(running){
        pthread_cleanup_push(przerwanie, NULL);
        int random_time = 200+ rand_r(&seed) % 1321; // 200 - 1520 ms
        struct timespec ts= {0,random_time * 1000000}; // zamiana na milisekundy
        nanosleep(&ts, NULL);
        int skok = 1 + rand_r(&seed) % 5; // skok 1-5
        if(position == -1){
            // ustawiamy psa na starcie 
            position = 0;
            // blokujemy pozycje startowa
            pthread_mutex_lock(&mutexy[position]);
            tor[position] +=1; // zajmujemy miejsce na torze
            pthread_mutex_unlock(&mutexy[position]);
            continue;
        }
        if(position + skok < n-1){ // to oznacza ze nie wejdziemy poza tor ani na mete
            // zwalniamy obecna pozycje
            pthread_mutex_lock(&mutexy[position]);
            tor[position] -=1; // zwalniamy miejsce na torze
            pthread_mutex_unlock(&mutexy[position]);
            position += skok;
            //blokujemy nowa pozycje
            pthread_mutex_lock(&mutexy[position]);
            tor[position] +=1;
            if(tor[position] > 1){
                // jest tu inny pies
                printf(" waf waf waf , pies: %d, na pozycji: %d\n", id, position);
            }
            pthread_mutex_unlock(&mutexy[position]);
            continue;
        }
        if(position+skok == n-1){  // wchodzimy na mete 
            pthread_mutex_lock(&mutexy[position]);
            tor[position] -=1; // zwalniamy miejsce na torze
            pthread_mutex_unlock(&mutexy[position]);
            position = n-1; // ustawiamy na mete
            // Zajmujemy mete
            pthread_mutex_lock(&mutexy[position]);
            tor[position] +=1;
            if(tor[position] > 1){
                // jest tu inny pies
                printf(" waf waf waf , pies: %d, na mecie \n", id);
            }
            pthread_mutex_unlock(&mutexy[position]);
            pthread_mutex_lock(&args->ile_na_mecie_mutex);
            args->ile_na_mecie +=1;
            pthread_mutex_unlock(&args->ile_na_mecie_mutex);
            // Obsługa miejsc na podium
            pthread_mutex_lock(&args->first_place_mutex);
            if(args->first_place == -1){
                args->first_place = id;
                printf("Pies %d zajął 1 miejsce!\n", id);
                pthread_mutex_unlock(&args->first_place_mutex);
                break; // pies konczy wyscig
            }
            pthread_mutex_unlock(&args->first_place_mutex);
            pthread_mutex_lock(&args->second_place_mutex);
            if(args->second_place == -1){
                args->second_place = id;
                printf("Pies %d zajął 2 miejsce!\n", id);
                pthread_mutex_unlock(&args->second_place_mutex);
                break; // pies konczy wyscig
            }
            pthread_mutex_unlock(&args->second_place_mutex);
            pthread_mutex_lock(&args->third_place_mutex);
            if(args->third_place == -1){
                args->third_place = id;
                printf("Pies %d zajął 3 miejsce!\n", id);
                pthread_mutex_unlock(&args->third_place_mutex);
                break; // pies konczy wyscig
            }
            pthread_mutex_unlock(&args->third_place_mutex);
            break;
        }
        if(position + skok > n-1){
            // odbicie
            int odbicie = (position + skok) - (n-1);
            // zwalniamy obecna pozycje
            pthread_mutex_lock(&mutexy[position]);
            tor[position] -=1; // zwalniamy miejsce na torze
            pthread_mutex_unlock(&mutexy[position]);
            position = (n-1) - odbicie;
            //blokujemy nowa pozycje
            pthread_mutex_lock(&mutexy[position]);
            tor[position] +=1;
            if(tor[position] > 1){
                // jest tu inny pies
                printf(" waf waf waf , pies: %d, na pozycji: %d\n", id, position);
            }
            pthread_mutex_unlock(&mutexy[position]);
            continue;
        }
        pthread_cleanup_pop(0); 
    }
    return NULL;
}

void* signal_handler_threat(void* arg){
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    int sig;
    while(1){
        if(sigwait(&set, &sig) != 0) ERR("sigwait");
        if(sig == SIGINT){
            printf("Odebrano SIGINT, zatrzymywanie wyscigu...\n");
            running = 0; // ustawiamy flage na 0, co spowoduje zakonczenie pracy watkow psow
            signal_args* s_args = (signal_args*) arg;
            for(int i = 0; i < s_args->k; i++){
                if(pthread_cancel(s_args->watki[i]) != 0) ERR("pthread_cancel");
            }
            break;
        }
    }
}

int main(int argc, char **argv) {
    if(argc != 3) ERR("zla ilosc argumentow");
    int n = atoi(argv[1]); // dlugosc toru
    int k = atoi(argv[2]); // ilosc psow 
    if(n <= 0 || k <= 0) ERR("argumenty musza byc dodatnie");

    // blokowanie sygnalu SIGINT w glownym watku
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    if(pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) ERR("pthread_sigmask");



    // 1. Inicjalizacja wspolnych zasobow (arg)
    arg* arguments = malloc(sizeof(arg));
    if (!arguments) ERR("malloc arguments");
    arguments->first_place = -1;
    arguments->second_place = -1;
    arguments->third_place = -1;
    if(pthread_mutex_init(&arguments->first_place_mutex, NULL) != 0) ERR("mutex init");
    if(pthread_mutex_init(&arguments->second_place_mutex, NULL) != 0) ERR("mutex init");
    if(pthread_mutex_init(&arguments->third_place_mutex, NULL) != 0) ERR("mutex init");

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
    pthread_t signal_thread;
    if(pthread_create(&signal_thread, NULL, signal_handler_threat, (void*)sig_args) != 0) ERR("pthread create");
    
    // TODO: Pętla glowna wypisujaca stan toru 
    while(running){
        struct timespec ts = {0,100000000}; // 1000 ms
        nanosleep(&ts, NULL);
        printf("Stan toru: ");
        for(int i = 0; i < n; i++){
            pthread_mutex_lock(&mutexy[i]);
            printf("[%d]: %d ", i, tor[i]);
            pthread_mutex_unlock(&mutexy[i]);
        }
        printf("\n");
        pthread_mutex_lock(&arguments->ile_na_mecie_mutex);
        printf("Ile psow na mecie: %d\n", arguments->ile_na_mecie);
        pthread_mutex_unlock(&arguments->ile_na_mecie_mutex);
    }

    // TODO: cleanup...
    free(dogs);
    free(tor);
    free(mutexy);
    free(arguments);
    free(watki);
    free(sig_args);
    pthread_mutex_destroy(&arguments->first_place_mutex);
    pthread_mutex_destroy(&arguments->second_place_mutex);
    pthread_mutex_destroy(&arguments->third_place_mutex);
    pthread_mutex_destroy(&arguments->ile_na_mecie_mutex);
    for (int i = 0; i < n; i++)
    {
        pthread_mutex_destroy(&mutexy[i]);
    }
    
    return EXIT_SUCCESS;
}