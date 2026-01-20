#define _GNU_SOURCE // Wymagane dla funkcji getline
#include <stdio.h> // Standardowe wejście/wyjście
#include <stdlib.h> // Funkcje standardowe (atoi, malloc itp.)
#include <unistd.h> // Funkcje systemowe POSIX
#include <pthread.h> // Biblioteka wątków POSIX
#include <string.h> // Operacje na napisach
#include <sys/types.h> // Definicje typów systemowych
#include <sys/stat.h> // Statystyki plików (rozmiar)

// Makro do obsługi błędów systemowych
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define MAX_PATH 100 // Maksymalna długość ścieżki

// Struktura pojedynczego ogniwa listy dwukierunkowej
typedef struct Node {
    char* line; // Przechowuje treść linii CSV
    struct Node *prev, *next; // Wskaźniki na poprzedni i następny element
} Node;

// Struktura zarządzająca całą listą
typedef struct List {
    Node *head, *tail; // Początek i koniec listy
} List;

// Główna struktura synchronizacyjna (Współdzielona)
typedef struct ogolna {
    off_t current_start; // Skąd dany wątek ma zacząć czytać w bajtach
    off_t current_size;  // Ile bajtów dany wątek ma przeczytać
    int task_ready;      // Flaga: 1 = zadanie czeka na wątek, 0 = brak zadania
    int m_left;          // Licznik pozostałych fragmentów do rozdania
    int active_workers;  // Ilu pracowników w tej sekundzie przetwarza dane
    int error_flag;      // Flaga błędu: 1 = wykryto błąd w CSV
    int commas_expected; // Prawidłowa liczba przecinków (z nagłówka)
    char* path;          // Ścieżka do pliku
    pthread_mutex_t mtx; // Mutex chroniący całą tę strukturę
    pthread_cond_t cond_worker; // Zmienna: Wątek czeka na zadanie od Maina
    pthread_cond_t cond_main;   // Zmienna: Main czeka aż wątek odbierze zadanie
} ogolna;

// Struktura argumentów przekazywanych do każdego wątku
typedef struct do_threada {
    ogolna* og; // Wskaźnik do struktury współdzielonej
    int id;     // Unikalny numer wątku (do logów)
    List* local_list; // Wskaźnik na prywatną listę tego wątku
} do_threada;

// Funkcja dodająca napis do listy dwukierunkowej
void add_to_list(List* l, const char* line) {
    Node* new_node = malloc(sizeof(Node)); // Alokacja pamięci na nowy węzeł
    new_node->line = strdup(line); // Kopiowanie napisu
    new_node->next = NULL; // Nowy element jest ostatni
    new_node->prev = l->tail; // Poprzednim jest obecny koniec
    if (l->tail) l->tail->next = new_node; // Jeśli lista nie była pusta, połącz
    else l->head = new_node; // Jeśli była pusta, to nowy jest głową
    l->tail = new_node; // Nowy staje się końcem listy
}

// Liczenie przecinków w napisie (walidacja CSV)
int count_commas(const char* s) {
    int count = 0; // Licznik
    while (*s) { if (*s == ',') count++; s++; } // Przejdź po każdym znaku
    return count; // Zwróć wynik
}

// Funkcja wykonywana przez każdy z N wątków
void* thread_work(void* arg) {
    do_threada* dt = (do_threada*)arg; // Rzutowanie argumentu
    ogolna* og = dt->og; // Skrót do struktury współdzielonej

    while (1) { // Pętla główna wątku (pula zadań)
        pthread_mutex_lock(&og->mtx); // Wejdź do sekcji krytycznej
        // Czekaj jeśli: nie ma zadania ORAZ są jeszcze jakieś zadania do wysłania ORAZ nie ma błędu
        while (og->task_ready == 0 && og->m_left > 0 && og->error_flag == 0) {
            pthread_cond_wait(&og->cond_worker, &og->mtx); // Zasypiaj na zmiennej worker
        }

        // Jeśli wszystkie m zadań rozdane LUB ktoś inny zgłosił błąd -> kończymy
        if ((og->m_left <= 0 && og->task_ready == 0) || og->error_flag) {
            pthread_mutex_unlock(&og->mtx); // Puść mutex przed wyjściem
            break; // Wyjdź z pętli while(1)
        }

        off_t start = og->current_start; // Skopiuj start zadania do zmiennej lokalnej
        off_t size = og->current_size;   // Skopiuj rozmiar zadania
        og->task_ready = 0; // Zdejmij flagę: zadanie odebrane
        og->active_workers++; // Zwiększ licznik pracujących wątków
        
        pthread_cond_signal(&og->cond_main); // Obudź Maina: "Wziąłem zadanie, możesz szykować następne"
        pthread_mutex_unlock(&og->mtx); // Wyjdź z sekcji krytycznej, by inni mogli działać

        FILE* f = fopen(og->path, "r"); // Otwórz plik (każdy wątek ma swój deskryptor)
        fseek(f, start, SEEK_SET); // Skocz do przypisanego bajtu
        char* line = NULL; size_t len = 0; off_t read_bytes = 0; // Zmienne do getline

        // Czytaj dopóki nie przekroczysz rozmiaru fragmentu
        while (read_bytes < size && getline(&line, &len, f) != -1) {
            read_bytes += strlen(line); // Zwiększ licznik przeczytanych bajtów
            if (count_commas(line) != og->commas_expected) { // Walidacja CSV
                pthread_mutex_lock(&og->mtx); // Błąd! Wejdź do sekcji krytycznej
                og->error_flag = 1; // Ustaw flagę błędu dla wszystkich
                fprintf(stderr, "Błąd w wątku %d: Niepoprawna liczba przecinków.\n", dt->id);
                og->active_workers--; // Ten wątek już nie pracuje (bo stoi na błędzie)
                while (og->active_workers > 0) { // CZEKAJ NA INNE WĄTKI (Wymóg zadania)
                    pthread_cond_wait(&og->cond_main, &og->mtx); // Czekaj aż inni skończą swoje linie
                }
                pthread_cond_broadcast(&og->cond_worker); // Obudź wszystkich śpiących, by zobaczyli błąd i wyszli
                pthread_mutex_unlock(&og->mtx); // Puść mutex
                free(line); fclose(f); return NULL; // Sprzątnij i zakończ wątek
            }
            add_to_list(dt->local_list, line); // Jeśli ok, dodaj do prywatnej listy
        }
        free(line); fclose(f); // Zamknij plik i zwolnij bufor getline

        pthread_mutex_lock(&og->mtx); // Zakończono czytanie fragmentu
        og->active_workers--; // Zmniejsz licznik aktywnych
        pthread_cond_broadcast(&og->cond_main); // Powiadom Maina/Wątek błędu o zakończeniu pracy
        pthread_mutex_unlock(&og->mtx); // Puść mutex
    }
    return NULL; // Koniec wątku
}

