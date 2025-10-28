#include <dirent.h>     // Do obsługi katalogów (opendir, readdir, closedir)
#include <stdio.h>      // Standardowe wejście/wyjście (printf, fprintf, stderr)
#include <unistd.h>     // Funkcje POSIX (getopt)
#include <stdlib.h>     // Standardowe funkcje (exit, atoi, calloc, free, getenv)
#include <fcntl.h>      // Do operacji na plikach (choć w tym kodzie nieużywane)
#include <sys/stat.h>   // Do pobierania statystyk plików (lstat, struct stat, S_ISDIR)
#include <string.h>     // Do operacji na stringach (strcpy, strcat, strcmp, strchr)
#include <errno.h>      // Do obsługi kodów błędów (errno)

// Makro do obsługi błędów (takie samo jak w poprzednim zadaniu)
// Wypisuje błąd systemowy (perror), nazwę pliku, linię i kończy program.
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

// Stałe definiujące limity
#define MAX_DIR_INPUT 10   // Maksymalna liczba katalogów z flagi -p
#define MAX_EXT 10         // (Nieużywane) Maksymalna długość rozszerzenia?
#define MAX_SUBDIRS 50     // Maksymalna liczba podkatalogów w jednym katalogu (do rekursji)
#define MAX_DIR_LEN 200    // Maksymalna długość ścieżki katalogu

/**
 * Główna funkcja listująca katalogi.
 * @param path Ścieżka do katalogu do wylistowania.
 * @param ext Wskaźnik na string z rozszerzeniem (NULL jeśli nie filtrować).
 * @param depth Głębokość rekursji (ile poziomów w dół).
 */
void list_dir(char* path, char* ext, int depth)
{
    // Warunek stopu rekursji (Etap 3). Jeśli depth spadnie do 0, nie wchodź głębiej.
    if (depth == 0)
        return;

    DIR* dp = opendir(path); // Otwórz strumień katalogu
    if (dp == NULL)          // Sprawdź błąd otwarcia
        ERR("opendir");       // Zakończ program, jeśli nie można otworzyć katalogu

    // Wypisz nagłówek "path:" zgodnie z formatem wyjścia
    printf("path: %s\n", path);

    errno = 0; // Zresetuj globalną zmienną errno przed użyciem readdir
    struct dirent* de = readdir(dp); // Wczytaj pierwszą pozycję z katalogu
    struct stat filestat;            // Struktura do przechowywania informacji o pliku
    
    // Tablica do przechowywania ścieżek podkatalogów, które odwiedzimy rekursyjnie
    char subdirs[MAX_SUBDIRS][MAX_DIR_LEN];
    int i = 0; // Licznik znalezionych podkatalogów

    while (de != NULL) // Pętla przez wszystkie pozycje w katalogu
    {
        // Alokacja pamięci na pełną ścieżkę pliku (path + "/" + d_name + '\0')
        char* temp = (char*)calloc(strlen(path) + 2 + strlen(de->d_name), sizeof(char));
        if (temp == NULL)
            ERR("malloc"); // Błąd alokacji pamięci

        // Składanie pełnej ścieżki
        strcpy(temp, path);
        strcat(temp, "/");
        strcat(temp, de->d_name);

        // Użyj lstat(), aby pobrać informacje o pliku (poprawnie obsługuje linki symboliczne)
        if (lstat(temp, &filestat) == -1)
            ERR("lstat"); // Błąd przy pobieraniu statystyk

        errno = 0; // Zresetuj errno
        free(temp); // Zwolnij pamięć zaalokowaną na 'temp' (już nie jest potrzebna)

        // BŁĄD LOGICZNY: Sprawdzanie errno bezpośrednio po free() jest bezcelowe.
        // free() nie ustawia errno w przypadku sukcesu. To sprawdzenie niczego nie wnosi.
        if (errno != 0)
            ERR("free");

        // Znajdź wskaźnik na *pierwsze* wystąpienie kropki w nazwie pliku
        // UWAGA: Dla plików "plik.tar.gz" i rozszerzenia "gz", to nie zadziała poprawnie.
        // Powinno się użyć strrchr() (ostatnia kropka).
        char* fext = strchr(de->d_name, '.');

        // Sprawdź, czy pozycja jest katalogiem
        if (S_ISDIR(filestat.st_mode))
        {
            // Pomiń specjalne katalogi "." (bieżący) i ".." (nadrzędny)
            if (strcmp(de->d_name, "..") == 0 || strcmp(de->d_name, ".") == 0)
            {
                de = readdir(dp); // Wczytaj następną pozycję
                continue; // Pomiń resztę pętli i idź do następnej iteracji
            }
            
            // Jeśli to "prawdziwy" podkatalog, zapisz jego ścieżkę do tablicy 'subdirs'
            strcpy(subdirs[i], path);
            strcat(subdirs[i], "/");
            strcat(subdirs[i++], de->d_name); // Zapisz ścieżkę i zwiększ licznik 'i'

            // Zabezpieczenie przed przepełnieniem tablicy 'subdirs'
            if (i == MAX_SUBDIRS - 1)
                break; // Przerwij pętlę while (nie optymalne, bo nie przetworzy reszty plików)
        }

        // Warunek filtrowania (Etap 2)
        // Wypisz, jeśli:
        // 1. Nie podano rozszerzenia (ext == NULL)
        // LUB
        // 2. Znaleziono kropkę (fext != NULL) ORAZ tekst PO kropce (fext+1) jest taki sam jak 'ext'
        if (ext == NULL || (fext != NULL && strcmp(fext + 1, ext) == 0))
        {
            // BŁĄD: Ten warunek wypisze *wszystko*, co pasuje do rozszerzenia,
            // włącznie z katalogami (np. jeśli mamy katalog `logs.txt`).
            // Powinno być dodatkowe sprawdzenie `!S_ISDIR(filestat.st_mode)`.
            printf("%s %ld\n", de->d_name, filestat.st_size);
        }

        de = readdir(dp); // Wczytaj następną pozycję z katalogu
    }

    // Sprawdź, czy pętla while zakończyła się z powodu błędu readdir
    if (errno != 0)
        ERR("readdir");
    
    if (closedir(dp)) // Zamknij strumień katalogu
        ERR("closedir");

    // BŁĄD: Całkowicie brakuje implementacji rekursji (Etap 3)!
    // Po zamknięciu katalogu powinna być pętla 'for' przechodząca po tablicy 'subdirs'
    // i wywołująca 'list_dir' dla każdego podkatalogu ze zmniejszoną głębokością:
    //
    // for (int j = 0; j < i; j++) {
    //     list_dir(subdirs[j], ext, depth - 1);
    // }
}

