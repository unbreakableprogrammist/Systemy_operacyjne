/* Używamy _GNU_SOURCE, aby uzyskać dostęp do makra TEMP_FAILURE_RETRY z errno.h.
 * To makro ułatwia obsługę funkcji systemowych, które mogą zostać przerwane
 * przez sygnał (zwracając błąd EINTR).
 */
#define _GNU_SOURCE
#include <errno.h> // Dla TEMP_FAILURE_RETRY i errno
#include <fcntl.h> // Dla open() i flag O_...
#include <stdio.h> // Dla perror, fprintf, stderr
#include <stdlib.h> // Dla exit, EXIT_FAILURE, EXIT_SUCCESS
#include <string.h> // Dla (nieużywane w tym kodzie, ale często potrzebne)
#include <unistd.h> // Dla read, write, close, ssize_t

/**
 * @brief Makro do obsługi błędów.
 * Wypisuje komunikat o błędzie (source) oraz błąd systemowy (perror).
 * Dodatkowo wypisuje plik i linię, w której wystąpił błąd.
 * Kończy program z kodem błędu EXIT_FAILURE.
 * @param source Opisowy komunikat o źródle błędu (np. "open").
 */
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

// Definicja rozmiaru bufora używanego do odczytu i zapisu.
#define FILE_BUF_LEN 256

/**
 * @brief Wypisuje informację o poprawnym użyciu programu i kończy działanie.
 * @param pname Nazwa programu (zazwyczaj argv[0]).
 */
void usage(const char *const pname)
{
    fprintf(stderr, "USAGE:%s path_1 path_2\n", pname);
    exit(EXIT_FAILURE);
}

/**
 * @brief Funkcja pomocnicza do wczytywania danych (read) w sposób "hurtowy".
 * Gwarantuje, że funkcja przeczyta 'count' bajtów, chyba że wystąpi błąd
 * lub koniec pliku (EOF). Obsługuje częściowe odczyty oraz błąd EINTR.
 *
 * @param fd Deskryptor pliku, z którego czytamy.
 * @param buf Wskaźnik do bufora, do którego dane są wczytywane.
 * @param count Liczba bajtów do wczytania.
 * @return Całkowita liczba wczytanych bajtów (może być mniejsza niż 'count'
 * tylko w przypadku EOF) lub -1 w przypadku błędu.
 */
ssize_t bulk_read(int fd, char *buf, size_t count)
{
    ssize_t c;     // Liczba bajtów wczytana w pojedynczej operacji read().
    ssize_t len = 0; // Całkowita liczba wczytanych bajtów.
    do
    {
        /*
         * Próbujemy wczytać dane. Makro TEMP_FAILURE_RETRY automatycznie
         * ponawia próbę read(), jeśli zostanie ona przerwana przez sygnał (EINTR).
         */
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));

        if (c < 0) // Jeśli read() zwróciło błąd (inny niż EINTR)
            return c;  // Zwróć błąd (-1)

        if (c == 0) // Jeśli read() zwróciło 0, oznacza to koniec pliku (EOF)
            return len; // Zwróć dotychczas wczytaną liczbę bajtów

        buf += c;   // Przesuń wskaźnik bufora o liczbę wczytanych bajtów
        len += c;   // Zwiększ całkowitą liczbę wczytanych bajtów
        count -= c; // Zmniejsz liczbę bajtów pozostałych do wczytania
    } while (count > 0); // Pętla kontynuuje, dopóki nie wczytamy żądanej liczby bajtów
    return len; // Zwróć całkowitą wczytaną liczbę bajtów
}

/**
 * @brief Funkcja pomocnicza do zapisywania danych (write) w sposób "hurtowy".
 * Gwarantuje, że funkcja zapisze 'count' bajtów, chyba że wystąpi błąd.
 * Obsługuje częściowe zapisy oraz błąd EINTR.
 *
 * @param fd Deskryptor pliku, do którego piszemy.
 * @param buf Wskaźnik do bufora, z którego dane są zapisywane.
 * @param count Liczba bajtów do zapisania.
 * @return Całkowita liczba zapisanych bajtów (powinna być równa 'count')
 * lub -1 w przypadku błędu.
 */
