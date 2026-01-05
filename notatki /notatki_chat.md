# Kompendium: System plików – Laboratorium 1 (SOP1)

Poniższe kompendium zawiera pełny opis wszystkich funkcji omawianych w ramach Laboratorium 1 z Systemów Operacyjnych (Politechnika Warszawska, Informatyka). Każda sekcja opisuje funkcję, jej argumenty, zwracane wartości, flagi, potencjalne błędy oraz przykłady kodu w języku C.

---

## 📁 Przeglądanie katalogów – `opendir`, `readdir`, `closedir`, `stat`, `lstat`

### 🔹 Nagłówki:

```c
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
```

### 🔹 `opendir()`

Otwiera katalog i zwraca wskaźnik do struktury `DIR`.

```c
DIR *opendir(const char *dirname);
```

Zwraca `NULL` w przypadku błędu (np. brak uprawnień lub nieistniejący katalog).

### 🔹 `readdir()`

Czyta kolejne wpisy z katalogu:

```c
struct dirent *readdir(DIR *dirp);
```

Zwraca strukturę:

```c
struct dirent {
    ino_t d_ino;     // numer inode
    char  d_name[];  // nazwa pliku
};
```

Gdy katalog się kończy — zwraca `NULL`. Należy wtedy sprawdzić `errno`, aby odróżnić koniec katalogu od błędu.

### 🔹 `closedir()`

Zamyka katalog:

```c
int closedir(DIR *dirp);
```

Zwraca `0` przy sukcesie, `-1` przy błędzie.

### 🔹 `stat()` i `lstat()`

Pozwalają pobrać szczegółowe informacje o pliku:

```c
int stat(const char *restrict path, struct stat *restrict buf);
int lstat(const char *restrict path, struct stat *restrict buf);
```

* `stat()` – pobiera dane o pliku, na który wskazuje link symboliczny.
* `lstat()` – pobiera dane o samym linku.

W strukturze `stat` znajduje się m.in. rozmiar (`st_size`), właściciel (`st_uid`), uprawnienia (`st_mode`) i czasy modyfikacji.

### 🔹 Makra typu pliku:

| Makro        | Znaczenie        |
| ------------ | ---------------- |
| `S_ISREG(m)` | zwykły plik      |
| `S_ISDIR(m)` | katalog          |
| `S_ISLNK(m)` | link symboliczny |

### 📄 Przykład – zliczanie typów plików:

```c
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void scan_dir() {
    DIR *dirp;
    struct dirent *dp;
    struct stat filestat;
    int dirs = 0, files = 0, links = 0, other = 0;

    if ((dirp = opendir(".")) == NULL)
        ERR("opendir");

    do {
        errno = 0;
        if ((dp = readdir(dirp)) != NULL) {
            if (lstat(dp->d_name, &filestat)) ERR("lstat");
            if (S_ISDIR(filestat.st_mode)) dirs++;
            else if (S_ISREG(filestat.st_mode)) files++;
            else if (S_ISLNK(filestat.st_mode)) links++;
            else other++;
        }
    } while (dp != NULL);

    if (errno != 0) ERR("readdir");
    if (closedir(dirp)) ERR("closedir");

    printf("Files: %d, Dirs: %d, Links: %d, Other: %d\n", files, dirs, links, other);
}

int main() {
    scan_dir();
    return EXIT_SUCCESS;
}
```

---

## 📂 Katalog roboczy – `getcwd`, `chdir`

```c
#include <unistd.h>
```

### 🔹 `getcwd()`

Pobiera ścieżkę aktualnego katalogu roboczego:

```c
char *getcwd(char *buf, size_t size);
```

Zwraca wskaźnik `buf` lub `NULL` przy błędzie.

### 🔹 `chdir()`

Zmienia katalog roboczy procesu:

```c
int chdir(const char *path);
```

Zwraca `0` przy sukcesie, `-1` przy błędzie.

### 📄 Przykład:

```c
#define MAX_PATH 101
char path[MAX_PATH];

if (getcwd(path, MAX_PATH) == NULL) ERR("getcwd");

if (chdir("/tmp")) ERR("chdir");
printf("Zmieniono katalog roboczy na: /tmp\n");
```

---

## 🌳 Rekurencyjne przeszukiwanie katalogów – `nftw`

```c
#include <ftw.h>
```

### 🔹 Deklaracja:

```c
int nftw(const char *path,
         int (*fn)(const char *, const struct stat *, int, struct FTW *),
         int fd_limit, int flags);
```

### 🔹 Flagi:

| Flaga       | Znaczenie                                 |
| ----------- | ----------------------------------------- |
| `FTW_PHYS`  | nie wchodzi w linki symboliczne           |
| `FTW_DEPTH` | przetwarza katalogi po ich zawartości     |
| `FTW_CHDIR` | zmienia katalog roboczy podczas chodzenia |
| `FTW_MOUNT` | nie przekracza granicy systemu plików     |

### 📄 Przykład:

