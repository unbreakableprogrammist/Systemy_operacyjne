#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define UNUSED(x) ((void)(x))

#define DECK_SIZE (4 * 13)
#define HAND_SIZE (7)

void usage(const char *program_name)
{
    fprintf(stderr, "USAGE: %s n\n", program_name);
    exit(EXIT_FAILURE);
}

void shuffle(int *array, size_t n)
{
    if (n > 1)
    {
        size_t i;
        for (i = 0; i < n - 1; i++)
        {
            size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
            int t = array[j];
            array[j] = array[i];
            array[i] = t;
        }
    }
}

void print_deck(const int *deck, int size)
{
    const char *suits[] = {" of Hearts", " of Diamonds", " of Clubs", " of Spades"};
    const char *values[] = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King", "Ace"};

    char buffer[1024];
    int offset = 0;

    if (size < 1 || size > DECK_SIZE)
        return;

    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "[");
    for (int i = 0; i < size; ++i)
    {
        int card = deck[i];
        if (card < 0 || card > DECK_SIZE)
            return;
        int suit = deck[i] % 4;
        int value = deck[i] / 4;

        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s", values[value]);
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s", suits[suit]);
        if (i < size - 1)
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, ", ");
    }
    snprintf(buffer + offset, sizeof(buffer) - offset, "]");

    puts(buffer);
}


typedef struct arg{
    int id;
    int hand[HAND_SIZE];
    int card_to_pass;
    int* recive;
    pthread_barrier_t* barriera;
    int* ready;
    pthread_cond_t* cv;
    pthread_mutex_t* mtx;
    volatile sig_atomic_t* game_over;
    pthread_t main_tid; // Potrzebne do pthread_kill
} arg;

void* thread_work(void* argument){
    arg* args = (arg*) argument;
    int id = args->id;
    int* hand = args->hand;
    volatile sig_atomic_t* game_over = args->game_over;
    
    // Unikalny seed dla każdego wątku
    unsigned int seed = time(NULL) ^ id;

    // --- POCZEKALNIA ---
    pthread_mutex_lock(args->mtx);
    while (!(*args->ready) && !(*game_over)) {
        pthread_cond_wait(args->cv, args->mtx);
    }
    pthread_mutex_unlock(args->mtx);

    // tutaj juz wiemy ze mozemy zaczac grac 
    if(*game_over) return NULL;

    // --- PĘTLA GRY ---
    while(!*game_over) {
        // synchronizujemy zeby wszyscy gracze tu byli 
        pthread_barrier_wait(args->barriera);
         

        // bierzemy pierwszy kolor
        int kolor = hand[0] % 4;
        int is_winner = 1;
        for(int i=0; i<HAND_SIZE; i++){  // idziemy po wszystkich kolorach 
            if(hand[i] % 4 != kolor) is_winner = 0;  // jesli natrafimy na inny to is_winner = false
        }
        // jesli wygralismy 
        if(is_winner) {
            pthread_mutex_lock(args->mtx); // blokujemy mutex, zeby nikt inny nie zdazyl tu wejsc
            if(!(*game_over)){
                *game_over = 1;
                printf("\n>>> GRACZ %d WYGRYWA! <<<\n", id);
                print_deck(hand, HAND_SIZE);
                // wysylamy siginta mainowi
                pthread_kill(args->main_tid, SIGINT);
            }
            // odblokowujemy gameover
            pthread_mutex_unlock(args->mtx);
        }

        // karte do oddania wybieramy losowo
        int idx_to_pass = rand_r(&seed) % HAND_SIZE;
        args->card_to_pass = hand[idx_to_pass];

        // teraz synchronizujemy czekajac az wszystkie watki wystawia karte 
        pthread_barrier_wait(args->barriera);

        //bierzemy sobie karte od innego gracza
        hand[idx_to_pass] = *(args->recive);
        
        // wypisujemy stan gry 
        printf("Gracz %d: ", id); 
        print_deck(hand, HAND_SIZE);
        
        // i czekamy az wszyscy gracze tu dojda
        pthread_barrier_wait(args->barriera);
    }
    
    return NULL;
}

