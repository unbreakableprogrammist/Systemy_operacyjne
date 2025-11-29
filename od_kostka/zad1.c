// Definiuje makro _GNU_SOURCE, które włącza dodatkowe, niestandardowe funkcje GNU
// (np. TEMP_FAILURE_RETRY, choć w tym kodzie nie jest jawnie używane,
// ale jest zalecane przy korzystaniu z funkcji POSIX).
#define _GNU_SOURCE
#include <dirent.h>     // Do obsługi katalogów (opendir, readdir, closedir)
#include <stdio.h>      // Standardowe wejście/wyjście (printf, fopen, fprintf, stderr)
#include <unistd.h>     // Funkcje POSIX (getopt, read, write, close)
#include <stdlib.h>     // Standardowe funkcje (exit, EXIT_FAILURE, EXIT_SUCCESS)
#include <fcntl.h>      // Opcje kontroli plików (open, O_WRONLY, O_APPEND, O_CREAT)
#include <sys/stat.h>   // Do pobierania statystyk plików (lstat, struct stat)
#include <errno.h>      // Do obsługi kodów błędów (errno)
#include <string.h>     // Do operacji na stringach (strcat, strlen, strcpy)

// Makro do obsługi błędów.
// source to nazwa funkcji, która zawiodła (np. "opendir")
// perror(source) wypisze komunikat błędu systemowego (np. "opendir: No such file or directory")
// fprintf(...) wypisze nazwę pliku i numer linii, gdzie wystąpił błąd
// exit(EXIT_FAILURE) natychmiast zakończy program z kodem błędu
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

// Stałe definiujące maksymalne rozmiary
#define MAX_PATH_LEN 256 // Maksymalna długość ścieżki pliku
#define MAX_FILES 20     // Maksymalna liczba katalogów do przetworzenia (z parametrów -p)
#define MAX_BUF 128      // Rozmiar bufora do zapisu/odczytu

// Funkcja pomocnicza do wczytywania danych z deskryptora pliku (fd)
// Gwarantuje, że odczyta DOKŁADNIE 'nbytes' bajtów, chyba że wystąpi błąd lub EOF.
// Obsługuje "krótkie odczyty" (short reads), ponawiając operację read.
// UWAGA: Funkcja 'TEMP_FAILURE_RETRY' jest zwykle używana do automatycznego ponawiania
// operacji przerwanej przez sygnał (EINTR). W tym kodzie jej brakuje,
// ale funkcjonalność pętli 'do-while' implementuje logikę "czytaj, aż zapełnisz bufor".
ssize_t buff_read(int fd, char* buf, size_t nbytes)
{
    ssize_t len = 0; // Licznik całkowitej liczby wczytanych bajtów
    ssize_t c;       // Licznik bajtów wczytanych w pojedynczym wywołaniu read()

    do
    {
        // Próba odczytania danych. Powinno być: c = TEMP_FAILURE_RETRY(read(fd, buf, nbytes));
        // ale w kodzie jest samo read. TEMP_FAILURE_RETRY jest zdefiniowane w unistd.h po _GNU_SOURCE.
        c = TEMP_FAILURE_RETRY(read(fd, buf, nbytes)); // Czytaj 'nbytes' do bufora 'buf'
        if (c == 0) // Jeśli read zwróci 0, oznacza to koniec pliku (EOF)
            return len; // Zwróć liczbę bajtów wczytanych do tej pory
        if (c < 0)  // Jeśli read zwróci < 0, oznacza to błąd
            return c;  // Zwróć kod błędu

        // Jeśli odczyt się powiódł (c > 0)
        buf += c;    // Przesuń wskaźnik bufora o 'c' bajtów
        len += c;    // Dodaj wczytane bajty do sumy
        nbytes -= c; // Zmniejsz liczbę bajtów do wczytania
    } while (nbytes > 0); // Pętla działa, dopóki nie wczytamy żądanej liczby bajtów
    return len; // Zwróć całkowitą liczbę wczytanych bajtów
}

