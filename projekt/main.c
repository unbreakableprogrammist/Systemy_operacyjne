#include "header.h"

static volatile sig_atomic_t running = 1;

// Funkcja usuwająca '\n' z końca
void remove_newline(char *line) { line[strcspn(line, "\n")] = 0; } // strcspn znajduje w line pierwszy znak z \n i zwraca jego indeks

// Wypisywanie listy
void list_all(struct wezel *head) { // przekazujemy wskaźnik na początek listy
  struct wezel *tmp = head;
  printf("--- Lista procesów ---\n");
  while (tmp != NULL) {
    printf("PID: %d | %s -> %s\n", tmp->pid, tmp->source_path,
           tmp->destination_path);
    tmp = tmp->next;
  }
  printf("----------------------\n");
}

// Przekazujemy adres wskaźnika head (**head)
void delete_node(struct wezel **head_ref, char *source_path, char *destination_path) { 
    struct wezel *tmp = *head_ref;
    struct wezel *prev = NULL;
    while (tmp != NULL) {
        // jesli source path i destination path są takie same jak te z wezla
        if (strcmp(tmp->source_path, source_path) == 0 && strcmp(tmp->destination_path, destination_path) == 0) {
            printf("Zatrzymywanie PID %d...\n", tmp->pid);
            // zabijamy proces o tym pid
            kill(tmp->pid, SIGTERM);
            if(waitpid(tmp->pid, NULL, 0)<0){
                if (errno == EINTR) continue;       
                perror("waitpid");
                return;
            }
            // usuwamy wezel z listy
            if (prev == NULL) {
                *head_ref = tmp->next;
            } else {
                prev->next = tmp->next;
            }
            free(tmp);
            return EXIT_SUCCESS;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    printf("Nie znaleziono takiego zadania.\n");
    return;
}

int tokenize(char *line, char *args[]) {
    int arg_count = 0;
    // nasza flaga czy w cudzyslowiach
    int in_quote = 0;
    char *src = line;
    char *dst = line;
    memset(args, 0, MAXARGS * sizeof(char *));
    while (*src && arg_count < MAXARGS) { // dopoki src jest rozny od \0 i arg_count jest mniejszy od MAXARGS
        while (*src == ' ' && !in_quote) // omijamy nadmairowe spacje i nie wewnątrz cudzysłowia
            src++;
        if (*src == '\0') // jesli doszlismy do konca linii to konczymy
            break;
        args[arg_count++] = dst; // przypisujemy adres do tablicy argumentow
        while (*src) { // dopoki src jest rozny od \0
            if (*src == '"') {
                in_quote = !in_quote; // zmieniamy flagę
                src++;
            } else if (*src == ' ' && !in_quote) { // jesli src jest spacja i nie wewnątrz cudzysłowia
                src++;
                break;
            } else {
                *dst++ = *src++; // przepisujemy znak do dst
            }
        }
        *dst++ = '\0'; // przepisujemy \0 do dst
    }
    return arg_count;
}

void handler(int sig) { running = 0; }

int is_subpath(const char *parent, const char *child) {
    char real_parent[PATH_MAX]; //cala sciezka zrodlowa
    char real_child[PATH_MAX]; //cala sciezka celu

    
    if (realpath(parent, real_parent) == NULL){ // zrodlo musi na pewno istniec , inaczej blad programu 
        perror("realpath");
        return EXIT_FAILURE;
    } 

    //Próbujemy rozwiązać ścieżkę docelowego katalogu
    if (realpath(child, real_child) == NULL) { // jesli cel nie istnieje 
        // Cel nie istnieje. Musimy rozwiązać jego katalog nadrzędny ("parent directory" celu).
        // Np. dla child="../dst1", parent_part="..", name_part="dst1"
        // przepisujemy do temp child taka sciezke jaka dostalismy
        char temp_child[PATH_MAX];
        strncpy(temp_child, child, PATH_MAX - 1);
        temp_child[PATH_MAX - 1] = '\0';
        // tworzymy tablice do przechowywania rodzica i nazwy
        char parent_part[PATH_MAX];
        char name_part[PATH_MAX];
        
        // Znajdujemy ostatni ukośnik
        char *last_slash = strrchr(temp_child, '/');

        if (last_slash == NULL) {
            // Brak ukośnika, np. "dst1". Rodzicem jest "."
            strcpy(parent_part, ".");
            strcpy(name_part, temp_child);
        } else {
            // Jest ukośnik, np. "../dst1" lub "/tmp/dst1"
            if (last_slash == temp_child) { // Przypadek "/dst1" , czyli przypadek z rootem
                strcpy(parent_part, "/");
            } else {
                *last_slash = '\0'; // Ucinamy string w miejscu /
                strcpy(parent_part, temp_child); // To co przed /
            }
            strcpy(name_part, last_slash + 1); // To co po /
        }

        // Teraz robimy realpath na RODZICU celu 
        char real_parent_part[PATH_MAX];
        if (realpath(parent_part, real_parent_part) == NULL) { // oznacza folder nie istnieje -> mozemy go stworzyc
           return 0;
        }
        // Sklejamy rozwiązaną ścieżkę rodzica z nazwą folderu
        snprintf(real_child, PATH_MAX, "%s/%s", real_parent_part, name_part);
    }


    // Sprawdź czy to ten sam katalog
    if (strcmp(real_parent, real_child) == 0) return 1;
    // Sprawdź czy real_child zaczyna się od real_parent
    size_t len = strlen(real_parent);
    if (strncmp(real_parent, real_child, len) == 0) {
        // Upewnij się, że to podkatalog (następny znak to '/' lub koniec)
        // Chroni przed: /home/test vs /home/test2
        if (real_child[len] == '/' || real_child[len] == '\0') {
            return 1; // błąd
        }
    }

    return 0; // Bezpiecznie
}

// Funkcja sprawdza, czy para <zrodlo> <cel> już istnieje w liście aktywnych procesów.
// Zwraca 1 (DUPLIKAT), 0 (OK)
int is_duplicate(struct wezel *head, const char *src, const char *dst) {
    struct wezel *tmp = head;
    while (tmp != NULL) {
        // Porównujemy napisy. Jeśli oba są identyczne jak w nowym poleceniu -> BŁĄD
        if (strcmp(tmp->source_path, src) == 0 && strcmp(tmp->destination_path, dst) == 0) {
            return 1; 
        }
        tmp = tmp->next;
    }
    return 0;
}

int main() {
    // bufor na linie wejściowe
    char line[MAXLINE];
    // tablica argumentów
    char *args[MAXARGS];
    // wskaźnik na początek listy tej pid - id procesu, source_path - źródło,
    // destination_path - cel
    struct wezel *head = NULL;
    printf("Start programu. Komendy: add <src> <dst1>..., list, end <src> <dst>, ""exit, help\n");
    // ignorujemy sygnały inne niż SIGINT, SIGTERM, SIGCHLD, SIGKILL, SIGSTOP, SIGSEGV, SIGFPE (SIGSEGV i SIGFPE sygnaly do handlowania bledow segfoult i floating point exception)
    struct sigaction sa_ign;
    memset(&sa_ign, 0, sizeof(sa_ign));
    sa_ign.sa_handler = SIG_IGN;
    sigemptyset(&sa_ign.sa_mask);
    for (int i = 1; i < NSIG; i++) {
        if (i != SIGINT && i != SIGTERM && i != SIGCHLD && i != SIGKILL && i != SIGSTOP && i != SIGSEGV && i != SIGFPE) {
            sigaction(i, &sa_ign, NULL);
        }
    }
    // nadpisujemy handler na sygnaly SIGINT i SIGTERM
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    if (sigaction(SIGINT, &sa, NULL) < 0)
        ERR("sigaction");
    if (sigaction(SIGTERM, &sa, NULL) < 0)
        ERR("sigaction");
    // dopoki nie przyjdzie sygnal SIGINT lub SIGTERM
    while (running) {
        if (fgets(line, sizeof(line), stdin) == NULL) { // bierz cala linijke wejsciowa az d- '\n'( null oznacza blad)
            if (errno == EINTR)
                continue; // jesli przerwanie zostalo przez sygnal to kontynuuj
            break;      // w takim przypadku zamykamy petle
        }
        remove_newline(line); // usuwamy '\n' z końca
        int argc = tokenize(line, args); // dzielimy linijke wejsciowa na argumenty
        if (argc == 0)
            continue; // jesli nie ma argumentow to kontynuuj

        if (strcmp(args[0], "exit") ==0) { // jesli pierwszy argument to exit to zamykamy petle
            running = 0;
            break;
        }

        if (strcmp(args[0], "add") == 0) {
            if (argc < 3) {
                printf("Za mało argumentów.\n");
                continue;
            }
            for (int i = 2; i < argc; i++) {
                if (is_subpath(args[1], args[i])) {
                    printf("BŁĄD: Cel '%s' znajduje się wewnątrz źródła '%s'!\n", args[i],args[1]);
                    continue;
                }
                if (is_duplicate(head, args[1], args[i])) {
                    printf("BŁĄD: Kopia '%s' -> '%s' jest już aktywna!\n", args[1], args[i]);
                    continue; // Pomijamy ten cel, nie robimy fork
                }
                pid_t pid = fork(); // dla kazdego celu tworzy nowy proces
                if (pid == -1) { // jesli blad wypisz blad i kontynuuj
                    perror("fork");
                    continue;
                }

                if (pid == 0) { // jesli jest dzieckiem to uruchom file_watcher_reccursive
                    file_watcher_reccursive(args[1], args[i]);
                    exit(EXIT_SUCCESS);
                }
                // RODZIC
                struct wezel *nowy = malloc(sizeof(struct wezel));
                nowy->pid = pid;
                strncpy(nowy->source_path, args[1], MAX_PATH - 1);
                nowy->source_path[MAX_PATH - 1] = 0; // ustawiamy ostatni znak na \0
                strncpy(nowy->destination_path, args[i], MAX_PATH - 1);
                nowy->destination_path[MAX_PATH - 1] = 0; // ustawiamy ostatni znak na \0
                nowy->next = head; // dodajemy nowy element na początek listy
                head = nowy;
            }
        } else if (strcmp(args[0], "list") == 0) {
            list_all(head); // wyswietla liste wszystkich procesow
        } else if (strcmp(args[0], "end") == 0) {
            if (argc < 3) {
                printf("Podaj źródło i cel.\n");
                continue;
            }
            delete_node(&head, args[1], args[2]); // usuwa proces z listy
        } else if (strcmp(args[0], "help") == 0) {
            printf("\n--- Dostępne komendy ---\n");
            printf("  add <zrodlo> <cel> [cel2 ...]  - Uruchamia backup\n");
            printf("  list                           - Lista procesów\n");
            printf("  end <zrodlo> <cel>             - Zatrzymuje proces\n");
            printf("  exit                           - Wyjście\n");
            printf("------------------------\n");
        }else{
            continue;
        }
    }
    // teraz juz jestesmy po exit wiec sobie wszystko czyscimy 
    struct wezel *current = head;
    while (current != NULL) {
        if (current->pid > 0) {
            kill(current->pid, SIGTERM);
        }
        struct wezel *to_free = current;
        current = current->next;
        free(to_free);
    }
    // zwalniamy zasoby
    while (wait(NULL) > 0);
    return EXIT_SUCCESS;
}
