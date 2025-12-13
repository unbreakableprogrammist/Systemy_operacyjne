#include "header.h"

// funkcja ktora usuwa '\n' z konca stringa ( ustawia '\0' w miejscu '\n')
void remove_newline(char *line) {
    line[strcspn(line, "\n")] = 0;
}

int tokenize(char *line, char *args[]) {
    int arg_count = 0;
    int in_quote = 0;      // Flaga: czy jesteśmy wewnątrz cudzysłowu?
    char *src = line;      // Skąd czytamy (wskaźnik źródłowy)
    char *dst = line;      // Gdzie piszemy (wskaźnik docelowy)
    // Czyścimy tablicę argumentów na start
    memset(args, 0, MAXARGS * sizeof(char*));
    while (*src && arg_count < MAXARGS) { // src to nasz wskaznik idacy po lini
        //Pomijamy spacje początkowe (tylko te poza cudzysłowem)
        while (*src == ' ' && !in_quote) src++;
        if (*src == '\0') break; // Koniec linii
        // teraz wiemy ze nie jestesmy na spacji poza "" ani na koncu linii wiec jestesmy na poczatku nowego argumentu
        args[arg_count++] = dst; 
        while (*src) {
            if (*src == '"') {
                // Znalazł cudzysłów: przełącz flage, ale nie kopiuje znaku " do wyniku
                in_quote = !in_quote;
                src++;
            } 
            else if (*src == ' ' && !in_quote) {
                // Spacja poza cudzysłowem = koniec tego argumentu
                src++; // Przeskocz spację w źródle
                break; // Wyjdź z pętli kopiowania
            } 
            else {
                // Zwykły znak (lub spacja wewnątrz cudzysłowu) -> kopiuj
                *dst++ = *src++;
            }
        }
        *dst++ = '\0';
    }
    return arg_count;
}

int main() {
    char line[MAXLINE];
    char *args[MAXARGS];
    while (1) {
        // wypisujemy znak > i flushujemy od razu
        printf("> "); 
        fflush(stdout);
        // teraz wczytujemy linijke do '\n'
        if (fgets(line, sizeof(line), stdin) == NULL){
            ERR("fgets");
            break;
        } 
        // usuwamy \n
        remove_newline(line); 
        // dzielimy na argumenty
        int argc = tokenize(line, args);
        if(argc == 0) continue;
        // jesli jest polecenie exit to wyjdz
        if (strcmp(args[0], "exit") == 0) break;
        for(int i = 1; i < argc; i++) {
            printf("   [%d] '%s'\n", i, args[i]);
        }
        printf("\n");
    }
    return 0;
}