// Funkcja pomocnicza do zapisu danych do deskryptora pliku (fd)
// Gwarantuje, że zapisze DOKŁADNIE 'nbytes' bajtów, chyba że wystąpi błąd.
// Obsługuje "krótkie zapisy" (short writes).
ssize_t buff_write(int fd, char* buf, size_t nbytes)
{
    ssize_t len = 0; // Licznik całkowitej liczby zapisanych bajtów
    ssize_t c;       // Licznik bajtów zapisanych w pojedynczym wywołaniu write()

    do
    {
        // Próba zapisu danych. Analogicznie do read, powinno być TEMP_FAILURE_RETRY.
        c = write(fd, buf, nbytes); // Zapisz 'nbytes' z bufora 'buf'
        if (c < 0)  // Jeśli write zwróci < 0, oznacza to błąd
            return c;  // Zwróć kod błędu
        
        // BŁĄD LOGICZNY: Ten warunek jest niemożliwy.
        // write() nigdy nie zwróci więcej bajtów niż 'nbytes'.
        // Prawdopodobnie miało to być inne sprawdzenie.
        if (c > nbytes)
            ERR("write");

        // Jeśli zapis się powiódł (c >= 0)
        buf += c;    // Przesuń wskaźnik bufora o 'c' bajtów (dane, które zostały zapisane)
        len += c;    // Dodaj zapisane bajty do sumy
        nbytes -= c; // Zmniejsz liczbę bajtów pozostałych do zapisu
    } while (nbytes > 0); // Pętla działa, dopóki nie zapiszemy wszystkiego
    return len; // Zwróć całkowitą liczbę zapisanych bajtów
}

// Funkcja listująca zawartość katalogu (Wersja 1: używa wysokopoziomowego I/O: FILE*)
// Ta funkcja nie jest ostatecznie używana w main()
void list_files(char* path, const char* out_path)
{
    DIR* dir = opendir(path); // Otwórz strumień katalogu
    if (dir == NULL)          // Sprawdź błąd otwarcia
        ERR("opendir");       // Zakończ program, jeśli nie można otworzyć katalogu

    if (out_path == NULL) // Sprawdź, czy ścieżka wyjściowa NIE została podana
    {
        // Jeśli nie, wypisz na standardowe wyjście (stdout)
        printf("SCIEZKA: \n");
        printf("%s\n", path);
        printf("LISTA PLIKOW: \n");

        struct dirent* de = readdir(dir); // Wczytaj pierwszą pozycję z katalogu
        struct stat filestat;            // Struktura do przechowywania informacji o pliku
        char full_path[MAX_PATH_LEN];    // Bufor na pełną ścieżkę (katalog + nazwa pliku)
        
        // BŁĄD: Bufor 'full_path' nie został zainicjowany przed pierwszym użyciem!
        // Pierwsze wywołanie strcat() będzie pisać na "śmieciach" z pamięci stosu.
        
        while (de != NULL) { // Pętla przez wszystkie pozycje w katalogu
            // BŁĄD: Ta linia powinna być `strcpy(full_path, path);`
            // lub `full_path[0] = '\0';` powinno być PRZED pętlą lub NA POCZĄTKU pętli.
            strcat(full_path, path); // Dołącz ścieżkę katalogu
            strcat(full_path, "/");  // Dołącz separator 
            strcat(full_path, de->d_name); // Dołącz nazwę pliku

            // Użyj lstat(), aby pobrać informacje o pliku (poprawnie obsługuje linki symboliczne)
            if (lstat(full_path, &filestat) == -1)
                ERR("lstat"); // Błąd przy pobieraniu statystyk

            // Wypisz nazwę pliku i jego rozmiar (%jd to format dla intmax_t, bezpieczny dla st_size)
            printf("%s  %jd\n", de->d_name, filestat.st_size);

            de = readdir(dir); // Wczytaj następną pozycję
            full_path[0] = '\0'; // Wyczyść bufor ścieżki (ale dopiero na końcu pętli)
        }
    }
    else // Jeśli ścieżka wyjściowa ZOSTAŁA podana
    {
        // Otwórz plik wyjściowy w trybie "a+" (append - dopisywanie; utwórz, jeśli nie istnieje)
        FILE* f = fopen(out_path, "a+");
        if (f == NULL)
            ERR("fopen"); // Błąd otwarcia pliku

        // Wypisz nagłówki do pliku
        fprintf(f, "SCIEZKA: \n");
        fprintf(f, "%s\n", path);
        fprintf(f, "LISTA PLIKOW: \n");

        struct dirent* de = readdir(dir); // Wczytaj pierwszą pozycję
        struct stat filestat;
        char full_path[MAX_PATH_LEN];

        // BŁĄD: Ten sam błąd co powyżej - 'full_path' jest niezainicjowany.
        while (de != NULL) { // Pętla przez wszystkie pozycje
            // BŁĄD: Składanie ścieżki na niezainicjowanym buforze
            strcat(full_path, path);
            strcat(full_path, "/");
            strcat(full_path, de->d_name);

            if (lstat(full_path, &filestat) == -1) // Pobierz statystyki
                ERR("lstat");

            // Wypisz dane do pliku
            fprintf(f, "%s  %jd\n", de->d_name, filestat.st_size);
            
            de = readdir(dir);   // Następna pozycja
            full_path[0] = '\0'; // Wyczyść bufor
        }

        if (fclose(f) == -1) // Zamknij plik
            ERR("fclose");
    }

    if (closedir(dir) == -1) // Zamknij strumień katalogu
        ERR("closedir");
}

