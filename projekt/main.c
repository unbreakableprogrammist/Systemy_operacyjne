#include "header.h"
#include <string.h>

// funkcja ktora usuwa '\n' z konca stringa ( ustawia '\0' w miejscu '\n')
void remove_newline(char *line) { line[strcspn(line, "\n")] = 0; }

int tokenize(char *line, char *args[]) {
  int arg_count = 0;
  int in_quote = 0; // Flaga: czy jesteśmy wewnątrz cudzysłowu?
  char *src = line; // Skąd czytamy (wskaźnik źródłowy)
  char *dst = line; // Gdzie piszemy (wskaźnik docelowy)
  // Czyścimy tablicę argumentów na start
  memset(args, 0, MAXARGS * sizeof(char *));
  while (*src && arg_count < MAXARGS) { // src to nasz wskaznik idacy po lini
    // Pomijamy spacje początkowe (tylko te poza cudzysłowem)
    while (*src == ' ' && !in_quote)
      src++;
    if (*src == '\0')
      break; // Koniec linii
    // teraz wiemy ze nie jestesmy na spacji poza "" ani na koncu linii wiec
    // jestesmy na poczatku nowego argumentu
    args[arg_count++] = dst;
    while (*src) {
      if (*src == '"') {
        // Znalazł cudzysłów: przełącz flage, ale nie kopiuje znaku " do wyniku
        in_quote = !in_quote;
        src++;
      } else if (*src == ' ' && !in_quote) {
        // Spacja poza cudzysłowem = koniec tego argumentu
        src++; // Przeskocz spację w źródle
        break; // Wyjdź z pętli kopiowania
      } else {
        // Zwykły znak (lub spacja wewnątrz cudzysłowu) -> kopiuj
        *dst++ = *src++;
      }
    }
    *dst++ = '\0';
  }
  return arg_count;
}
struct wezel {
  pid_t pid;
  char source_path[MAX_PATH];
  char destination_path[MAX_PATH];
  struct wezel *next;
};

int main() {
  char line[MAXLINE];
  char *args[MAX_PATH];
  struct wezel *head = NULL;
  while (1) {
    // wypisujemy znak > i flushujemy od razu
    printf("> ");
    fflush(stdout);
    // teraz wczytujemy linijke do '\n'
    if (fgets(line, sizeof(line), stdin) == NULL) {
      ERR("fgets");
      break;
    }
    // usuwamy \n
    remove_newline(line);
    // dzielimy na argumenty
    int argc = tokenize(line, args);
    if (argc == 0)
      continue;
    // jesli jest polecenie exit to wyjdz
    if (strcmp(args[0], "exit") == 0)
      break;
    for (int i = 1; i < argc; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            ERR("fork");
        }
        if (pid == 0) { // tu uruchamiamy dziecko
            file_watcher_reccursive(args[i], args[i + 1]);
            exit(0);
        }
        struct wezel *nowy = malloc(sizeof(struct wezel));
        nowy->pid = pid;
        strncpy(nowy->source_path, args[i], MAX_PATH);
        nowy->source_path[MAX_PATH - 1] = '\0';
        strncpy(nowy->destination_path, args[i + 1], MAX_PATH);
        nowy->destination_path[MAX_PATH - 1] = '\0';
        nowy->next = NULL;
        if (head == NULL) head = nowy;

    }
    printf("\n");
  }
  return 0;
}