ssize_t bulk_write(int fd, char *buf, size_t count)
{
    ssize_t c;     // Liczba bajtów zapisana w pojedynczej operacji write().
    ssize_t len = 0; // Całkowita liczba zapisanych bajtów.
    do
    {
        /*
         * Próbujemy zapisać dane. Makro TEMP_FAILURE_RETRY automatycznie
         * ponawia próbę write(), jeśli zostanie ona przerwana przez sygnał (EINTR).
         */
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));

        if (c < 0) // Jeśli write() zwróciło błąd (inny niż EINTR)
            return c;  // Zwróć błąd (-1)

        // Uwaga: write() zwracające 0 jest nietypowe, ale możliwe (np. pełny dysk).
        // W pętli jest to obsługiwane poprawnie (ponowienie próby).

        buf += c;   // Przesuń wskaźnik bufora o liczbę zapisanych bajtów
        len += c;   // Zwiększ całkowitą liczbę zapisanych bajtów
        count -= c; // Zmniejsz liczbę bajtów pozostałych do zapisania
    } while (count > 0); // Pętla kontynuuje, dopóki nie zapiszemy żądanej liczby bajtów
    return len; // Zwróć całkowitą zapisaną liczbę bajtów
}

int main(const int argc, const char *const *const argv)
{
    // Sprawdzenie, czy podano dokładnie dwa argumenty (ścieżki do plików)
    if (argc != 3)
        usage(argv[0]);

    const char *const path_1 = argv[1]; // Ścieżka do pliku źródłowego
    const char *const path_2 = argv[2]; // Ścieżka do pliku docelowego

    // Otworzenie pliku źródłowego (path_1) w trybie tylko do odczytu
    const int fd_1 = open(path_1, O_RDONLY);
    if (fd_1 == -1) // Obsługa błędu otwarcia
        ERR("open (source file)");

    /*
     * Otworzenie pliku docelowego (path_2) w trybie do zapisu (O_WRONLY).
     * O_CREAT: Utwórz plik, jeśli nie istnieje.
     * O_TRUNC: Jeśli plik istnieje, skasuj jego zawartość (nie ma go tutaj, ale często jest używane)
     * 0777: Uprawnienia dla nowego pliku (jeśli jest tworzony).
     * Są to bardzo szerokie uprawnienia (rwx dla wszystkich).
     * Bezpieczniej jest użyć np. 0666 lub 0644,
     * respektując umask systemu.
     */
    const int fd_2 = open(path_2, O_WRONLY | O_CREAT, 0777);
    if (fd_2 == -1) // Obsługa błędu otwarcia
        ERR("open (destination file)");

    // Bufor na dane kopiowane między plikami
    char file_buf[FILE_BUF_LEN];

    // Główna pętla kopiowania pliku
    for (;;)
    {
        // Wczytaj fragment danych (do FILE_BUF_LEN bajtów) z pliku źródłowego
        const ssize_t read_size = bulk_read(fd_1, file_buf, FILE_BUF_LEN);

        if (read_size == -1) // Obsługa błędu odczytu
            ERR("bulk_read");

        if (read_size == 0) // Jeśli wczytano 0 bajtów, to koniec pliku
            break; // Przerwij pętlę

        /*
         * Zapisz wczytane dane do pliku docelowego.
         * Ważne: zapisujemy dokładnie 'read_size' bajtów, czyli tyle,
         * ile faktycznie udało się wczytać (szczególnie istotne przy końcu pliku).
         */
        if (bulk_write(fd_2, file_buf, read_size) == -1)
            ERR("bulk_write");
    }

    // Zamknięcie pliku docelowego
    if (close(fd_2) == -1)
        ERR("close (destination file)");

    // Zamknięcie pliku źródłowego
    if (close(fd_1) == -1)
        ERR("close (source file)");

    // Zakończ program z sukcesem
    return EXIT_SUCCESS;
}