// Funkcja listująca zawartość katalogu (Wersja 2: używa niskopoziomowego I/O: int fd)
// To jest wersja faktycznie używana przez main().
void list_files_v2(char* path, const char* out_path)
{
    DIR* dir = opendir(path); // Otwórz strumień katalogu
    
    // UWAGA: Zgodnie z "Etapem 6" zadania, ten błąd powinien być wypisany na stderr,
    // a program powinien kontynuować. Makro ERR() kończy program.
    if (dir == NULL)
        ERR("opendir"); // Błąd otwarcia katalogu

    if (out_path == NULL) // Jeśli nie ma pliku wyjściowego, pisz na stdout
    {
        // Ta część jest identyczna jak w list_files (Wersja 1)
        printf("SCIEZKA: \n");
        printf("%s\n", path);
        printf("LISTA PLIKOW: \n");
        struct dirent* de = readdir(dir);
        struct stat filestat;
        char full_path[MAX_PATH_LEN];
        
        // BŁĄD: Ten sam błąd co powyżej - 'full_path' jest niezainicjowany.
        while (de != NULL) {
            // BŁĄD: Składanie ścieżki na niezainicjowanym buforze
            strcat(full_path, path);
            strcat(full_path, "/");
            strcat(full_path, de->d_name);

            // UWAGA: Jak wyżej, ERR() zakończy program zamiast kontynuować.
            if (lstat(full_path, &filestat) == -1)
                ERR("lstat");
            
            printf("%s  %jd\n", de->d_name, filestat.st_size);
            de = readdir(dir);
            full_path[0] = '\0'; // Reset bufora
        }
    }
    else // Jeśli podano plik wyjściowy
    {
        // Otwórz plik używając niskopoziomowej funkcji open()
        // O_WRONLY: Tylko do zapisu
        // O_APPEND: Dopisywanie na końcu pliku
        // O_CREAT: Utwórz plik, jeśli nie istnieje
        // 0666: Prawa dostępu dla nowego pliku (rw-rw-rw-)
        int fd = open(out_path, O_WRONLY | O_APPEND | O_CREAT, 0666);
        if (fd < 0)
            ERR("open"); // Błąd otwarcia pliku

        char buf[MAX_BUF]; // Bufor do formatowania tekstu przed zapisem

        // BŁĄD KRYTYCZNY: Bufor 'buf' jest niezainicjowany.
        // Pierwszy strcat() pisze w losowe miejsce pamięci.
        // Powinno być: `strcpy(buf, "SCIEZKA: \n");`
        strcat(buf, "SCIEZKA: \n");
        strcat(buf, path); // Dołączanie ścieżki do "śmieci"
        strcat(buf, "\nLISTA PLIKOW: \n"); // Dołączanie reszty

        // Zapisz zbuforowany nagłówek do pliku za pomocą naszej pomocniczej funkcji
        ssize_t bytes_written = buff_write(fd, buf, strlen(buf));
        
        buf[0] = '\0'; // Wyczyść bufor (za późno, błąd już wystąpił)
        
        if (bytes_written < 0) // Sprawdź błąd zapisu
            ERR("write");

        struct dirent* de = readdir(dir); // Wczytaj pierwszą pozycję katalogu
        struct stat filestat;
        char full_path[MAX_PATH_LEN];

        // BŁĄD: Ten sam błąd co powyżej - 'full_path' jest niezainicjowany.
        while (de != NULL) {
            // BŁĄD: Składanie ścieżki na niezainicjowanym buforze
            strcat(full_path, path);
            strcat(full_path, "/");
            strcat(full_path, de->d_name);
            
            if (lstat(full_path, &filestat) == -1)
                ERR("lstat");

            // Bufor tymczasowy na sformatowaną linię (nazwa + rozmiar)
            char temp_buf[MAX_BUF];
            
            // Sformatuj linię do bufora
            // UWAGA: Użycie %ld dla st_size (off_t) jest mniej przenośne niż %jd.
            // W wersji 1 (list_files) użyto poprawnie %jd.
            sprintf(temp_buf, "%s %ld\n", de->d_name, filestat.st_size);
            
            // Zapisz sformatowaną linię do pliku
            bytes_written = buff_write(fd, temp_buf, strlen(temp_buf));
            if (bytes_written < 0)
                ERR("write");

            de = readdir(dir); // Wczytaj następną pozycję
            full_path[0] = '\0'; // Wyczyść bufor ścieżki
            
            // Ta linia jest zbędna, ponieważ temp_buf jest deklarowany
            // na nowo w każdej iteracji pętli (zmienna lokalna na stosie).
            temp_buf[0] = '\0'; 
        }

        if (close(fd) == -1) // Zamknij deskryptor pliku
            ERR("close");
    }
    if (closedir(dir) == -1) // Zamknij strumień katalogu
        ERR("closedir");
}

