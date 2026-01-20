#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>


#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define MAX_PATH 50

typedef struct ogolna{
    off_t start_byte;
    off_t byte_to_read;
    int jest_zadanie;
    pthread_mutex_t task_mtx;
    pthread_cond_t zadanie_od_main;
    int can_send;
    pthread_mutex_t can_send_mtx;
    pthread_cond_t mozna_wyslac;
}ogolna;

typedef struct do_threada{
    ogolna* ogolne;
    int id;
    off_t start;
} do_threada;

int main(int argc,char** argv){
    if(argc != 4) ERR("invalid number of arguments");
    // wczytanie argumentow 
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    char path[MAX_PATH];
    snprintf(argv[3],MAX_PATH,"%s",argv[3]);
    // czytamy sobie rzeczy o pliku 
    struct stat st;
    if (stat(path, &st) != 0) {
        perror("Błąd stat");
        return 1;
    }
    off_t file_size = st.st_size;
    // teraz sobie otwieramy plik 
    FILE *fp = fopen(path,"r"); // otwieramy plik do czytania 
    char *header = NULL;
    size_t header_len = 0;
    getline(&header, &header_len, fp); // czytamy do endline ( włacznie )
    off_t data_start_offset = ftell(fp); // ftell mowi nam gdzie skonczylismy getlinem czytac 
    off_t data_to_read = file_size - data_start_offset; // mowi nam ile mamy liczb
    off_t data_per_task = data_to_read / m; // ile bajtow per task
    

}