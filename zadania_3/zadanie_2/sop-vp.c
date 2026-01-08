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
void circular_buffer_push(circular_buffer* buffer, video_frame* frame) {
    int czy_udalo_sie=0;
    while(!czy_udalo_sie){ // dopoki sie nie udaje
        
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