// Główna funkcja programu
int main(int argc, char** argv)
{
    int c; // Zmienna do przechowywania opcji zwróconej przez getopt
    char* out_path = NULL; // Wskaźnik na ścieżkę pliku wyjściowego, domyślnie NULL (stdout)
    
    // Tablica wskaźników na ścieżki katalogów podane z flagą -p
    char* files_to_list[MAX_FILES];
    int i = 0; // Licznik, ile ścieżek -p zostało podanych

    // Pętla getopt do parsowania argumentów linii poleceń
    // "p:o:" oznacza, że oczekujemy opcji -p z argumentem i opcji -o z argumentem
    while ((c = getopt(argc, argv, "p:o:")) != -1)
    {
        if (c == 'p') // Jeśli znaleziono opcję -p
        {
            // Zapisz wskaźnik na argument (optarg) do tablicy
            // i zwiększ licznik.
            // UWAGA: Program się wysypie, jeśli podamy więcej niż MAX_FILES (20) flag -p
            files_to_list[i] = optarg;
            i++;
        }
        if (c == 'o') // Jeśli znaleziono opcję -o
        {
            if (out_path != NULL) // Sprawdź, czy -o nie zostało już podane
            {
                // Błąd: podano więcej niż jedno -o
                fprintf(stderr, "Wrong Parameters: -o can be specified only once\n");
                exit(EXIT_FAILURE); // Zakończ program
            }
            out_path = optarg; // Zapisz ścieżkę pliku wyjściowego
        }
        if (c == '?') // Jeśli getopt zwróci '?' (nieznana opcja lub brak argumentu)
        {
            // Błąd: nieobsługiwane parametry
            fprintf(stderr, "Wrong Parameters: unknown option or missing argument\n");
            // TODO: Zgodnie z "Etapem 6" tutaj powinno być wypisane "usage"
            exit(EXIT_FAILURE); // Zakończ program
        }
    }
    
    // TODO: Zgodnie z "Etapem 6" program powinien sprawdzić, czy są
    // dodatkowe argumenty pozycyjne (np. `./program /etc` zamiast `./program -p /etc`)
    // Można to zrobić sprawdzając, czy `optind < argc`.
    // Jeśli tak, należy wypisać "usage" i zakończyć.

    // Ta linia jest niepotrzebna, ponieważ 'i' przechowuje dokładną liczbę podanych ścieżek
    // files_to_list[i] = '\0'; 

    // Pętla po wszystkich zebranych ścieżkach -p
    for (int j = 0; j < i; j++)
    {
        // Wywołaj funkcję listującą (wersję 2, niskopoziomową)
        // dla każdej ścieżki
        list_files_v2(files_to_list[j], out_path);
    }
    
    // Jeśli nie podano żadnych ścieżek -p (i == 0), program nic nie zrobi.
    // Zgodnie z "Etapem 1" powinien był wylistować katalog roboczy.
    // Ta funkcjonalność z "Etapu 1" została utracona w "Etapie 3".

    return EXIT_SUCCESS; // Zakończ program pomyślnie
}