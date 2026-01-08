#include "video-player.h"

// TODO in stage 3
typedef struct circular_buffer
{
    video_frame* buffer[BUFFER_SIZE];
    pthread_mutex_t buffer_mtx;
    int head;  // miesce wstawiania nowego elementu
    pthread_mutex_t head_mtx;
    int tail;  // miejsce wyjmowania elementu
    pthread_mutex_t tail_mtx;
    int n;  // ilosc frameow w buforze
    pthread_mutex_t n_mtx;
} circular_buffer;

// funkcja ktora tworzy bufor cykliczny i wypelnia go wartosciami domyslnymi 
circular_buffer* circular_buffer_create() {
    circular_buffer* cb = (circular_buffer*) malloc(sizeof(circular_buffer));
    if(cb==NULL) ERR("malloc");
    cb->head = 0;
    cb->tail=0;
    cb->n=0;
    if(pthread_mutex_init(&cb->buffer_mtx,NULL)) ERR("mutex innit");
    if(pthread_mutex_init(&cb->head_mtx,NULL)) ERR("mutex innit");
    if(pthread_mutex_init(&cb->tail_mtx,NULL)) ERR("mutex innit");
    if(pthread_mutex_init(&cb->n_mtx,NULL)) ERR("mutex innit");
    return cb;
}   

// ta funckja musi puszowac wartosci , wazne jest to ze musi to robic TYLKO gdy n< BUFFERSIZE i musi robic to cyklicznie (%BUFFERSIZE)
void circular_buffer_push(circular_buffer* cb, video_frame* frame) {
    int czy_udalo_sie=0;
    struct timespec ts = {0,5000000}; // czas czekania to 5milisekund 
    while(!czy_udalo_sie){ // dopoki sie nie udaje
        pthread_mutex_lock(&cb->n_mtx);
        int n = cb->n;
        pthread_mutex_unlock(&cb->n_mtx);
        if(n >= BUFFER_SIZE) nanosleep(&ts,NULL); // jesli bufor jest pelen to busy waiting 5 ms
        else{
            pthread_mutex_lock(&cb->buffer_mtx);
            cb->buffer[cb->head] = frame;
            pthread_mutex_unlock(&cb->buffer_mtx);
            pthread_mutex_lock(&cb->head_mtx);
            cb->head=(cb->head+1)%BUFFER_SIZE; // zmieniamy heada i zakrecamy
            pthread_mutex_unlock(cb->head_mtx);
            n+=1;
            pthread_mutex_lock(&cb->n_mtx);
            cb->n = n;
            pthread_mutex_unlock(&cb->n_mtx);
        
        }
    }
}
// video_frame* circular_buffer_pop(circular_buffer* buffer) {}
// void circular_buffer_destroy(circular_buffer* buffer) {}

int main(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);


    // one-thread video_frame flow example. You can remove those lines
    video_frame* f = decode_frame();
    transform_frame(f);
    display_frame(f);

    exit(EXIT_SUCCESS);
}
