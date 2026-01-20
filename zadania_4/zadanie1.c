#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define MAX_PATH 50

// Struktura listy dwukierunkowej (dla etapu 3 i 5)
typedef struct Node {
    char* line;
    struct Node *prev, *next;
} Node;

typedef struct List {
    Node *head, *tail;
} List;

// Struktura współdzielona przez wszystkie wątki
typedef struct ogolna {
    // Dane zadania (przekazywane z Main do Workerów)
    off_t current_start;
    off_t current_size;
    int task_ready;        // 1 jeśli Main przygotował zadanie
    int m_left;            // Ile fragmentów (m) zostało do rozdania
    
    // Status puli
    int active_workers;    // Ilu pracowników obecnie przetwarza plik
    int error_flag;        // Flaga błędu (1 jeśli ktoś napotkał błąd)
    int error_line_global; // Numer linii z błędem
    int commas_expected;   // Ile przecinków musi być w linii (z nagłówka)

    char* path;            // Ścieżka do pliku (do ponownego otwarcia w wątkach)

    // Synchronizacja
    pthread_mutex_t mtx;
    pthread_cond_t cond_worker; // Wątki czekają na zadanie
    pthread_cond_t cond_main;   // Main czeka aż zadanie zostanie odebrane
} ogolna;

// Dane przekazywane do każdego wątku przy starcie
typedef struct do_threada {
    ogolna* og;
    int id;
    List* local_list; // Każdy wątek ma swoją listę
} do_threada;

// Funkcja pomocnicza do dodawania do listy
void add_to_list(List* l, const char* line) {
    Node* new_node = malloc(sizeof(Node));
    new_node->line = strdup(line);
    new_node->next = NULL;
    new_node->prev = l->tail;
    if (l->tail) l->tail->next = new_node;
    else l->head = new_node;
    l->tail = new_node;
}

// Funkcja licząca przecinki (do walidacji CSV)
int count_commas(const char* s) {
    int count = 0;
    while (*s) if (*s++ == ',') count++;
    return count;
}

void* thread_work(void* arg) {
    do_threada* dt = (do_threada*)arg;
    ogolna* og = dt->og;

    while (1) {
        pthread_mutex_lock(&og->mtx);
        // Czekamy na zadanie lub sygnał zakończenia/błędu
        while (og->task_ready == 0 && og->m_left > 0 && og->error_flag == 0) {
            pthread_cond_wait(&og->cond_worker, &og->mtx);
        }

        // Jeśli nie ma zadań lub jest błąd u kogoś innego - kończymy
        if ((og->m_left <= 0 && og->task_ready == 0) || og->error_flag) {
            pthread_mutex_unlock(&og->mtx);
            break;
        }

        // Pobieramy dane zadania
        off_t start = og->current_start;
        off_t size = og->current_size;
        og->task_ready = 0;    // "Zabrałem zadanie"
        og->active_workers++;  // Zaczynam pracę
        
        pthread_cond_signal(&og->cond_main); // Daj znać Mainowi: "Zwolniłem miejsce na nowe zadanie"
        pthread_mutex_unlock(&og->mtx);

        // --- PRACA WŁAŚCIWA (Czytanie fragmentu) ---
        FILE* f = fopen(og->path, "r");
        fseek(f, start, SEEK_SET);
        char* line = NULL;
        size_t len = 0;
        off_t read_bytes = 0;

        while (read_bytes < size && getline(&line, &len, f) != -1) {
            read_bytes += strlen(line);
            if (count_commas(line) != og->commas_expected) {
                // NAPOTKANO BŁĄD
                pthread_mutex_lock(&og->mtx);
                og->error_flag = 1;
                og->error_line_global = -1; // W uproszczeniu - błąd w fragmencie
                fprintf(stderr, "BŁĄD CSV w wątku %d!\n", dt->id);
                
                og->active_workers--;
                // WYMÓG: Czekaj aż inne aktualnie działające skończą
                while (og->active_workers > 0) {
                    pthread_cond_wait(&og->cond_main, &og->mtx);
                }
                
                pthread_cond_broadcast(&og->cond_worker); // Obudź resztę, by zginęły
                pthread_mutex_unlock(&og->mtx);
                free(line);
                fclose(f);
                return NULL; 
            }
            add_to_list(dt->local_list, line);
        }
        free(line);
        fclose(f);

        // Kończenie zadania
        pthread_mutex_lock(&og->mtx);
        og->active_workers--;
        pthread_cond_broadcast(&og->cond_main); // Powiadom, że skończyłeś (ważne dla błędu)
        pthread_mutex_unlock(&og->mtx);
    }
    return NULL;
}

int main(int argc, char** argv) {
    if (argc != 4) ERR("Użycie: n m path");

    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    char path[MAX_PATH];
    snprintf(path, MAX_PATH, "%s", argv[3]); // Poprawione kopiowanie

    struct stat st;
    if (stat(path, &st) != 0) ERR("stat");
    
    FILE *fp = fopen(path, "r");
    if (!fp) ERR("fopen");

    char *header = NULL; size_t h_len = 0;
    getline(&header, &h_len, fp);
    int commas = count_commas(header);
    off_t data_start = ftell(fp);
    fclose(fp);

    // Inicjalizacja struktury ogólnej
    ogolna og = {
        .m_left = m, .task_ready = 0, .active_workers = 0, 
        .error_flag = 0, .commas_expected = commas, .path = path,
        .mtx = PTHREAD_MUTEX_INITIALIZER,
        .cond_worker = PTHREAD_COND_INITIALIZER,
        .cond_main = PTHREAD_COND_INITIALIZER
    };

    pthread_t threads[n];
    do_threada thread_data[n];
    List local_lists[n];

    for (int i = 0; i < n; i++) {
        local_lists[i].head = local_lists[i].tail = NULL;
        thread_data[i] = (do_threada){&og, i, &local_lists[i]};
        pthread_create(&threads[i], NULL, thread_work, &thread_data[i]);
    }

    // MAIN ROZDAJE ZADANIA
    off_t total_data_size = st.st_size - data_start;
    off_t chunk = total_data_size / m;

    for (int i = 0; i < m; i++) {
        pthread_mutex_lock(&og.mtx);
        
        // Czekaj aż któryś wątek zwolni "slot" na zadanie
        while (og.task_ready == 1 && og.error_flag == 0) {
            pthread_cond_wait(&og.cond_main, &og.mtx);
        }

        if (og.error_flag) {
            pthread_mutex_unlock(&og.mtx);
            break;
        }

        og.current_start = data_start + (i * chunk);
        og.current_size = (i == m - 1) ? (st.st_size - og.current_start) : chunk;
        og.task_ready = 1;
        og.m_left--;

        pthread_cond_signal(&og.cond_worker);
        pthread_mutex_unlock(&og.mtx);
    }

    // Sprzątanie i łączenie
    for (int i = 0; i < n; i++) pthread_join(threads[i], NULL);

    if (!og.error_flag) {
        printf("--- WYNIKI CSV ---\n%s", header);
        for (int i = 0; i < n; i++) {
            Node* curr = local_lists[i].head;
            while (curr) {
                printf("%s", curr->line);
                curr = curr->next;
            }
        }
    }

    free(header);
    return 0;
}