
-----

# 🚀 SOP - Notatki do Laboratorium 1

To jest Twój wyczerpujący przewodnik po funkcjach wymaganych na Laboratorium 1. Został zorganizowany w grupy tematyczne odpowiadające zadaniom z instrukcji.

## 📋 Spis Treści

1.  [Ważne uwagi (kompilacja, błędy)](#wazne-uwagi)
2.  [Grupa 1: Przetwarzanie argumentów (`getopt`)](#grupa-1-przetwarzanie-argumentow-linii-polecen)
3.  [Grupa 2: Wysokopoziomowe I/O (`stdio.h`)](#grupa-2-wysokopoziomowe-buforowane-io---stdioh)
      * `fopen()`
      * `fclose()`
      * `fseek()`
      * `fprintf()` / `fputc()` / `fwrite()`
      * `rand()` i `srand()`
4.  [Grupa 3: Zarządzanie plikami i uprawnieniami](#grupa-3-zarzadzanie-plikami-i-uprawnieniami)
      * `unlink()`
      * `umask()`
5.  [Grupa 4: Przechodzenie drzewa katalogów (`nftw`)](#grupa-4-przechodzenie-drzewa-katalogow-zadanie-2)
6.  [Grupa 5: Niskopoziomowe I/O (`unistd.h`, `fcntl.h`)](#grupa-5-niskopoziomowe-niebuforowane-io-zadanie-3)
      * `open()`
      * `close()`
      * `write()`
      * `read()`
      * `lseek()`

-----

## Ważne uwagi

### 1\. Kompilacja ⚙️

Aby skompilować programy, używaj `gcc`. Flaga `-Wall` włączy wszystkie ostrzeżenia (bardzo przydatne\!), a `-o NAZWA` ustawi nazwę pliku wyjściowego.

```bash
gcc -Wall -o moj_program moj_program.c
```

### 2\. Definicja `_XOPEN_SOURCE` 📜

Niektóre funkcje (zwłaszcza `nftw`) są częścią standardu POSIX (XOPEN), a nie czystego C. Aby kompilator je "widział", **musisz** dodać tę linijkę **na samym początku** pliku `.c`, przed wszystkimi `#include`:

```c
#define _XOPEN_SOURCE 500 // Lub 600, lub 700
```

### 3\. Obsługa Błędów 🚨

W programowaniu systemowym **ZAWSZE** musisz sprawdzać, czy funkcja się udała.

  * Funkcje zwracające wskaźnik (jak `fopen`) zwracają `NULL` przy błędzie.
  * Funkcje zwracające `int` (jak `open`, `close`, `fseek`) zwracają `-1` przy błędzie.

Gdy wystąpi błąd, globalna zmienna `errno` jest ustawiana na kod błędu. Aby wypisać ludzki komunikat, użyj `perror()`.

```c
#include <stdio.h>
#include <errno.h> // Wymagane dla `errno`

// ...
if (jaks_funkcja() == -1) {
    perror("Błąd w 'jaks_funkcja'"); // Wypisze np. "Błąd w 'jaks_funkcja': Permission denied"
    exit(EXIT_FAILURE); // Zakończ program
}
```

-----

## Grupa 1: Przetwarzanie argumentów linii poleceń

Funkcje te służą do wczytania argumentów programu (np. `-n NAZWA`).

### `getopt()`

  * **Nagłówek:** `<unistd.h>`
  * **Sygnatura:** `int getopt(int argc, char * const argv[], const char *optstring);`
  * **Opis:** Parsuje argumenty linii poleceń. Wywołuje się ją w pętli `while`.
  * **Kluczowe parametry:**
      * `argc`, `argv`: Dokładnie te, które dostajesz w `main()`.
      * `optstring`: "Szablon" opcji, których szukasz.
          * `"n:p:s:"` oznacza: szukaj flag `-n`, `-p` i `-s`.
          * **Dwukropek (`:`)** po literze oznacza, że ta flaga **wymaga argumentu** (np. `-n plik.txt`).
          * `"h"` (bez dwukropka) oznaczałoby flagę bez argumentu (np. `-h` dla pomocy).
  * **Wartość zwracana:**
      * Kolejna znaleziona litera opcji (np. `'n'`, `'p'`, `'s'`).
      * `'?'` jeśli znaleziono nieznaną opcję lub brakowało argumentu.
      * `-1` gdy skończą się opcje do przetworzenia (co kończy pętlę `while`).
  * **Zmienne globalne (automatycznie używane przez `getopt`):**
      * **`char *optarg`**: Gdy `getopt` znajdzie opcję z argumentem (np. `-n plik.txt`), ustawi `optarg` tak, by wskazywał na ten argument (na `"plik.txt"`).
      * `int optind`: Indeks następnego argumentu w `argv` do przetworzenia.

#### Przykład (Zadanie 1)

```c
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> // dla strtol

int main(int argc, char *argv[]) {
    int c;
    char *nazwa_pliku = NULL;
    long uprawnienia = -1;
    long rozmiar = -1;

    // Szukamy n:, p: i s: (wszystkie z argumentami)
    while ((c = getopt(argc, argv, "n:p:s:")) != -1) {
        switch (c) {
            case 'n':
                nazwa_pliku = optarg; // optarg wskazuje na tekst po -n
                break;
            case 'p':
                // Konwertujemy tekst (ósemkowy!) na liczbę
                uprawnienia = strtol(optarg, NULL, 8); 
                break;
            case 's':
                // Konwertujemy tekst (dziesiętny) na liczbę
                rozmiar = strtol(optarg, NULL, 10);
                break;
            case '?': // Nieznana opcja lub brak argumentu
            default:
                fprintf(stderr, "Użycie: %s -n NAZWA -p OKTAL -s ROZMIAR\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Sprawdzamy, czy wczytaliśmy wszystko
    if (nazwa_pliku == NULL || uprawnienia == -1 || rozmiar == -1) {
        fprintf(stderr, "Błąd: Brak wszystkich wymaganych argumentów.\n");
        fprintf(stderr, "Użycie: %s -n NAZWA -p OKTAL -s ROZMIAR\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Plik: %s, Uprawnienia: %lo, Rozmiar: %ld\n", 
           nazwa_pliku, uprawnienia, rozmiar);

    // ... reszta programu ...
    return 0;
}
```

-----

## Grupa 2: Wysokopoziomowe (buforowane) I/O - `stdio.h`

Jest to "przyjazny" zestaw funkcji do operacji na plikach. Działają na buforze, co jest wydajne przy małych operacjach. Używają wskaźnika `FILE *`.

### `fopen()`

  * **Nagłówek:** `<stdio.h>`
  * **Sygnatura:** `FILE *fopen(const char *path, const char *mode);`
  * **Opis:** Otwiera lub tworzy plik i zwraca wskaźnik do "strumienia" (`FILE *`), którego będziesz używać do dalszych operacji.
  * **Kluczowe parametry:**
      * `path`: Ścieżka do pliku (np. `"plik.txt"`).
      * `mode`: Tryb otwarcia (jako tekst):
          * `"r"`: (Read) Tylko do odczytu. Plik musi istnieć.
          * `"w"`: (Write) Tylko do zapisu. **Czyści plik do zera** jeśli istnieje. Tworzy plik, jeśli nie istnieje.
          * `"a"`: (Append) Do dopisywania. Zapisuje na końcu pliku. Tworzy plik, jeśli nie istnieje.
          * `"r+"`: Odczyt i zapis. Plik musi istnieć.
          * `"w+"`: Odczyt i zapis. **Czyści plik do zera** jeśli istnieje. Tworzy plik, jeśli nie istnieje.
          * `"a+"`: Odczyt i dopisywanie. Zapis zawsze na końcu. Tworzy plik, jeśli nie istnieje.
  * **Wartość zwracana:**
      * Wskaźnik `FILE *` w przypadku sukcesu.
      * `NULL` w przypadku błędu (np. brak pliku w trybie `"r"`, brak uprawnień).

### `fclose()`

  * **Nagłówek:** `<stdio.h>`
  * **Sygnatura:** `int fclose(FILE *stream);`
  * **Opis:** Zamyka strumień. **TO JEST KRYTYCZNE\!** `fclose` opróżnia bufor (tzw. "flush"), czyli fizycznie zapisuje na dysku wszystkie dane, które "czekały" w pamięci.
  * **Wartość zwracana:**
      * `0` w przypadku sukcesu.
      * `EOF` (zazwyczaj `-1`) w przypadku błędu.

### `fseek()`

  * **Nagłówek:** `<stdio.h>`
  * **Sygnatura:** `int fseek(FILE *stream, long offset, int whence);`
  * **Opis:** Przesuwa "kursor" (wskaźnik pozycji) w pliku.
  * **Kluczowe parametry:**
      * `stream`: Wskaźnik `FILE *` z `fopen`.
      * `offset`: O ile bajtów się przesunąć (może być ujemny).
      * `whence`: Skąd liczyć:
          * `SEEK_SET`: Od początku pliku.
          * `SEEK_CUR`: Od bieżącej pozycji kursora.
          * `SEEK_END`: Od końca pliku.
  * **Wartość zwracana:** `0` (sukces) lub `-1` (błąd).
  * **Sztuczka (Zadanie 1): "Rozciąganie" pliku**
    Zadanie 1 wymaga stworzenia pliku o rozmiarze `SIZE`, wypełnionego zerami. Nie musisz pisać `SIZE` zer w pętli. Wystarczy, że skoczysz na ostatni bajt i coś tam zapiszesz.
    ```c
    // Chcemy plik o rozmiarze 1024 bajtów
    long size = 1024;
    FILE *f = fopen("plik.txt", "w");

    // 1. Skocz na pozycję 1023 (1024 - 1)
    fseek(f, size - 1, SEEK_SET); 

    // 2. Zapisz tam JEDEN bajt (np. zero)
    fputc('\0', f);

    // 3. Zamknij plik
    fclose(f);

    // System operacyjny automatycznie wypełni "dziurę" (bajty 0-1022) zerami.
    ```

### `fprintf()` / `fputc()` / `fwrite()`

  * **Opis:** Służą do zapisu danych do strumienia.
  * `int fprintf(FILE *stream, const char *format, ...);`
      * Działa jak `printf`, ale pisze do pliku. Np. `fprintf(f, "Liczba: %d\n", 10);`.
  * `int fputc(int c, FILE *stream);`
      * Zapisuje jeden znak (bajt), np. `fputc('A', f);`.
  * `size_t fwrite(void *ptr, size_t size, size_t nitems, FILE *stream);`
      * Zapisuje surowe dane binarne (np. całą strukturę lub tablicę).

### `rand()` i `srand()`

  * **Nagłówek:** `<stdlib.h>` (i `<time.h>` dla `srand`)
  * **Opis:** Służą do generowania liczb pseudolosowych.
  * `void srand(unsigned int seed);`
      * Inicjalizuje generator. **Musisz to wywołać RAZ na początku programu**, aby za każdym razem losować inne liczby.
  * `int rand(void);`
      * Zwraca losową liczbę całkowitą (między 0 a `RAND_MAX`).
  * **Jak używać (Zadanie 1):**
    ```c
    #include <stdlib.h>
    #include <time.h> // dla time()

    // 1. Inicjalizacja (RAZ w main())
    srand(time(NULL)); 

    // ...

    // 2. Losowanie znaku [A-Z]
    // 'Z' - 'A' + 1 = 26
    char losowa_litera = 'A' + (rand() % 26);

    // 3. Losowanie pozycji w pliku o rozmiarze 'size'
    long losowa_pozycja = rand() % size;
    ```

-----

## Grupa 3: Zarządzanie plikami i uprawnieniami

### `unlink()`

  * **Nagłówek:** `<unistd.h>`
  * **Sygnatura:** `int unlink(const char *path);`
  * **Opis:** Usuwa plik (kasuje jego nazwę z katalogu).
  * **Wartość zwracana:** `0` (sukces) lub `-1` (błąd).
  * **Wskazówka (Zadanie 1):** Zadanie każe usunąć plik, *jeśli istnieje*. `unlink` zwróci błąd (`-1`), jeśli pliku nie ma. Nie chcemy wtedy przerywać programu.
    ```c
    #include <errno.h> // dla ENOENT

    // ...
    if (unlink(nazwa_pliku) == -1) {
        // Sprawdzamy, czy błąd to "Plik nie istnieje"
        if (errno != ENOENT) {
            // Jeśli to był INNY błąd (np. brak uprawnień)
            perror("Błąd unlink");
            exit(EXIT_FAILURE);
        }
        // Jeśli errno == ENOENT, to wszystko OK, pliku po prostu nie było.
    }
    ```
    *Krótsza wersja (dla zaawansowanych):* `if (unlink(name) && errno != ENOENT) ERR("unlink");`

### `umask()`

  * **Nagłówek:** `<sys/stat.h>`
  * **Sygnatura:** `mode_t umask(mode_t mask);`
  * **Opis:** Ustawia "maskę" tworzenia plików dla procesu. **To jest bardzo podchwytliwe\!**
  * **Jak to działa:** `umask` to **negatyw**. Mówi systemowi, jakie uprawnienia ma **ZABRAĆ** plikowi podczas tworzenia.
      * Domyślnie `fopen` (i `open`) chcą tworzyć pliki z uprawnieniami `0666` (`rw-rw-rw-`).
      * Domyślna maska systemu to często `0022` (`----w--w-`).
      * System robi: `0666 & (~0022)` = `0666 & 0755` = `0644`. Wynik: `rw-r--r--`.
  * **Problem (Zadanie 1):** Chcemy *dokładnie* uprawnienia `perms` (np. `0600`). Musimy obliczyć maskę, która to da.
  * **Formuła:** `maska = ~perms & 0777;`
      * `perms = 0600` (`110 000 000`)
      * `~perms` = `...001 111 111`
      * `& 0777` = `0177`
      * Ustawiamy `umask(0177)`.
      * System robi: `0666 & (~0177)` = `0666 & 0600` = `0600`. **Działa\!**

#### Przykład (Zadanie 1)

```c
#include <sys/stat.h>

// ...
mode_t perms = 0644; // Wczytane z strtol(..., 8)

// 1. Ustaw maskę, aby uzyskać DOKŁADNE uprawnienia
// MUSI być wywołane PRZED fopen() / open()
umask(~perms & 0777); 

// 2. Stwórz plik
FILE *f = fopen("plik.txt", "w");
if (f == NULL) {
    perror("fopen");
    exit(EXIT_FAILURE);
}
// Plik "plik.txt" będzie miał teraz uprawnienia 0644
// ...
```

-----

## Grupa 4: Przechodzenie drzewa katalogów (Zadanie 2)

### `nftw()`

  * **Nagłówek:** `<ftw.h>`
  * **WYMAGANIE:** Musisz zdefiniować `#define _XOPEN_SOURCE 500` na górze pliku.
  * **Sygnatura:** `int nftw(const char *path, int (*fn)(...*), int fd_limit, int flags);`
  * **Opis:** (New File Tree Walk) "Spaceruje" po drzewie katalogów (`path`) i dla każdego znalezionego elementu wywołuje Twoją funkcję `fn`.
  * **Kluczowe parametry:**
      * `path`: Ścieżka startowa (np. `"."`).
      * `fn`: Wskaźnik na Twoją funkcję zwrotną (callback), którą `nftw` ma wywoływać.
      * `fd_limit`: Limit otwartych deskryptorów plików (np. `20`).
      * `flags`: Flagi kontrolujące zachowanie:
          * `FTW_DEPTH`: (Depth-first) Najpierw odwiedź zawartość katalogu, potem sam katalog (z flagą `FTW_DP`).
          * `FTW_PHYS`: (Physical) Nie podążaj za dowiązaniami symbolicznymi (symlinkami).

### Funkcja zwrotna (Callback) `fn`

To jest serce operacji. Twoja funkcja **musi** mieć taką sygnaturę:
`int moj_callback(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)`

  * **Parametry, które dostajesz:**
      * `fpath`: Pełna ścieżka do bieżącego elementu (np. `./katalog/plik.txt`).
      * `sb`: Wskaźnik na strukturę `stat` z metadanymi pliku (rozmiar `sb->st_size`, uprawnienia `sb->st_mode`, itd.).
      * `typeflag`: Mówi, czym jest ten element:
          * `FTW_F`: Zwykły plik.
          * `FTW_D`: Katalog (odwiedzany *przed* jego zawartością).
          * `FTW_DP`: Katalog (odwiedzany *po* jego zawartości, tylko z `FTW_DEPTH`).
          * `FTW_SL`: Symlink.
          * `FTW_DNR`: Katalog, którego nie da się przeczytać.
      * `ftwbuf`: Informacje o "spacerze" (np. `ftwbuf->level` - poziom zagłębienia).
  * **Wartość zwracana (przez Twój callback):**
      * **`0`**: Kontynuuj przeglądanie.
      * **Inna wartość**: Zatrzymaj `nftw` natychmiast i zwróć tę wartość.

#### Przykład (Zadanie 2)

```c
// MUSI BYĆ NA SAMEJ GÓRZE PLIKU
#define _XOPEN_SOURCE 500

#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

// Nasza funkcja callback
int licz_pliki(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    
    // Interesują nas tylko zwykłe pliki
    if (typeflag == FTW_F) {
        printf("Plik: %s (Rozmiar: %ld bajtów)\n", fpath, sb->st_size);
        // Tutaj można by np. sumować rozmiary w zmiennej globalnej
    }
    
    // Zwracamy 0, aby nftw szło dalej
    return 0; 
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Użycie: %s <ścieżka_startowa>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *sciezka = argv[1];
    int flagi = 0; // Domyślne flagi
    
    // Użyj flagi FTW_PHYS, aby nie podążać za symlinkami
    // flagi |= FTW_PHYS; 

    // Wywołujemy nftw
    if (nftw(sciezka, licz_pliki, 20, flagi) == -1) {
        perror("Błąd nftw");
        exit(EXIT_FAILURE);
    }

    return 0;
}
```

-----

## Grupa 5: Niskopoziomowe (niebuforowane) I/O (Zadanie 3)

Jest to "surowy" interfejs do operacji na plikach. Działa na **deskryptorach plików** (File Descriptors - małe liczby `int`, np. `3`, `4`, `5`). Jest mniej wygodny, ale daje większą kontrolę.

### `open()`

  * **Nagłówki:** `<sys/stat.h>`, `<fcntl.h>`
  * **Sygnatura:** `int open(const char *path, int oflag, ... /* mode_t mode */);`
  * **Opis:** Odpowiednik `fopen`. Zwraca deskryptor pliku.
  * **Kluczowe parametry:**
      * `path`: Ścieżka do pliku.
      * `oflag`: Flagi (łączone bitowym OR `|`):
          * `O_RDONLY`: Tylko do odczytu.
          * `O_WRONLY`: Tylko do zapisu.
          * `O_RDWR`: Odczyt i zapis.
          * `O_CREAT`: Stwórz plik, jeśli nie istnieje.
          * `O_TRUNC`: Wyczyść plik do zera, jeśli istnieje.
          * `O_APPEND`: Pisz zawsze na końcu.
          * `O_EXCL`: (z `O_CREAT`) Zwróć błąd, jeśli plik już istnieje.
      * `mode_t mode`: **Wymagany tylko**, gdy używasz `O_CREAT`. Ustawia uprawnienia (np. `0644`).
  * **Wartość zwracana:**
      * Deskryptor pliku (np. `3`) w przypadku sukcesu.
      * `-1` w przypadku błędu.

### `close()`

  * **Nagłówek:** `<unistd.h>`
  * **Sygnatura:** `int close(int fd);`
  * **Opis:** Zamyka deskryptor pliku. Odpowiednik `fclose`.
  * **Wartość zwracana:** `0` (sukces) lub `-1` (błąd).

### `write()`

  * **Nagłówek:** `<unistd.h>`
  * **Sygnatura:** `ssize_t write(int fd, const void *buf, size_t count);`
  * **Opis:** Zapisuje `count` bajtów z bufora `buf` do pliku `fd`.
  * **Wartość zwracana:**
      * Liczba *faktycznie* zapisanych bajtów (sukces).
      * `-1` w przypadku błędu.

### `read()`

  * **Nagłówek:** `<unistd.h>`
  * **Sygnatura:** `ssize_t read(int fd, void *buf, size_t count);`
  * **Opis:** Czyta *co najwyżej* `count` bajtów z pliku `fd` do bufora `buf`.
  * **Wartość zwracana:**
      * Liczba *faktycznie* odczytanych bajtów (może być mniejsza niż `count`\!).
      * **`0`**: Koniec pliku (End-of-File).
      * `-1`: Błąd.
  * **⚠️ PUŁAPKA:** `read` **NIE** dodaje znaku `\0` na końcu wczytanych danych\! Jeśli wczytujesz tekst, musisz zrobić to ręcznie.

### `lseek()`

  * **Nagłówek:** `<unistd.h>`
  * **Sygnatura:** `off_t lseek(int fd, off_t offset, int whence);`
  * **Opis:** Odpowiednik `fseek`. Przesuwa kursor.
  * **Parametry:** Działają tak samo jak w `fseek` (`SEEK_SET`, `SEEK_CUR`, `SEEK_END`).
  * **Wartość zwracana:** Nowa pozycja kursora (w bajtach od początku) lub `-1` (błąd).

#### Przykład (Zadanie 3)

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main() {
    const char *plik = "test_low_level.txt";
    const char *napis = "Hello low-level!";
    int fd; // Deskryptor pliku
    char bufor[100];
    ssize_t ile_bajtow;

    // --- ZAPIS (odpowiednik "w") ---
    // Flagi: Zapis | Stwórz | Wyczyść
    int flagi = O_WRONLY | O_CREAT | O_TRUNC;
    // Uprawnienia (wymagane przez O_CREAT)
    mode_t tryb = 0644; 

    fd = open(plik, flagi, tryb);
    if (fd == -1) {
        perror("Błąd open (zapis)");
        exit(EXIT_FAILURE);
    }

    // Zapisujemy string (używamy strlen do policzenia bajtów)
    ile_bajtow = write(fd, napis, strlen(napis));
    if (ile_bajtow == -1) {
        perror("Błąd write");
        exit(EXIT_FAILURE);
    }
    printf("Zapisano %ld bajtów.\n", ile_bajtow);

    if (close(fd) == -1) {
        perror("Błąd close (zapis)");
        exit(EXIT_FAILURE);
    }


    // --- ODCZYT (odpowiednik "r") ---
    fd = open(plik, O_RDONLY);
    if (fd == -1) {
        perror("Błąd open (odczyt)");
        exit(EXIT_FAILURE);
    }

    // Czytamy do bufora (max 99 bajtów, by zostawić miejsce na \0)
    ile_bajtow = read(fd, bufor, 99);
    if (ile_bajtow == -1) {
        perror("Błąd read");
        exit(EXIT_FAILURE);
    }

    // KRYTYCZNE: read() NIE dodaje '\0'!
    bufor[ile_bajtow] = '\0';

    printf("Odczytano %ld bajtów: '%s'\n", ile_bajtow, bufor);
    
    if (close(fd) == -1) {
        perror("Błąd close (odczyt)");
        exit(EXIT_FAILURE);
    }

    return 0;
}
```