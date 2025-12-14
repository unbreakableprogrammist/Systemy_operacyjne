#include "header.h"

// ZMIANA: static sprawia, że ta zmienna jest widoczna tylko w main.c
// Rozwiązuje to konflikt z plikiem file_watcher_reccursive.c
static volatile sig_atomic_t running = 1;

// Funkcja usuwająca '\n' z końca
void remove_newline(char *line) { line[strcspn(line, "\n")] = 0; }

// Wypisywanie listy
void list_all(struct wezel *head) {
    struct wezel *tmp = head;
    printf("--- Lista procesów ---\n");
    while (tmp != NULL) {
        printf("PID: %d | %s -> %s\n", tmp->pid, tmp->source_path, tmp->destination_path);
        tmp = tmp->next;
    }
    printf("----------------------\n");
}

// Przekazujemy adres wskaźnika head (**head)
void delete_node(struct wezel **head_ref, char *source_path, char *destination_path) {
    struct wezel *tmp = *head_ref;
    struct wezel *prev = NULL;

    while (tmp != NULL) {
        if (strcmp(tmp->source_path, source_path) == 0 && strcmp(tmp->destination_path, destination_path) == 0) {
            
            printf("Zatrzymywanie PID %d...\n", tmp->pid);
            kill(tmp->pid, SIGTERM);
            waitpid(tmp->pid, NULL, 0); 

            if (prev == NULL) {
                *head_ref = tmp->next; 
            } else {
                prev->next = tmp->next; 
            }

            free(tmp);
            return;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    printf("Nie znaleziono takiego zadania.\n");
}

int tokenize(char *line, char *args[]) {
    int arg_count = 0;
    int in_quote = 0;
    char *src = line;
    char *dst = line;
    memset(args, 0, MAXARGS * sizeof(char *));
    while (*src && arg_count < MAXARGS) {
        while (*src == ' ' && !in_quote) src++;
        if (*src == '\0') break;
        args[arg_count++] = dst;
        while (*src) {
            if (*src == '"') { in_quote = !in_quote; src++; }
            else if (*src == ' ' && !in_quote) { src++; break; }
            else { *dst++ = *src++; }
        }
        *dst++ = '\0';
    }
    return arg_count;
}

void handler(int sig){
    (void)sig; // Unused warning fix
    running = 0;
}

int main() {
    char line[MAXLINE];
    char *args[MAXARGS];
    struct wezel *head = NULL;

    printf("Start programu. Komendy: add <src> <dst1>..., list, end <src> <dst>, exit\n");
    
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    // Ważne: restartujemy syscalls (jak read), żeby fgets nie zwracał błędu przy byle sygnale, 
    // chyba że chcemy przerwać. Tu zostawiamy standardowo.
    if (sigaction(SIGINT, &sa, NULL) < 0) ERR("sigaction");
    if (sigaction(SIGTERM, &sa, NULL) < 0) ERR("sigaction");

    while (running) {
        printf("> ");
        fflush(stdout); 

        if (fgets(line, sizeof(line), stdin) == NULL) {
            if (errno == EINTR) continue;
            break;
        }
        remove_newline(line);

        int argc = tokenize(line, args);
        if (argc == 0) continue;

        if (strcmp(args[0], "exit") == 0) {
            running = 0;
            break; 
        }

        if (strcmp(args[0], "add") == 0) {
            if (argc < 3) { printf("Za mało argumentów.\n"); continue; }

            for (int i = 2; i < argc; i++) {
                pid_t pid = fork();
                if (pid == -1) { perror("fork"); continue; }
                
                if (pid == 0) { 
                    // DZIECKO
                    // Resetujemy obsługę sygnałów w dziecku na domyślną, 
                    // lub ustawiamy własną (zdefiniowaną w file_watcher).
                    // Tutaj file_watcher_reccursive ustawia swoje handlery.
                    file_watcher_reccursive(args[1], args[i]);
                    exit(EXIT_SUCCESS);
                }

                // RODZIC
                struct wezel *nowy = malloc(sizeof(struct wezel));
                nowy->pid = pid;
                strncpy(nowy->source_path, args[1], MAX_PATH-1); nowy->source_path[MAX_PATH-1] = 0;
                strncpy(nowy->destination_path, args[i], MAX_PATH-1); nowy->destination_path[MAX_PATH-1] = 0;

                nowy->next = head;
                head = nowy;
            }
        } 
        else if (strcmp(args[0], "list") == 0) {
            list_all(head);
        } 
        else if (strcmp(args[0], "end") == 0) {
            if (argc < 3) { printf("Podaj źródło i cel.\n"); continue; }
            delete_node(&head, args[1], args[2]);
        }
        else if(strcmp(args[0],"help")==0){
            printf("\n--- Dostępne komendy ---\n");
            printf("  add <zrodlo> <cel> [cel2 ...]  - Uruchamia backup\n");
            printf("  list                           - Lista procesów\n");
            printf("  end <zrodlo> <cel>             - Zatrzymuje proces\n");
            printf("  exit                           - Wyjście\n");
            printf("------------------------\n");  
        }
        sleep(1);
    }
    
    // Sprzątanie
    struct wezel *tmp = head;
    while(tmp != NULL) {
        kill(tmp->pid, SIGTERM);
        tmp = tmp->next;
    }
 
    while(wait(NULL) > 0);
    return EXIT_SUCCESS;
}