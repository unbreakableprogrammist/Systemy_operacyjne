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

    while(!czy_udalo_sie) { 
        // 1. Blokujemy dostęp do całego bufora na początku
        pthread_mutex_lock(&cb->mtx);
        
        if(cb->n >= BUFFER_SIZE) {
            // Jeśli pełny, musimy ODBLOKOWAĆ mutex przed pójściem spać,
            // inaczej nikt nie będzie mógł wyjąć elementu (konsument też potrzebuje tego mutexa!)
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
     while(1) { 
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

}
void circular_buffer_destroy(circular_buffer* buffer) {
    if(pthread_mutex_destroy(&buffer->mtx)) ERR("mutex destroy");
    free(buffer);
}

int main(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    
    // // one-thread video_frame flow example. You can remove those lines
    // video_frame* f = decode_frame();
    // transform_frame(f);
    // display_frame(f);

    exit(EXIT_SUCCESS);
}
