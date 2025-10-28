#include <dirent.h>     // Do obsługi katalogów (opendir, readdir, closedir, mkdir)
#include <stdio.h>      // Standardowe wejście/wyjście (printf, fopen, fprintf, stderr, fscanf)
#include <unistd.h>     // Funkcje POSIX (getopt, unlink, close)
#include <stdlib.h>     // Standardowe funkcje (exit, calloc, malloc, free, realloc)
#include <fcntl.h>      // Do operacji na plikach (open, O_CREAT)
#include <sys/stat.h>   // Do pobierania statystyk plików (lstat, struct stat)
#include <stdbool.h>    // Do używania typu bool (true, false)
#include <string.h>     // Do operacji na stringach (strcpy, strcat, strcmp, strlen)
#include <errno.h>      // Do obsługi kodów błędów (errno, EEXIST)

// Stałe
#define MAX_VENV_UPDATES 10 // Maksymalna liczba środowisk podanych flagą -v
// Makro do obsługi błędów
#define ERR(source) (perror(source),fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

/**
 * Usuwa pakiet ze środowiska.
 * @param venv Nazwa środowiska.
 * @param package Nazwa pakietu do usunięcia.
 *
 * UWAGA: Ta funkcja jest wywoływana w main() z błędem.
 * Otrzymuje 'package' jako "nazwa==wersja" zamiast "nazwa".
 */
void remove_package(char* venv, char* package)
{
    // Alokacja pamięci na ścieżkę do pliku pakietu: "./<venv>/<package>"
    char* full_env_path = (char*)calloc(strlen(venv) + 4 + strlen(package), sizeof(char));
    if (full_env_path == NULL)
        ERR("calloc");
    
    // Budowanie ścieżki
    strcpy(full_env_path, "./");
    strcat(full_env_path, venv);
    strcat(full_env_path, "/");
    strcat(full_env_path, package); // BŁĄD: Tutaj 'package' to np. "numpy==1.0.0" zamiast "numpy"

    // Sprawdzenie, czy plik pakietu istnieje (czy pakiet jest "zainstalowany")
    FILE* fp_pack = fopen(full_env_path, "r");
    if (fp_pack == NULL)
    {
        // Jeśli plik nie istnieje, pakiet nie jest zainstalowany
        fprintf(stderr, "package doesn't exist");
        // BŁĄD: Zgodnie z treścią zadania, błąd powinien wstrzymać program.
        // Ta funkcja powinna zwracać błąd do main(), a nie robić exit().
        exit(EXIT_FAILURE);
    }
    if (fclose(fp_pack) == -1)
        ERR("fclose");

    // Usunięcie pliku pakietu
    if (unlink(full_env_path) == -1)
        ERR("unlink");
    
    free(full_env_path); // Zwolnienie pamięci
    full_env_path = NULL;

    // Alokacja pamięci na ścieżkę do pliku requirements: "./<venv>/requirements.txt"
    char* req_path = (char*)calloc(strlen(venv) + 3 + strlen("/requirements.txt"), sizeof(char));
    if (req_path == NULL)
        ERR("calloc");
    
    // Budowanie ścieżki
    strcpy(req_path, "./");
    strcat(req_path, venv);
    strcat(req_path, "/requirements.txt");

    // Otwarcie pliku requirements do CZYTANIA
    FILE* fp_req = fopen(req_path, "r");
    struct stat stat_st;
    // Pobranie statystyk pliku (głównie rozmiaru)
    if (stat(req_path, &stat_st) == -1)
        ERR("stat");

    // Bufory na odczytaną nazwę pakietu (f) i wersję (v)
    char f[40];
    char v[20];
    
    // Alokacja bufora na *nową* zawartość pliku requirements
    char* buf = (char*)malloc(stat_st.st_size);
    // BŁĄD KRYTYCZNY: Zaalokowany bufor 'buf' nie jest zainicjowany!
    // Pierwsze wywołanie strcat() poniżej będzie pisać na "śmieciach" z pamięci.
    // Powinno być: `calloc(stat_st.st_size + 1, sizeof(char))` lub `buf[0] = '\0';`

    // Pętla wczytująca plik requirements linia po linii (format "nazwa wersja")
    while (fscanf(fp_req, "%s %s", f, v) == 2)
    {
        // Jeśli wczytana nazwa (f) jest INNA niż pakiet do usunięcia...
        if (strcmp(f, package) != 0) // BŁĄD: 'package' to "nazwa==wersja", a 'f' to "nazwa". Zawsze będą inne.
        {
            // ...dopisz tę linię do bufora 'buf'
            strcat(buf, f);
            strcat(buf, " ");
            strcat(buf, v);
            strcat(buf, "\n");
        }
    }
    if (fclose(fp_req) == -1) // Zamknij plik (tryb 'r')
        ERR("fclose");

    // Otwórz ten sam plik requirements, ale w trybie "w" (PISANIE + obcięcie pliku do zera)
    FILE* fp_req2 = fopen(req_path, "w");
    fprintf(fp_req2, buf); // Zapisz przefiltrowaną zawartość z bufora do pliku
    
    free(buf); // Zwolnij bufor
    free(req_path); // Zwolnij ścieżkę
    
    if (fclose(fp_req2) == -1) // Zamknij plik (tryb 'w')
        ERR("fclose");
}

