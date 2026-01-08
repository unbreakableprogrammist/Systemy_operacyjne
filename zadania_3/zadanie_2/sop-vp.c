#define _POSIX_C_SOURCE 200809L // 1. Niezbędne dla sigwait/nanosleep
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include "video-player.h" 

// TODO in stage 3
typedef struct circular_buffer
{
    video_frame* buffer[BUFFER_SIZE];
    int head;  // miejsce wstawiania nowego elementu
    int tail;  // miejsce wyjmowania elementu
    int n;     // ilosc frameow w buforze
    pthread_mutex_t mtx; // jeden mutex do ochrony calej struktury
} circular_buffer;


typedef struct do_transforma{
    circular_buffer* bufor_a;
    circular_buffer* bufor_b;
} do_transforma;

volatile  sig_atomic_t working = 1;

// Funkcja tworząca bufor
circular_buffer* circular_buffer_create() {
    circular_buffer* cb = (circular_buffer*) malloc(sizeof(circular_buffer));
    if(cb == NULL) ERR("malloc");
    
    cb->head = 0;
    cb->tail = 0;
    cb->n = 0;
    
    // Inicjalizujemy tylko ten jeden mutex
    if(pthread_mutex_init(&cb->mtx, NULL)) ERR("mutex init");
    
    return cb;
}   

void circular_buffer_push(circular_buffer* cb, video_frame* frame) {
    int czy_udalo_sie = 0;
    struct timespec ts = {0, 5000000}; // 5ms

    // 2. Dodano '&& working'. Inaczej watek zawiesi sie na exit, jesli bufor pelny.
    while(!czy_udalo_sie && working) { 
        // 1. Blokujemy dostęp do całego bufora na początku
        pthread_mutex_lock(&cb->mtx);
        
        if(cb->n >= BUFFER_SIZE) {
            // Jeśli pełny, musimy ODBLOKOWAĆ mutex przed pójściem spać
            pthread_mutex_unlock(&cb->mtx);
            nanosleep(&ts, NULL); 
        } else {
            // Wpisujemy frame-a
            cb->buffer[cb->head] = frame;
            // Zmieniamy heada
            cb->head = (cb->head + 1) % BUFFER_SIZE;
            // Zwiększamy licznik n
            cb->n += 1; 
            // Dopiero gdy wszystko gotowe, zwalniamy blokadę
            pthread_mutex_unlock(&cb->mtx);
            czy_udalo_sie = 1; 
        }
    }
}

video_frame* circular_buffer_pop(circular_buffer* cb) {
    struct timespec ts = {0, 5000000}; // 5ms
     // 3. Zmieniono while(1) na while(working). Inaczej deadlock na pustym buforze przy exit.
     while(working) { 
        // Blokujemy dostęp do całego bufora na początku
        pthread_mutex_lock(&cb->mtx);
        if(cb->n == 0) { // w buforze nic nie ma 
            pthread_mutex_unlock(&cb->mtx);
            nanosleep(&ts, NULL); 
        } else { // to oznacza ze w buforze cos jest
            video_frame* zwroc = cb->buffer[cb->tail];
            cb->n-=1;
            cb->tail=(cb->tail+1) % BUFFER_SIZE;
            pthread_mutex_unlock(&cb->mtx);
            return zwroc;
        }
    }
    return NULL; // 4. Musimy zwrocic NULL gdy konczymy prace
}

void circular_buffer_destroy(circular_buffer* buffer) {
    if(!buffer) return;
    
    // 5. Najpierw czyscimy pamiec, potem mutex (bezpieczniej)
    int indx = buffer->tail;
    for(int i=0;i<buffer->n;i++) {
        if(buffer->buffer[indx]) free(buffer->buffer[indx]); // check na NULL
        indx = (indx+1) % BUFFER_SIZE;
    }
    
    if(pthread_mutex_destroy(&buffer->mtx)) ERR("mutex destroy");
    free(buffer);
}

// 6. Funkcja cleanup musi przyjmowac void* (inaczej warning/blad kompilatora)
void canceletion_point(void* arg){
    UNUSED(arg);
    printf("Kocham Legię \n");
}