int main(int argc, char** argv) {
    if (argc != 4) ERR("Użycie: n m path"); // Sprawdzenie liczby argumentów

    int n = atoi(argv[1]); // n: liczba wątków
    int m = atoi(argv[2]); // m: liczba fragmentów pliku
    char path[MAX_PATH]; // Bufor na ścieżkę
    snprintf(path, MAX_PATH, "%s", argv[3]); // Bezpieczne kopiowanie ścieżki

    struct stat st; // Struktura na info o pliku
    if (stat(path, &st) != 0) ERR("Błąd pliku"); // Pobierz rozmiar pliku
    
    FILE *fp = fopen(path, "r"); // Otwórz plik do wczytania nagłówka
    char *header = NULL; size_t h_len = 0; // Zmienne dla nagłówka
    getline(&header, &h_len, fp); // Wczytaj pierwszą linię
    int commas = count_commas(header); // Ustal wzorcową liczbę przecinków
    off_t data_start = ftell(fp); // Zapamiętaj gdzie kończy się nagłówek
    fclose(fp); // Zamknij plik

    // Inicjalizacja współdzielonej struktury
    ogolna og = {
        .m_left = m, .task_ready = 0, .active_workers = 0, 
        .error_flag = 0, .commas_expected = commas, .path = path,
        .mtx = PTHREAD_MUTEX_INITIALIZER, // Statyczny mutex
        .cond_worker = PTHREAD_COND_INITIALIZER, // Statyczna zmienna warunkowa
        .cond_main = PTHREAD_COND_INITIALIZER    // Statyczna zmienna warunkowa
    };

    pthread_t threads[n]; // Tablica ID wątków
    do_threada thread_data[n]; // Tablica argumentów dla wątków
    List local_lists[n]; // Prywatne listy dla każdego wątku

    for (int i = 0; i < n; i++) { // Tworzenie n wątków
        local_lists[i].head = local_lists[i].tail = NULL; // Zerowanie listy
        thread_data[i] = (do_threada){&og, i, &local_lists[i]}; // Przygotowanie danych
        pthread_create(&threads[i], NULL, thread_work, &thread_data[i]); // Start wątku
    }

    off_t total_data = st.st_size - data_start; // Rozmiar samych danych w bajtach
    off_t chunk = total_data / m; // Bazowy rozmiar jednego z m fragmentów

    for (int i = 0; i < m; i++) { // Główna pętla Maina - rozdawanie m zadań
        pthread_mutex_lock(&og.mtx); // Sekcja krytyczna
        while (og.task_ready == 1 && og.error_flag == 0) { // Dopóki poprzednie zadanie nie odebrane
            pthread_cond_wait(&og.cond_main, &og.mtx); // Main zasypia (czeka na wolny slot)
        }
        if (og.error_flag) { pthread_mutex_unlock(&og.mtx); break; } // Jeśli błąd, przestań rozdawać

        og.current_start = data_start + (i * chunk); // Wylicz start bajtowy
        // Ostatni fragment bierze wszystko do końca pliku
        og.current_size = (i == m - 1) ? (st.st_size - og.current_start) : chunk; 
        og.task_ready = 1; // Ustaw flagę: Zadanie gotowe!
        pthread_cond_signal(&og.cond_worker); // Obudź jeden wolny wątek robotniczy
        pthread_mutex_unlock(&og.mtx); // Puść mutex
    }

    for (int i = 0; i < n; i++) pthread_join(threads[i], NULL); // Czekaj na zakończenie wszystkich n wątków

    if (!og.error_flag) { // Jeśli nie było błędu, wypisz wyniki
        printf("%s", header); // Wypisz nagłówek raz
        for (int i = 0; i < n; i++) { // Przejdź po listach wszystkich wątków
            Node* curr = local_lists[i].head; // Zacznij od głowy każdej listy
            while (curr) { printf("%s", curr->line); curr = curr->next; } // Drukuj i idź dalej
        }
    }

    free(header); // Zwolnij pamięć nagłówka
    return 0; // Koniec programu
}