/**
 * Wyciąga nazwę pakietu ze stringa "nazwa==wersja".
 * @param package String w formacie "nazwa==wersja" lub "nazwa".
 * @return Wskaźnik na nowo zaalokowany string z samą nazwą ("nazwa").
 */
char* get_package(char* package)
{
    // Alokuje 100 bajtów na start (więcej niż potrzeba)
    char* name = (char*)calloc(100, sizeof(char));
    if (name == NULL)
        ERR("calloc");
    int i = 0;
    while (package[i] != '\0') // Pętla po stringu wejściowym
    {
        if (package[i] == '=') // Jeśli znajdzie '=', przerwij kopiowanie
        {
            break;
        }
        name[i] = package[i]; // Kopiuj znak
        i++;
    }
    // Zmniejsz bufor do dokładnie wymaganego rozmiaru (i + 1 na znak null '\0')
    name = (char*)realloc(name, 1 + i * sizeof(char));
    if (name == NULL)
        ERR("realloc");
    return name;
}

/**
 * Konwertuje string "nazwa==wersja" na "nazwa wersja".
 * @param package String w formacie "nazwa==wersja".
 * @return Wskaźnik na nowo zaalokowany string ("nazwa wersja").
 */
char* get_package_line(char* package)
{
    char* name = (char*)calloc(100, sizeof(char)); // Bufor 100 B
    if (name == NULL)
        ERR("calloc");
    int i = 0; // Indeks dla stringu wejściowego 'package'
    int j = 0; // Indeks dla stringu wyjściowego 'name'
    
    while (package[i] != '\0') // Pętla po stringu wejściowym
    {
        if (package[i] == '=') // Jeśli znaleziono '='
        {
            // Pomija ewentualne podwójne spacje (np. "nazwa ==wersja")
            if (name[j - 1] == ' ') 
            {
                i++;
                continue;
            }
            name[j] = ' '; // Zamień pierwszy '=' na spację
            i++; // Pomiń drugi '='
            j++;
            continue;
        }
        name[j] = package[i]; // Kopiuj znak
        i++;
        j++;
    }
    // Zmniejsz bufor do wymaganego rozmiaru (j + 1 na znak null '\0')
    name = (char*)realloc(name, 1 + j * sizeof(char));
    if (name == NULL)
        ERR("realloc");
    return name;
}

/**
 * "Instaluje" pakiet w danym środowisku.
 * @param env_path Nazwa środowiska (np. "new_environment").
 * @param package String pakietu (np. "numpy==1.0.0").
 */