int main(int argc, char** argv)
{
    int c; // Zmienna do przechowywania opcji zwróconej przez getopt
    char* dirs[MAX_DIR_INPUT]; // Tablica wskaźników na ścieżki katalogów (-p)
    char* ext = NULL; // Wskaźnik na string z rozszerzeniem (-e)
    int d = 1; // Głębokość przeszukiwania (-d), domyślnie 1
    int i = 0; // Licznik, ile ścieżek -p zostało podanych

    // BŁĄD: Brakuje obsługi flagi '-o' (Etap 4) w stringu getopt.
    // Powinno być "p:e:d:o" (ponieważ -o nie przyjmuje argumentu).
    while ((c = getopt(argc, argv, "p:e:d:")) != -1)
    {
        switch (c)
        {
        case 'p': // Znaleziono opcję -p
            dirs[i++] = optarg; // Zapisz wskaźnik na argument (ścieżkę) do tablicy
            break;
        case 'e': // Znaleziono opcję -e
            ext = optarg; // Zapisz wskaźnik na argument (rozszerzenie)
            break;
        case 'd': // Znaleziono opcję -d
            d = atoi(optarg); // Konwertuj argument (głębokość) na liczbę całkowitą
            break;
        default: // Nieznana opcja lub brak argumentu dla opcji, która go wymaga
            ERR("getopt");
        }
    }

    // BŁĄD: Całkowicie brakuje implementacji Etapu 4 (flaga -o).
    // Program powinien sprawdzić, czy flaga -o została ustawiona,
    // następnie odczytać zmienną środowiskową L1_OUTPUTFILE (za pomocą getenv),
    // a na końcu przekierować standardowe wyjście (stdout) do tego pliku
    // (np. używając freopen() lub dup2() z open()).

    // Pętla po wszystkich zebranych ścieżkach -p
    for (int j = 0; j < i; j++)
    {
        // Wywołaj funkcję listującą dla każdej ścieżki
        list_dir(dirs[j], ext, d);
    }
    
    return EXIT_SUCCESS; // Zakończ program pomyślnie
}