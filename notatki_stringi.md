
---

# Praktyczny przewodnik po stringach w C (z przykładami)

> **Cel:** bezpiecznie i skutecznie obrabiać łańcuchy znaków w C.
> **Założenie:** łańcuch = tablica `char[]` zakończona bajtem `'\0'` (C-string).

## Spis treści

* [1. Zasady ogólne](#1-zasady-ogólne)
* [2. Podstawowe funkcje (`<string.h>`)](#2-podstawowe-funkcje-stringh)
* [3. Wyszukiwanie i skanowanie](#3-wyszukiwanie-i-skanowanie)
* [4. Konwersje tekst ↔ liczby (`<stdlib.h>`)](#4-konwersje-tekst--liczby-stdlibh)
* [5. Tokenizacja (dzielenie tekstu)](#5-tokenizacja-dzielenie-tekstu)
* [6. Operacje na pamięci (nie tylko dla stringów)](#6-operacje-na-pamięci-nie-tylko-dla-stringów)
* [7. Przydatne rozszerzenia (POSIX/GNU)](#7-przydatne-rozszerzenia-posixgnu)
* [8. Wzorce „do wklejenia” (trim/split/replace itd.)](#8-wzorce-do-wklejenia-trimsplitreplace-itd)
* [9. Najczęstsze pułapki i jak ich unikać](#9-najczęstsze-pułapki-i-jak-ich-unikać)
* [10. Miejsca do doprecyzowania (Twoje decyzje)](#10-miejsca-do-doprecyzowania-twoje-decyzje)
* [11. Ścieżka wnioskowania (jak doszliśmy do wniosków)](#11-ścieżka-wnioskowania-jak-doszliśmy-do-wniosków)
* [12. Źródła / dalsza lektura](#12-źródła--dalsza-lektura)

---

## 1. Zasady ogólne

1. **Zawsze licz bajty/bufory.** Funkcje z `<string.h>` *nie* sprawdzają rozmiarów.
2. **`'\0'` jest obowiązkowy.** Większość funkcji przetwarza do pierwszego bajtu `0`.
3. **UTF-8 ≠ „znak to znak”.** Funkcje liczą **bajty**, nie znaki Unicode.
4. **Dla nakładających się obszarów używaj `memmove`, a nie `memcpy`.**
5. **Preferuj warianty z limitem** (`strncpy`, `strncat`) lub `snprintf`/„own safe copy”.

---

## 2. Podstawowe funkcje (`<string.h>`)

```c
#include <string.h>
#include <stdio.h>
```

| Funkcja            | Co robi               | Przykład                                | Uwaga                      |
| ------------------ | --------------------- | --------------------------------------- | -------------------------- |
| `strlen(s)`        | Długość bez `'\0'`.   | `printf("%zu\n", strlen("abc"));` → `3` | Nie licz na O(1); to O(n). |
| `strcpy(d, s)`     | Kopiuje `s` → `d`.    | `strcpy(dst, "ala");`                   | NIE sprawdza rozmiaru `d`. |
| `strncpy(d, s, n)` | Max `n` bajtów.       | `strncpy(d, s, n);`                     | Nie zawsze doda `'\0'`.    |
| `strcat(d, s)`     | Dokleja `s` do `d`.   | `strcat(path, ".txt");`                 | Bufor `d` musi pomieścić.  |
| `strncat(d, s, n)` | Dokleja `n` bajtów.   | `strncat(buf, ext, 4);`                 | Często lepiej `snprintf`.  |
| `strcmp(a,b)`      | Porównanie leksyk.    | `if (!strcmp(a,b))`                     | Zwraca `<0`, `0`, `>0`.    |
| `strncmp(a,b,n)`   | Porównuje `n` bajtów. | `strncmp("ab","ac",1)==0`               | Używaj do prefiksów.       |

**Bezpieczna alternatywa:** formatowanie do bufora przez `snprintf`:

```c
char buf[128];
snprintf(buf, sizeof(buf), "%s/%s", dir, file);
```

---

## 3. Wyszukiwanie i skanowanie

| Funkcja           | Opis                                    | Przykład                             |
| ----------------- | --------------------------------------- | ------------------------------------ |
| `strchr(s, c)`    | Pierwszy znak `c` w `s`.                | `char *p = strchr(path, '/');`       |
| `strrchr(s, c)`   | Ostatni znak `c`.                       | `char *p = strrchr(path, '.');`      |
| `strstr(s, sub)`  | Pierwsze wystąpienie `sub`.             | `strstr("abcdef","cd")`              |
| `strpbrk(s, set)` | Pierwszy znak z zestawu.                | `strpbrk("name@example.com","@+")`   |
| `strspn(s, set)`  | Długość prefiksu z samych znaków `set`. | `strspn("123abc","0123456789") == 3` |
| `strcspn(s, set)` | Długość prefiksu bez znaków `set`.      | `strcspn("abc.def","./") == 3`       |

**Wycinanie rozszerzenia pliku:**

```c
char name[256] = "report.final.txt";
char *dot = strrchr(name, '.');
if (dot) *dot = '\0'; // name = "report.final"
```

---

## 4. Konwersje tekst ↔ liczby (`<stdlib.h>`)

```c
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
```

### `strtol` / `strtoll` – inty

```c
errno = 0;
char *end;
long v = strtol("0xFFabc", &end, 0); // base=0: auto (0x, 0...)
if (errno == ERANGE) { /* overflow/underflow */ }
printf("wartość=%ld, reszta=\"%s\"\n", v, end); // 255, "abc"
```

* **Używaj `end`** do wykrycia, czy cokolwiek sparsowano (`end == s` → brak cyfr).
* **`base=0`** = automatyka (`0x..` heks, `0..` ósemkowo, inaczej dziesiętnie).
* **Sprawdzaj `errno`** i porównuj z `LONG_MAX/MIN`.

### `strtod` / `strtof` – floaty

```c
errno = 0;
char *end;
double d = strtod("  3.14e2abc", &end);
if (errno == ERANGE) { /* +/-HUGE_VAL */ }
printf("%g, reszta=%s\n", d, end); // 314, "abc"
```

> `atoi`, `atof` są wygodne, ale **nieobsługujące błędów** → wolimy `strto*`.

---

## 5. Tokenizacja (dzielenie tekstu)

### `strtok` (modyfikuje wejściowy bufor)

```c
char line[] = "kot,pies;królik";
for (char *tok = strtok(line, ",;"); tok; tok = strtok(NULL, ",;"))
    printf("[%s]\n", tok);
```

* Modyfikuje `line` (wstawia `'\0'`).
* Nie jest bezpieczny wątkowo (użyj `strtok_r` na POSIX).

### `strtok_r` (POSIX)

```c
char line[] = "a b c";
char *save = NULL;
for (char *tok = strtok_r(line, " ", &save); tok; tok = strtok_r(NULL, " ", &save))
    puts(tok);
```

---

## 6. Operacje na pamięci (nie tylko dla stringów)

```c
#include <string.h>
```

| Funkcja          | Co robi                                   | Użycie                                  |
| ---------------- | ----------------------------------------- | --------------------------------------- |
| `memcpy(d,s,n)`  | Kopiuje **n bajtów**.                     | Szybkie kopiowanie, **bez** nakładania. |
| `memmove(d,s,n)` | Kopiuje bezpiecznie gdy obszary nachodzą. | „Shiftowanie” w buforze.                |
| `memset(p,c,n)`  | Wypełnia bajtem `c`.                      | Zerowanie: `memset(buf, 0, n)`.         |
| `memcmp(a,b,n)`  | Porównuje **n** bajtów.                   | Zwraca `<0/0/>0`.                       |

**Przykład — usuwanie znaku (przesuw w lewo):**

```c
char s[] = "Ala ma kota";
size_t i = 3; // usuń spację po "Ala"
memmove(s + i, s + i + 1, strlen(s) - i); // w tym '\0'
```

---

## 7. Przydatne rozszerzenia (POSIX/GNU)

> Nie w czystym ISO C, ale powszechnie dostępne na Linuksie/macOS.

| Funkcja                      | Nagłówek      | Opis                                                      |
| ---------------------------- | ------------- | --------------------------------------------------------- |
| `strdup`, `strndup`          | `<string.h>`  | Alokują nowy string i kopiują.                            |
| `strcasecmp`, `strncasecmp`  | `<strings.h>` | Porównanie **case-insensitive**.                          |
| `strlcpy`, `strlcat` *(BSD)* | `<string.h>`  | Kopiowanie/sklejanie z gwarantowanym zakończeniem `'\0'`. |
| Regex                        | `<regex.h>`   | Wzorce regularne POSIX (parsowanie „na poważnie”).        |

**Przykład — `strdup`:**

```c
char *copy = strdup(src);
if (!copy) { /* ENOMEM */ }
/* ... */
free(copy);
```

---

## 8. Wzorce „do wklejenia” (trim/split/replace itd.)

### 8.1. Bezpieczna kopia z gwarantowanym `'\0'`

```c
/* Zastępuje nieportable strlcpy */
size_t safe_copy(char *dst, size_t cap, const char *src) {
    if (cap == 0) return 0;
    size_t n = 0;
    while (n + 1 < cap && src[n]) { dst[n] = src[n]; n++; }
    dst[n] = '\0';
    return n; // liczba skopiowanych bez '\0'
}
```

### 8.2. Trim (usuń białe znaki z obu końców)

```c
#include <ctype.h>

char *trim(char *s) {
    char *end;
    while (*s && isspace((unsigned char)*s)) s++;
    if (!*s) return s; // sam whitespace
    end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}
```

### 8.3. Zamiana pierwszego wystąpienia podciągu

```c
void replace_first(char *s, const char *oldw, const char *neww) {
    char *p = strstr(s, oldw);
    if (!p) return;
    size_t lo = strlen(oldw), ln = strlen(neww);
    if (ln == lo) { memcpy(p, neww, ln); return; }
    // założenie: bufor s ma zapas!
    size_t tail = strlen(p + lo) + 1; // +1 na '\0'
    if (ln > lo) memmove(p + ln, p + lo, tail);
    else         memmove(p + ln, p + lo, tail);
    memcpy(p, neww, ln);
}
```

### 8.4. Rozbijanie linii CSV-like (prosty, bez cudzysłowów)

```c
size_t split_simple(char *s, const char *delims, char *out[], size_t maxout) {
    size_t n = 0;
    for (char *tok = strtok(s, delims); tok && n < maxout; tok = strtok(NULL, delims))
        out[n++] = tok;
    return n;
}
```

### 8.5. Parsowanie liczb z walidacją (`strtol`)

```c
int parse_int(const char *s, long *out) {
    errno = 0;
    char *end;
    long v = strtol(s, &end, 10);
    if (end == s) return 0;             // brak cyfr
    if (errno == ERANGE) return 0;      // overflow/underflow
    while (*end && isspace((unsigned char)*end)) end++; // dopuszczamy spacje
    if (*end) return 0;                  // śmieci po liczbie
    *out = v; 
    return 1;
}
```

---

## 9. Najczęstsze pułapki i jak ich unikać

* **Przepełnienie bufora**: unikaj „gołych” `strcpy/strcat`; preferuj `snprintf` lub własne bezpieczne kopie.
* **Brak `'\0'` po `strncpy`**: ręcznie dodać `dest[n-1] = '\0';` gdy kopiujesz do stałego bufora.
* **`memcpy` przy nakładaniu**: zamień na `memmove`.
* **`strtok` modyfikuje dane**: jeśli potrzebujesz oryginału → pracuj na kopii, albo użyj parsera bez destrukcji.
* **UTF-8**: `strlen` liczy bajty, nie znaki — przy znakach wielobajtowych uważaj na cięcia w środku kodpunktu.
* **`atoi/atof`**: brak sygnalizacji błędów → preferuj `strto*`.
* **Lokalizacja**: `strtod` zależy od `locale` (przecinek/kropka). Jeśli chcesz zawsze kropkę, ustaw `setlocale(LC_NUMERIC, "C");` lub użyj własnego parsera.

---

## 10. Miejsca do doprecyzowania (Twoje decyzje)

1. **UTF-8 czy czyste ASCII?** Jeśli UTF-8, potrzebujesz funkcji „świadomych” wielobajtowości (np. ICU) do liczenia znaków/cięć.
2. **Środowisko:** czyste ISO C, POSIX (Linux/macOS), czy też BSD-owe `strlcpy/strlcat`?
3. **Wielowątkowość:** czy będziesz tokenizować w wątkach? Jeśli tak, trzymaj się `strtok_r`.
4. **Bezpieczeństwo buforów:** wolisz `snprintf` wszędzie, czy jednak dopuszczasz `strn*` z własnym kontraktem na bufor?
5. **CSV/parsowanie z cudzysłowami:** jeśli tak, prosty `strtok` nie wystarczy — trzeba parsera stanowego lub regex POSIX.

---

## 11. Ścieżka wnioskowania (jak doszliśmy do wniosków)

1. **Zidentyfikowaliśmy problem**: operacje na C-stringach wymagają manualnej kontroli buforów i zakończeń `'\0'`.
2. **Zmapowaliśmy API standardowe**: core z `<string.h>` + konwersje z `<stdlib.h>` rozwiązuje 80% przypadków.
3. **Dodaliśmy POSIX/GNU** dla praktycznych usprawnień (`strdup`, `strtok_r`, `strcasecmp`).
4. **Wytypowaliśmy ryzyka** (przepełnienia, nakładanie, lokalizacja, UTF-8) → zaproponowaliśmy bezpieczne wzorce (`snprintf`, `memmove`, walidacja `errno`, `endptr`).
5. **Stworzyliśmy gotowce** (trim/split/replace/safe_copy/parse) tak, by minimalizować błędy w labach/konkursach.

---

## 12. Źródła / dalsza lektura

* **ISO C (C17)**: Rozdz. 7.24 *String handling*; 7.21 *Input/output*.
* **man-pages** (Linux):

  * `man 3 string` — dokumentacja `strlen/strcpy/strcat/...`
  * `man 3 strtol`, `man 3 strtod` — konwersje z walidacją
  * `man 3 strtok`, `man 3 strtok_r`
* **The Open Group POSIX** (SUS): `strcasecmp`, `strtok_r`, `regex(7)`
* **glibc manual/source**: implementacyjne detale i warianty rozszerzeń

---

**Hint:** Jeśli chcesz, mogę dorzucić tu gotową sekcję **„ściągawka A4”** (same prototypy + pułapki) lub wyeksportować to do **PDF/MD** z krótkim indeksem funkcji.