void install_package(char* env_path, char* package)
{
    // Alokacja na ścieżkę do requirements: "./<env_path>/requirements.txt"
    char* full_env_path = (char*)calloc(strlen(env_path) + 4 + strlen("requirements.txt"), sizeof(char));
    strcpy(full_env_path, "./");
    strcat(full_env_path, env_path);
    strcat(full_env_path, "/requirements.txt");

    // Otwórz requirements w trybie "a" (append - dopisywanie na końcu)
    FILE* fp = fopen(full_env_path, "a");
    if (fp == NULL)
        // BŁĄD: Ta ścieżka to plik, nie katalog. Błąd powinien być "fopen", nie "opendir"
        ERR("opendir"); 
    
    // Przetwarzanie stringu wejściowego
    char* package_line = get_package_line(package); // "numpy==1.0.0" -> "numpy 1.0.0"
    char* package_name = get_package(package);      // "numpy==1.0.0" -> "numpy"

    // Alokacja na ścieżkę do pliku pakietu: "./<env_path>/<package_name>"
    char* full_package_path = (char*)calloc(strlen(env_path) + 5 + strlen(package_name), sizeof(char));
    strcpy(full_package_path, "./");
    strcat(full_package_path, env_path);
    strcat(full_package_path, "/");
    strcat(full_package_path, package_name); // Użyj wyczyszczonej nazwy ("numpy")

    // Sprawdzenie, czy pakiet (plik) już istnieje
    FILE* fp_pack = fopen(full_package_path, "r");
    if (fp_pack != NULL) // Jeśli fopen się udało, plik istnieje
    {
        fprintf(stderr, "package already exists");
        // Posprzątaj pamięć i deskryptory przed wyjściem
        free(package_line);
        free(package_name);
        free(full_env_path);
        free(full_package_path);
        fclose(fp);
        fclose(fp_pack);
        // BŁĄD: Zgodnie z treścią zadania, błąd powinien zatrzymać *cały* program.
        // exit() to robi, ale lepszą praktyką byłoby zwrócenie błędu do main.
        exit(EXIT_FAILURE);
    }
    // Jeśli fp_pack == NULL, plik nie istnieje (to dobrze).

    // Dopisz linię "nazwa wersja" do pliku requirements.txt
    fprintf(fp, "%s\n", package_line);

    // Utwórz pusty plik pakietu (np. "numpy") z uprawnieniami
    // O_CREAT: utwórz plik
    // 0440: uprawnienia r--r----- (właściciel odczyt, grupa odczyt)
    // UWAGA: Treść zadania prosiła o "odczyt dla wszystkich" (0444: r--r--r--)
    int fd = open(full_package_path, O_CREAT, 0440);
    close(fd); // Zamknij deskryptor pliku

    fclose(fp); // Zamknij plik requirements
    
    // Posprzątaj pamięć
    free(package_line);
    free(package_name);
    free(full_env_path);
    free(full_package_path);
}

/**
 * Tworzy nowe środowisko wirtualne.
 * @param env_name Nazwa środowiska do utworzenia.
 */
void create_venv(char* env_name)
{
    // Otwarcie bieżącego katalogu "."
    DIR* dp = opendir(".");
    if (dp == NULL)
        ERR("opendir");
    // UWAGA: Ten deskryptor 'dp' nie jest do niczego używany, poza sprawdzeniem
    // czy mamy dostęp do bieżącego katalogu.

    errno = 0; // Zresetuj errno przed wywołaniem systemowym
    // Utwórz katalog o podanej nazwie i uprawnieniach 0777 (rwxrwxrwx)
    int res = mkdir(env_name, 0777); 
    
    // Sprawdź, czy błąd (jeśli wystąpił) to EEXIST (plik/katalog już istnieje)
    if (errno == EEXIST) 
    {
        fprintf(stderr, "The virtual environment already exists");
        exit(EXIT_FAILURE);
    }
    if (res == -1) // Jeśli był inny błąd niż EEXIST
        ERR("mkdir");

    // Alokacja na ścieżkę do pliku requirements: "./<env_name>/requirements.txt"
    char* new_env_path = (char*)calloc(3 + strlen(env_name) + strlen("requirements.txt"), sizeof(char));
    strcpy(new_env_path, "./");
    strcat(new_env_path, env_name);
    strcat(new_env_path, "/requirements.txt");

    // Utwórz pusty plik requirements.txt (tryb "w+" tworzy i otwiera do zapisu/odczytu)
    FILE* file = fopen(new_env_path, "w+");
    if (file == NULL)
        ERR("fopen");
    
    if (fclose(file) == -1) // Zamknij plik
        ERR("fclose");
    if (closedir(dp) == -1) // Zamknij deskryptor katalogu
        ERR("closedir");
    
    free(new_env_path); // Zwolnij pamięć
}