void* decode_thread(void* arg){
    pthread_cleanup_push(canceletion_point,NULL);
    circular_buffer* wejscie = (circular_buffer*) arg; // zdejmujemy strukturke 
    while(working){
        video_frame* klatka = decode_frame();
        circular_buffer_push(wejscie,klatka);
    }
    pthread_cleanup_pop(0);
    return NULL; // 7. Watek musi cos zwracac
}

void* transform_thread(void* arg){
    pthread_cleanup_push(canceletion_point,NULL);
    do_transforma* wejscie = (do_transforma*)arg;
    circular_buffer* buffor1 = wejscie->bufor_a;
    circular_buffer* buffor2 = wejscie->bufor_b;
    while(working){
        video_frame* klatka = circular_buffer_pop(buffor1);
        // 8. Musimy sprawdzic NULL, bo pop zwraca NULL przy zamykaniu -> inaczej Segfault
        if(klatka) { 
            transform_frame(klatka);
            circular_buffer_push(buffor2,klatka);
        }
    }
    pthread_cleanup_pop(0);
    return NULL;
}

void* display_thread(void* arg){
    pthread_cleanup_push(canceletion_point,NULL);
    circular_buffer* wejscie = (circular_buffer*) arg;
    while(working){
        struct timespec start;
        struct timespec koniec;
        if(clock_gettime(CLOCK_REALTIME,&start)) ERR("clock realtime");
        
        video_frame* klatka = circular_buffer_pop(wejscie);
        // 9. Check na NULL
        if(klatka) {
            display_frame(klatka); 
        }

        if(clock_gettime(CLOCK_REALTIME,&koniec)) ERR("clock realtime");
        long elapsed = (koniec.tv_sec - start.tv_sec) * 1000000000L + (koniec.tv_nsec - start.tv_nsec); 
        if(elapsed < 33330000){ // jesli nie minelo 33,33 ms 
            struct timespec gotosleep = {0,33330000-elapsed};
            nanosleep(&gotosleep,NULL);
        }
    }
    pthread_cleanup_pop(0);
    return NULL;
}

int main(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    // blokujemy siginta 
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set,SIGINT);
    if(pthread_sigmask(SIG_BLOCK,&set,NULL)) ERR("sigmask"); // check bledu

    circular_buffer* buffor_a = circular_buffer_create();  // ten bufor sie kontaktuje miedzy decode <-> transform
    circular_buffer* buffor_b = circular_buffer_create();  // ten bufor kontaktuje sie miedzy transform <-> display
    
    do_transforma* dt = (do_transforma*)malloc(sizeof(do_transforma));
    if(!dt) ERR("malloc");

    pthread_t decode;
    pthread_t transform;
    pthread_t display;
    dt->bufor_a = buffor_a;
    dt->bufor_b = buffor_b;

    if(pthread_create(&decode,NULL,decode_thread,buffor_a)) ERR("thread create");
    if(pthread_create(&transform,NULL,transform_thread,dt)) ERR("thread create");
    // 10. CRITICAL FIX: Display musi czytac z buffor_b, a nie a!
    if(pthread_create(&display,NULL,display_thread,buffor_b)) ERR("thread create");
    
    // teraz te watki sobue jakos dzialaja wiec main tylko czeka na sygnal
    int sig;
    if(sigwait(&set,&sig)) ERR("sigwait");
    
    
    if(sig == SIGINT){
        working = 0;
        if(pthread_cancel(decode)) ERR("pthread cancel");
        if(pthread_cancel(transform)) ERR("pthread cancel");
        if(pthread_cancel(display)) ERR("pthread cancel");

        if(pthread_join(decode,NULL)) ERR("pthread join");
        if(pthread_join(transform,NULL)) ERR("pthread join");
        if(pthread_join(display,NULL)) ERR("pthread join");
        
        // Sprzatanie pamieci
        circular_buffer_destroy(buffor_a);
        circular_buffer_destroy(buffor_b);
        free(dt);
    }
    exit(EXIT_SUCCESS);
}