```c
#define _XOPEN_SOURCE 500
#include <ftw.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int dirs = 0, files = 0, links = 0, other = 0;

int walk(const char *name, const struct stat *s, int type, struct FTW *f) {
    switch (type) {
        case FTW_D: dirs++; break;
        case FTW_F: files++; break;
        case FTW_SL: links++; break;
        default: other++; break;
    }
    return 0;
}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (nftw(argv[i], walk, 20, FTW_PHYS) == 0)
            printf("%s: files=%d, dirs=%d, links=%d, other=%d\n", argv[i], files, dirs, links, other);
        else perror("nftw");
        dirs = files = links = other = 0;
    }
    return 0;
}
```

---

## 📘 Operacje na plikach – `fopen`, `fseek`, `fread`, `fwrite`, `unlink`, `umask`

### 🔹 `fopen()`

```c
FILE *fopen(const char *pathname, const char *mode);
```

Tryby: `r`, `w`, `a`, `r+`, `w+`, `a+`.
Zwraca wskaźnik `FILE*` lub `NULL`.

### 🔹 `fseek()`

```c
int fseek(FILE *stream, long offset, int whence);
```

Ustawia pozycję kursora. `whence`: `SEEK_SET`, `SEEK_CUR`, `SEEK_END`.

### 🔹 `fread()` / `fwrite()`

```c
size_t fread(void *ptr, size_t size, size_t nitems, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nitems, FILE *stream);
```

### 🔹 `unlink()`

Usuwa plik:

```c
int unlink(const char *pathname);
```

### 🔹 `umask()`

Ustawia maskę tworzenia plików:

```c
mode_t umask(mode_t mask);
```

---

## ⚙️ Operacje niskopoziomowe – `open`, `read`, `write`, `close`

```c
#include <fcntl.h>
#include <unistd.h>
```

### 🔹 `open()`

```c
int open(const char *path, int oflag, ...);
```

Flagi: `O_RDONLY`, `O_WRONLY`, `O_RDWR`, `O_CREAT`, `O_TRUNC`, `O_APPEND`.

### 🔹 `read()` / `write()`

```c
ssize_t read(int fd, void *buf, size_t nbyte);
ssize_t write(int fd, const void *buf, size_t nbyte);
```

### 📄 Przykład kopiowania pliku:

```c
#define BUF 256
int fd1 = open("in.txt", O_RDONLY);
int fd2 = open("out.txt", O_WRONLY | O_CREAT, 0666);
char buf[BUF];
ssize_t r;
while ((r = read(fd1, buf, BUF)) > 0)
    write(fd2, buf, r);
close(fd1);
close(fd2);
```

---

## 🧩 Operacje wektorowe – `writev`, `readv`

```c
#include <sys/uio.h>
```

### 🔹 `writev()`

```c
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
```

Struktura:

```c
struct iovec {
    void   *iov_base;
    size_t  iov_len;
};
```

### 📄 Przykład:

```c
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    struct iovec iov[2];
    char buf1[] = "Hello, ";
    char buf2[] = "world!\n";

    iov[0].iov_base = buf1; iov[0].iov_len = 7;
    iov[1].iov_base = buf2; iov[1].iov_len = 7;

    int fd = open("output.txt", O_WRONLY | O_CREAT, 0666);
    writev(fd, iov, 2);
    close(fd);
}
```

---

## 🧠 Buforowanie wyjścia i `fprintf(stderr, ...)`

Funkcje `printf` buforują dane – wypisywane są dopiero po znaku nowej linii lub zakończeniu programu. Strumień błędów (`stderr`) nie jest buforowany, dlatego warto go używać do debugowania.

### 📄 Eksperyment:

```c
for (int i = 0; i < 10; i++) {
    printf("%d\n", i);  // buforowane
    sleep(1);
}
```

Zachowanie:

* przy wyjściu na terminal — dane pojawiają się natychmiast,
* przy przekierowaniu do pliku — mogą się pojawić dopiero po zakończeniu programu.

Aby wypisywać natychmiast, użyj:

```c
fprintf(stderr, "%d\n", i);
```

---

## 🧩 Makro `ERR` i `errno`

Standardowy sposób obsługi błędów:

```c
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
```

* `perror()` wypisuje komunikat systemowy na `stderr`,
* `errno` to globalna zmienna opisująca ostatni błąd.

---

## 📚 Źródła i manuale

* `man 3p opendir`, `readdir`, `closedir`
* `man 3p stat`, `lstat`, `fstatat`
* `man 3p nftw`
* `man 3p fopen`, `fseek`, `fread`, `fwrite`
* `man 3p open`, `read`, `write`, `close`
* `man 3p writev`, `readv`
* `man 3p umask`, `unlink`
* `man 3 errno`, `man 3 perror`

---

✅ **Podsumowanie:**
To kompendium obejmuje wszystkie kluczowe funkcje z pierwszego laboratorium SOP1, wraz z przykładami, opisami flag i typowych błędów. Może służyć jako samodzielna ściąga podczas laboratoriów lub pracy z systemem plików w języku C.
