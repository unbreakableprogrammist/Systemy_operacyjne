#include "header.h"

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

// POPRAWKA: Przekazujemy adres wskaźnika head (**head), żeby móc zmienić początek listy
void delete_node(struct wezel **head_ref, char *source_path, char *destination_path) {
    struct wezel *tmp = *head_ref;
    struct wezel *prev = NULL;

    while (tmp != NULL) {
        // Sprawdzamy czy ścieżki się zgadzają
        if (strcmp(tmp->source_path, source_path) == 0 && strcmp(tmp->destination_path, destination_path) == 0) {
            
            // 1. Najważniejsze: ZABIJAMY PROCES
            printf("Zatrzymywanie PID %d...\n", tmp->pid);
            kill(tmp->pid, SIGTERM);
            waitpid(tmp->pid, NULL, 0); // Czekamy aż na pewno zniknie

            // 2. Przepinamy wskaźniki
            if (prev == NULL) {
                *head_ref = tmp->next; // Usuwamy pierwszy element
            } else {
                prev->next = tmp->next; // Usuwamy ze środka/końca
            }

            // 3. Zwalniamy pamięć
            free(tmp);
            return;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    printf("Nie znaleziono takiego zadania.\n");
}

// (Funkcja tokenize pozostaje bez zmian - jest OK)
int tokenize(char *line, char *args[]) {
    // ... (Twoja implementacja tokenize) ...
    // Wklej tutaj swoją funkcję tokenize
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

int main() {
    char line[MAXLINE];
    char *args[MAXARGS];
    struct wezel *head = NULL;

    printf("Start programu. Komendy: add <src> <dst1>..., list, end <src> <dst>, exit\n");

    while (1) {
        printf("> ");
        fflush(stdout); // Ważne!

        if (fgets(line, sizeof(line), stdin) == NULL) break;
        remove_newline(line);

        int argc = tokenize(line, args);
        if (argc == 0) continue;

        if (strcmp(args[0], "exit") == 0) {
            // Tu przydałaby się pętla czyszcząca całą listę i zabijająca dzieci
            break; 
        }

        if (strcmp(args[0], "add") == 0) {
            if (argc < 3) { printf("Za mało argumentów.\n"); continue; }

            // Iterujemy po celach
            for (int i = 2; i < argc; i++) {
                pid_t pid = fork();
                if (pid == -1) { ERR("fork"); }
                
                if (pid == 0) { 
                    // DZIECKO
                    file_watcher_reccursive(args[1], args[i]);
                    exit(EXIT_SUCCESS);
                }

                // RODZIC
                struct wezel *nowy = malloc(sizeof(struct wezel));
                nowy->pid = pid;
                strncpy(nowy->source_path, args[1], MAX_PATH-1); nowy->source_path[MAX_PATH-1] = 0;
                strncpy(nowy->destination_path, args[i], MAX_PATH-1); nowy->destination_path[MAX_PATH-1] = 0;

                // POPRAWKA: Wstawianie na początek listy (zawsze działa)
                nowy->next = head;
                head = nowy;
                
            }
        } 
        else if (strcmp(args[0], "list") == 0) {
            list_all(head);
        } 
        else if (strcmp(args[0], "end") == 0) {
            if (argc < 3) { printf("Podaj źródło i cel.\n"); continue; }
            // POPRAWKA: Przekazujemy adres wskaźnika head (&head)
            delete_node(&head, args[1], args[2]);
        }
        sleep(1);
    }
    
    // Czekamy na wszystkie dzieci przy wyjściu
    while(wait(NULL) > 0);
    return EXIT_SUCCESS;
}