int main(int argc, char *argv[])
{

    if(argc != 2) ERR("usage: ./program N");
    int n = atoi(argv[1]);
    if (n < 2) ERR("min 2 players");

    srand(time(NULL));

    int deck[DECK_SIZE];
    for (int i = 0; i < DECK_SIZE; ++i) deck[i] = i;
    shuffle(deck, DECK_SIZE);

    // wszystkie rzeczy trzymamy na STOSIE maina, ogl moza sie dostac do tego stosu z innych watkow poprzez adres
    arg argumenty[n];  // tu beda nasze argumenty dla kazdego watku 
    pthread_t watki[n]; // tablica na watki 
    pthread_barrier_t bariera; // bariera
    pthread_cond_t cv; // cond variable potrzebna do poczekania na graczy
    pthread_mutex_t mtx; // mtx do cv 
    volatile sig_atomic_t game_over = 0;  // game over 
    int ready = 0;  // czy mozna zaczynac gre

    // Pobieramy ID wątku Main
    pthread_t main_thread_id = pthread_self();

    // inicjalizujemy wszystko
    if(pthread_barrier_init(&bariera, NULL, n) != 0) ERR("barrier");
    if(pthread_cond_init(&cv, NULL) != 0) ERR("cond");
    if(pthread_mutex_init(&mtx, NULL) != 0) ERR("mutex");

    // ustawiamy handlowanie sygnalow 
    sigset_t mask;
    int sig;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGUSR1);
    if(pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0) ERR("sigmask");

    printf("PID: %d. Czekam na %d graczy...\n", getpid(), n);


    for(int i=0; i<n; i++){
        // zasypiamy i czekamy sigwaitem na jakis blokowany sygnal
        if(sigwait(&mask, &sig) != 0) ERR("sigwait");
        
        switch (sig)
        {
        case SIGUSR1:  // jestli dostalismy siguser1
            printf("Dolacza gracz %d\n", i);
            // ustawiamy mu wszystko 
            argumenty[i].id = i;
            argumenty[i].barriera = &bariera;
            argumenty[i].game_over = &game_over;
            argumenty[i].cv = &cv;
            argumenty[i].mtx = &mtx; 
            argumenty[i].ready = &ready;
            argumenty[i].main_tid = main_thread_id;
            
            // sprytny sposob , dajemy sasiadowi po lewej wskaznik na zmienna do ktorej bedziemy wpisywac nasza karte ktora oddajemy
            int left = (i - 1 + n) % n;
            argumenty[i].recive = &argumenty[left].card_to_pass;

            // wpisujemy karty , skoro sa poshuflowane to po prostu pierwsze 7 kazdemu 
            int iter = 0;
            for(int j=i*HAND_SIZE; j<i*HAND_SIZE+HAND_SIZE; j++){
                argumenty[i].hand[iter] = deck[j];
                iter++;
            }
            // tworzymy watki 
            if(pthread_create(&watki[i], NULL, thread_work, &argumenty[i]) != 0) ERR("create");
            break;

        case SIGINT:
            printf("\nPrzerwano (Ctrl+C). Koniec.\n");
            exit(0);
        }
    }
    
    
    printf("Start gry!\n");
    pthread_mutex_lock(&mtx);
    ready = 1;
    pthread_cond_broadcast(&cv); // budzimy watki 
    pthread_mutex_unlock(&mtx);

    // czekamy na SIGINT z klawiatury lub pthread_kill
    if(sigwait(&mask, &sig) != 0) ERR("sigwait");
    
    printf("\nKoniec gry (Odebrano sygnał %d). Sprzątam...\n", sig);
    
    // ustawiamy koniec gry 
    pthread_mutex_lock(&mtx);
    game_over = 1;
    pthread_cond_broadcast(&cv); 
    pthread_mutex_unlock(&mtx);

    // czyszczenie 
    for(int i=0; i<n; i++){
        pthread_join(watki[i], NULL);
    }

    printf("Wszystkie wątki zakończone.\n");
    pthread_barrier_destroy(&bariera);
    pthread_cond_destroy(&cv);
    pthread_mutex_destroy(&mtx);

    exit(EXIT_SUCCESS);
}