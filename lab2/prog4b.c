#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

/*
 * Globalna zmienna zliczająca liczbę odebranych sygnałów SIGUSR1.
 * sig_atomic_t + volatile -> bezpieczna do użycia w handlerze.
 */
volatile sig_atomic_t sig_count = 0;

/* ---------- PROTOTYPY ---------- */
void usage(char *name);
void handler(int sig);
void sigchld_handler(int sig);
void child_work(int m);
void parent_work(int b, int s, char *name);
ssize_t bulk_read(int fd, char *buf, size_t count);
ssize_t bulk_write(int fd, char *buf, size_t count);

/* ---------- FUNKCJE POMOCNICZE ---------- */

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s m b s name\n", name);
    fprintf(stderr,
            "m - number of 1/1000 milliseconds between signals [1,999]\n");
    fprintf(stderr, "b - number of blocks [1,999]\n");
    fprintf(stderr, "s - size of blocks [1,999] in MB\n");
    fprintf(stderr, "name - output file name\n");
    exit(EXIT_FAILURE);
}

/* Handler SIGUSR1 w procesie rodzica – tylko inkrementuje licznik. */
void handler(int sig)
{
    (void)sig;          // żeby kompilator się nie czepiał
    sig_count++;
}

/*
 * Handler SIGCHLD – sprząta zakończone procesy potomne (bez blokowania).
 * Dzięki temu unikamy zombie, jeśli dzieci kończą się wcześniej.
 */
void sigchld_handler(int sig)
{
    (void)sig;
    pid_t pid;

    while (1)
    {
        pid = waitpid(0, NULL, WNOHANG);
        if (pid == 0)
            return;                 // brak zakończonych dzieci
        if (pid < 0)
        {
            if (errno == ECHILD)    // nie ma już dzieci
                return;
            ERR("waitpid");
        }
        // jeśli pid > 0 – jedno dziecko zostało właśnie "zebrane"
    }
}

/*
 * Dziecko:
 *  - co m*(1/100000000) sekund (timespec {0, m*10000}) śpi,
 *  - po każdym takim "tiku" wysyła SIGUSR1 do swojego rodzica,
 *  - SIGUSR1 u dziecka ma być domyślny (SIG_DFL), żeby na końcu,
 *    gdy rodzic wyśle kill(0, SIGUSR1), dziecko zostało zabite.
 */
void child_work(int m)
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = m * 10000;  // tak jak w oryginalnym programie

    /* Ustawiamy SIGUSR1 na domyślne działanie TYLKO w dziecku. */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;
    if (sigaction(SIGUSR1, &sa, NULL) < 0)
        ERR("sigaction SIGUSR1 in child");

    while (1)
    {
        nanosleep(&t, NULL);
        /* Wysyłamy SIGUSR1 do rodzica (getppid()). */
        if (kill(getppid(), SIGUSR1) < 0)
            ERR("kill from child");
    }
}

/*
 * bulk_read i bulk_write – "bezpieczne" czytanie/zapis dużych bloków,
 * dopóki nie przeczytamy/zapiszemy żądanej liczby bajtów lub nie będzie EOF.
 */

ssize_t bulk_read(int fd, char *buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;

    do
    {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;               // błąd read

        if (c == 0)
            return len;             // EOF

        buf   += c;
        len   += c;
        count -= c;
    } while (count > 0);

    return len;
}

ssize_t bulk_write(int fd, char *buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;

    do
    {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return c;               // błąd write

        buf   += c;
        len   += c;
        count -= c;
    } while (count > 0);

    return len;
}

/*
 * Rodzic:
 *  - otwiera /dev/urandom do czytania,
 *  - otwiera plik wyjściowy do pisania,
 *  - b razy:
 *      - wczytuje s bajtów z /dev/urandom (bulk_read),
 *      - zapisuje s bajtów do pliku (bulk_write),
 *      - wypisuje na stderr ile bajtów przeniósł i ile sygnałów SIGUSR1 otrzymał.
 *  - na końcu:
 *      - zamyka pliki,
 *      - free(buf),
 *      - wysyła SIGUSR1 do całej grupy procesów (kill(0, SIGUSR1)),
 *        co zabije dziecko (dzieci), bo u nich SIGUSR1 ma akcję domyślną.
 */
void parent_work(int b, int s, char *name)
{
    int in, out;
    ssize_t count;
    char *buf = malloc(s);
    if (!buf)
        ERR("malloc");

    if ((out = TEMP_FAILURE_RETRY(open(name, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0777))) < 0)
        ERR("open output");

    if ((in = TEMP_FAILURE_RETRY(open("/dev/urandom", O_RDONLY))) < 0)
        ERR("open /dev/urandom");

    for (int i = 0; i < b; i++)
    {
        if ((count = bulk_read(in, buf, s)) < 0)
            ERR("bulk_read");

        if ((count = bulk_write(out, buf, count)) < 0)
            ERR("bulk_write");

        if (fprintf(stderr, "Block of %ld bytes transferred. Signals RX:%d\n",
                    (long)count, sig_count) < 0)
            ERR("fprintf");
    }

    if (TEMP_FAILURE_RETRY(close(in)) < 0)
        ERR("close in");
    if (TEMP_FAILURE_RETRY(close(out)) < 0)
        ERR("close out");

    free(buf);

    /* Na końcu wysyłamy SIGUSR1 do całej grupy procesów.
     *  - rodzic ma handler -> nic mu się nie stanie (tylko by wzrósł sig_count),
     *  - dziecko ma SIGUSR1 = SIG_DFL -> proces zostanie zakończony.
     */
    if (kill(0, SIGUSR1) < 0)
        ERR("kill group SIGUSR1");
}

/* ---------- MAIN ---------- */

int main(int argc, char **argv)
{
    int m, b, s;
    char *name;

    if (argc != 5)
        usage(argv[0]);

    m = atoi(argv[1]);   // przerwa (w "tikach") między SIGUSR1 wysyłanymi do rodzica
    b = atoi(argv[2]);   // liczba bloków
    s = atoi(argv[3]);   // rozmiar bloku w MB
    name = argv[4];      // nazwa pliku wyjściowego

    if (m <= 0 || m > 999 || b <= 0 || b > 999 || s <= 0 || s > 999)
        usage(argv[0]);

    /* Ustawiamy sigaction dla SIGUSR1 – w rodzicu: handler zlicza sygnały.
     * Dziecko odziedziczy tę akcję, ale my w child_work nadpiszemy SIGUSR1 na SIG_DFL.
     */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa, NULL) < 0)
        ERR("sigaction SIGUSR1");

    /* SIGCHLD – sprzątanie dzieci bez zombie. */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGCHLD, &sa, NULL) < 0)
        ERR("sigaction SIGCHLD");

    pid_t pid = fork();
    if (pid < 0)
        ERR("fork");

    if (pid == 0)
    {
        /* Kod dziecka */
        child_work(m);
        /* child_work nigdy nie wraca, ale dla porządku: */
        exit(EXIT_SUCCESS);
    }
    else
    {
        /* Kod rodzica */
        parent_work(b, s * 1024 * 1024, name);

        /* Dodatkowe zabezpieczenie – jeśli coś przeżyło, zbieramy dzieci. */
        while (wait(NULL) > 0)
        {
        }
    }

    return EXIT_SUCCESS;
}
