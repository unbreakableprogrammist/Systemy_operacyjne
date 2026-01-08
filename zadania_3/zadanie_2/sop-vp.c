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
    // zeby nie robic double free idziemy od taila i zwalniamy tyle ile jest elementow
    int indx = buffer->tail;
    for(int i=0;i<buffer->n;i++) {
        free(buffer->buffer[indx]);
        indx = (indx+1) % BUFFER_SIZE;
    }
    free(buffer);
}

void canceletion_point(){
    printf("Kocham Legię \n");
}

void* decode_thread(void* arg){
    pthread_cleanup_push(canceletion_point,NULL);
    // wywolujemy funkcje decode_frame i klatka po klatce wkladamy do bufora za pomoca circular_buffer_push
    circular_buffer* wejscie = (circular_buffer*) arg; // zdejmujemy strukturke 
    while(working){
        video_frame* klatka = decode_frame();
        circular_buffer_push(wejscie,klatka);
    }
    pthread_cleanup_pop(0);
}

void* transform_thread(void* arg){
    pthread_cleanup_push(canceletion_point,NULL);
    do_transforma* wejscie = (do_transforma*)arg;
    circular_buffer* buffor1 = wejscie->bufor_a;
    circular_buffer* buffor2 = wejscie->bufor_b;
    while(working){
        video_frame* klatka = circular_buffer_pop(buffor1);
        transform_frame(klatka);
        circular_buffer_push(buffor2,klatka);
    }
    pthread_cleanup_pop(0);
}

// teraz najtrudniejsza funkcja ( chodzi o to ze musimy sobie zapamietywac ostatni czas , sprawdzac terazniejszy i odejmowac sprawdzajac czy trafimy w 33ms)
void* display_thread(void* arg){
    pthread_cleanup_push(canceletion_point,NULL);
    circular_buffer* wejscie = (circular_buffer*) arg;
    struct timespec last_frame = {0, 0};
    struct timespec small_break ={0,500000}; // 0.5ms
    while(working){
        video_frame* klatka = circular_buffer_pop(wejscie);
        display_frame(klatka);
    }
    pthread_cleanup_pop(0);
}

int main(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    // blokujemy siginta 
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set,SIGINT);
    pthread_sigmask(SIG_BLOCK,&set,NULL);

    circular_buffer* buffor_a = circular_buffer_create();  // ten bufor sie kontaktuje miedzy decode <-> transform
    circular_buffer* buffor_b = circular_buffer_create();  // ten bufor kontaktuje sie miedzy transform <-> display
    
    do_transforma* dt = (do_transforma*)malloc(sizeof(do_transforma));
    
    pthread_t decode;
    pthread_t transform;
    pthread_t display;
    dt->bufor_a = buffor_a;
    dt->bufor_b = buffor_b;

    if(pthread_create(&decode,NULL,decode_thread,buffor_a)) ERR("thread create");
    if(pthread_create(&transform,NULL,transform_thread,dt)) ERR("thread create");
    if(pthread_create(&decode,NULL,decode_thread,buffor_a)) ERR("thread create");
    
    // teraz te watki sobue jakos dzialaja wiec main tylko czeka na sygnal


    
    // // one-thread video_frame flow example. You can remove those lines
    // video_frame* f = decode_frame();
    // transform_frame(f);
    // display_frame(f);

    free(dt);
    exit(EXIT_SUCCESS);
}