int main(int argc, char** argv)
{
    int c; // Zmienna na opcje z getopt
    char* new_env_name = NULL; // Wskaźnik na nazwę nowego środowiska (-c)
    char* ex_env_name[MAX_VENV_UPDATES]; // Tablica wskaźników na nazwy istn. środowisk (-v)
    int i = 0; // Licznik, ile podano flag -v
    char* package = NULL; // Wskaźnik na string pakietu (-i)
    bool remove = false; // Flaga, czy usuwamy pakiet (-r)

    // UWAGA: String getopt() jest niespójny z treścią zadania.
    // Treść zadania: -r <PACKAGE_NAME> (np. "-r numpy")
    // Kod (getopt ... "c:v:i:r"): -r jest flagą BEZ argumentu, a -i (z argumentem)
    // musi być użyte do podania nazwy pakietu, nawet przy usuwaniu.
    while ((c = getopt(argc, argv, "c:v:i:r")) != -1)
    {
        switch (c)
        {
        case 'c': // Tworzenie środowiska, 'optarg' to nazwa
            new_env_name = optarg;
            break;
        case 'v': // Użycie istniejącego środowiska, 'optarg' to nazwa
            ex_env_name[i++] = optarg; // Zapisz do tablicy i zwiększ licznik
            break;
        case 'i': // Instalacja pakietu, 'optarg' to "nazwa==wersja" LUB nazwa przy usuwaniu
            package = optarg;
            break;
        case 'r': // Flaga usuwania
            remove = true;
            break;
        case '?': // Błąd getopt (np. nieznana opcja, brak argumentu dla -c, -v, -i)
            fprintf(stderr, "Wrong arguments");
            exit(EXIT_FAILURE);
        default:
            continue;
        }
    }
    
    // Logika wyboru operacji na podstawie zebranych flag

    // 1. TWORZENIE: Jeśli podano -c ORAZ nie podano -v, -r, -i
    if (new_env_name != NULL && ex_env_name[0] == NULL && !remove && package == NULL)
    {
        // UWAGA: Treść zadania sugerowała "-c -v <nazwa>", ale kod implementuje "-c <nazwa>"
        create_venv(new_env_name);
    }
    // 2. INSTALACJA: Jeśli nie podano -c, podano -i, podano przynajmniej jedno -v, nie podano -r
    else if (new_env_name == NULL && package != NULL && ex_env_name[0] != NULL && !remove)
    {
        // Pętla po wszystkich podanych środowiskach (zgodnie z Etapem 3)
        for (int j = 0; j < i; j++)
        {
            install_package(ex_env_name[j], package);
        }
    }
    // 3. USUWANIE: Jeśli podano -r, podano przynajmniej jedno -v, nie podano -c, podano -i
    else if (remove && ex_env_name[0] != NULL && new_env_name == NULL && package != NULL)
    {
        // BŁĄD: Kod implementuje usuwanie tylko z PIERWSZEGO podanego środowiska (ex_env_name[0]).
        // Powinien, podobnie jak instalacja, iterować po wszystkich 'i' środowiskach.
        
        // BŁĄD KRYTYCZNY: Do remove_package przekazywany jest surowy string z -i (np. "numpy==1.0.0").
        // Funkcja remove_package oczekuje *tylko* nazwy ("numpy"), aby usunąć plik
        // i znaleźć linię w requirements.
        // Powinno być:
        // char* pkg_name = get_package(package);
        // for (int j = 0; j < i; j++) {
        //     remove_package(ex_env_name[j], pkg_name);
        // }
        // free(pkg_name);
        remove_package(ex_env_name[0], package);
    }
    // 4. BŁĄD: Każda inna kombinacja flag (np. -c i -i razem) jest nieprawidłowa
    else
    {
        fprintf(stderr, "Wrong arguments");
        // TODO: Tutaj program powinien wypisać "usage" (jak używać programu)
        exit(EXIT_FAILURE);
    }
    
    return EXIT_SUCCESS; // Zakończ pomyślnie
}