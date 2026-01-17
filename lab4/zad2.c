#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

#define BUFFERSIZE 256  // rozmiar bufora do czytania/zapisu
#define THREAD_NUM 3  // liczba wątków roboczych

volatile sig_atomic_t work = 1;  // flaga pracy programu

ssize_t bulk_read(int fd, char *buf, size_t count) {  // skopiowane , czyta dokładnie count bajtów
    int c;
    size_t len = 0;
    do {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0) return c;
        if (c == 0) return len;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

ssize_t bulk_write(int fd, char *buf, size_t count) {
    int c;
    size_t len = 0;
    do {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0) return c;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

typedef struct common_data {  // dane wspodzielone
    int is_work;
    int free_threads;
    pthread_cond_t cv;
    pthread_mutex_t mutex;
} common_data;

typedef struct arg {  // argumenty wątku (kazda ma unikalne id)
    int id;
    common_data* cd;
} arg;

void handler(int sig) {  // obsługa sygnału SIGINT
    work = 0;
}

void cleanup(void* arg) {  // funkcja sprzątająca do pthread_cleanup_push/pop ( jak przyjdzie sygnal)
    pthread_mutex_unlock((pthread_mutex_t*)arg);
}

void* thread_work(void* argument) {
    arg* args = (arg*) argument;  // zdejmujemy argumenty
    common_data* cd = args->cd;
    char buffer[BUFFERSIZE];

    while (1) {  // GŁÓWNA PĘTLA WĄTKA
        pthread_mutex_lock(&cd->mutex);  // blokujemy mutex na czas sprawdzania warunków
        
        int should_exit = 0;

        pthread_cleanup_push(cleanup, &cd->mutex);  // dodajemy co jesli przyjdzie sygnal - odblokowanie mutexa

        cd->free_threads++; // jestesmy wolnym watkiem 

        while (cd->is_work == 0 && work) {  // czekamy az is_work zostanie ustawione na 1 lub przyjdzie SIGINT
            pthread_cond_wait(&cd->cv, &cd->mutex);  // to od razy odblokowuje mutex i blokuje watek, az gdy cv zostanie zsignalone
        }
        // skoro tu jestesmy to albo is_work == 1 = mamy prace , albo work == 0 ( SIGINT)
        if (!work) { // jesli dostalismy sygnal do zakonczenia
            should_exit = 1;  // zmieniamy flage ze chcemy wyjsc 
        } else {
            cd->is_work = 0;
            cd->free_threads--;
        }
        // poniewaz jest 1 - zawsze zdejmuje cleanup ( odblokowuje mutex)
        pthread_cleanup_pop(1); 
        
        // jesli mamy konczyc to nara
        if (should_exit) break;

        // zaczynamy prace 
        printf("Watek %d zaczyna prace\n", args->id);
        // losowe ziarno dla kazdego watku i randomowa liczba bajtow do czytania
        unsigned int seed = time(NULL) ^ args->id;
        int number_of_bytes = (rand_r(&seed) % (BUFFERSIZE - 1)) + 1;
        printf("Watek %d bedzie czytal %d bajtow\n", args->id, number_of_bytes);

        // tworzymy nazwe pliku
        char filename[32];
        snprintf(filename, sizeof(filename), "random%d.bin", args->id);

        int fd_in, fd_out;
        // otweramy pliki
        if ((fd_in = TEMP_FAILURE_RETRY(open("/dev/urandom", O_RDONLY))) < 0) 
            ERR("open input");
            
        if ((fd_out = TEMP_FAILURE_RETRY(open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0644))) < 0) {
            close(fd_in);
            ERR("open output");
        }
        // czytamy sobie
        ssize_t read_bytes = bulk_read(fd_in, buffer, number_of_bytes);
        if (read_bytes < 0) ERR("bulk read");
        // to dodalem zeby udawalo ze to pracuje ( zbyt szybko czytal skubany)
        printf("Watek %d udaje, ze ciezko pracuje (spie 5 sekund)...\n", args->id);
        sleep(5);

        ssize_t written_bytes = bulk_write(fd_out, buffer, read_bytes);
        if (written_bytes < 0) ERR("bulk write");

        printf("Watek %d zakonczyl prace\n", args->id);

        close(fd_in);
        close(fd_out);
    }
    
    free(args);
    return NULL;
}

int main() {
    // obsluga sygnalu SIGINT
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    if (sigaction(SIGINT, &sa, NULL) < 0) ERR("sigaction");

    // alokujemy i wypelniamy  pamiec wspoldzielona
    common_data* argument = malloc(sizeof(common_data)); 
    if (!argument) ERR("malloc");
    argument->is_work = 0;
    argument->free_threads = 0; 
    if (pthread_cond_init(&argument->cv, NULL) != 0) ERR("init");
    if (pthread_mutex_init(&argument->mutex, NULL) != 0) ERR("init");

    // tablica threadow 
    pthread_t tid[THREAD_NUM];
    for (int i = 0; i < THREAD_NUM; i++) {
        arg* personalised_id = malloc(sizeof(arg));
        personalised_id->cd = argument;
        personalised_id->id = i + 1;
        if (pthread_create(&tid[i], NULL, thread_work, personalised_id) != 0) ERR("create");
    }


    char input_buffer[20];
    while (work) {
        printf("Nacisnij enter aby obudzic watek...\n");
        if (fgets(input_buffer, BUFFERSIZE, stdin) != NULL) {
            pthread_mutex_lock(&argument->mutex); // blokujemy mutex
            if (argument->free_threads > 0) {  // jesli mamy wolnych murzynow
                argument->is_work = 1;  // dajemy prace 
                pthread_cond_signal(&argument->cv); // uruchamiamy 
            } else {
                printf("Brak wolnych watkow, czekam...\n");
            }
            pthread_mutex_unlock(&argument->mutex);  // odblokowanie mutexa 
        } else {
            if (errno == EINTR) continue; // jesli to bylo jakies dziwne przerwanie to lecimy dalej 
            break;
        }
    }
    // WAŻNE wybudzamy wszystkie watki 
    pthread_cond_broadcast(&argument->cv);

    //joinujemy
    for (int i = 0; i < THREAD_NUM; i++) {
        pthread_join(tid[i], NULL);
    }
    
    pthread_mutex_destroy(&argument->mutex);
    pthread_cond_destroy(&argument->cv);
    free(argument);

    return 0;
}