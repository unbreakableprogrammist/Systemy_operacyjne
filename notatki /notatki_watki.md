
# Ściągawka z Wątków w C (pthreads)

Wątki w obrębie procesu dzielą:
* **Sterte (heap)** (pamięć alokowana dynamicznie `malloc`).
* **Zmienne globalne**.
* **Deskryptory plików**.

Każdy wątek ma własny **stos (stack)**.


### 1. Nagłówki i Makra
```c
#define _POSIX_C_SOURCE 200809L // Wymagane dla nanosleep, sigwait itp.
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

// Makro do obsługi błędów (przykładowe)
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

```

---

### 2. Tworzenie Wątków (`pthread_create`)

#### A. Wątek standardowy ("Joinable")

Domyślny tryb. Musisz na niego poczekać (`pthread_join`), inaczej wyciekną zasoby.

```c
pthread_t tid; // Tutaj system wpisze ID nowego wątku

// Argument 1: Wskaźnik na zmienną pthread_t (gdzie zapisać ID)
// Argument 2: Atrybuty (NULL = domyślne)
// Argument 3: Funkcja wątku (musi przyjmować void* i zwracać void*)
// Argument 4: Argument dla funkcji (wskaźnik void* na dane)
if (pthread_create(&tid, NULL, funkcja_watku, &moje_dane) != 0) {
    ERR("pthread_create failed");
}

```

#### B. Wątek odłączony ("Detached") od startu

Dla wątków, na które nie będziesz czekać. System sam sprząta po ich zakończeniu.

```c
pthread_t tid;
pthread_attr_t threadAttr; // 1. Zmienna na atrybuty

// 2. Inicjalizacja atrybutów
if (pthread_attr_init(&threadAttr) != 0) ERR("attr init");

// 3. Ustawienie trybu DETACHED
// Argument 1: Atrybuty do zmiany
// Argument 2: Flaga odłączenia
if (pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED) != 0) {
    ERR("setdetachstate failed");
}

// 4. Tworzenie wątku z atrybutami
if (pthread_create(&tid, &threadAttr, funkcja_watku, &moje_dane) != 0) {
    ERR("create detached failed");
}

// 5. Sprzątanie atrybutów (nie wpływa na stworzony już wątek)
pthread_attr_destroy(&threadAttr);

```

#### C. Odłączanie w trakcie (`pthread_detach`)

Jeśli wątek był *joinable*, ale zmieniliśmy zdanie.

```c
// Opcja 1: Wątek odłącza sam siebie (wewnątrz funkcji wątku)
pthread_detach(pthread_self());

// Opcja 2: Main odłącza wątek
if (pthread_detach(tid) != 0) ERR("detach failed");

```

---

### 3. Kończenie i Odbieranie Wyniku (`pthread_join`)

Służy do czekania na wątek *joinable* i sprzątania jego stosu.

```c
void* wynik; // Zmienna na wskaźnik zwrócony przez wątek

// Argument 1: ID wątku, na który czekamy
// Argument 2: Wskaźnik na wskaźnik (void**), gdzie zapisać wynik (lub NULL)
if (pthread_join(tid, &wynik) != 0) {
    ERR("pthread_join failed");
}

// Jeśli wątek zwrócił mallocowaną pamięć, trzeba ją zwolnić w mainie!
if (wynik != NULL) free(wynik);

```

---

### 4. Anulowanie Wątków (`Cancellation`)

#### A. Wysłanie żądania (np. w Main)

```c
// To tylko prośba o zakończenie, wątek musi ją "odebrać"
if (pthread_cancel(tid) != 0) ERR("cancel failed");

```

#### B. Konfiguracja wewnątrz wątku

Zabezpieczanie się przed przerwaniem w złym momencie (np. gdy trzymamy mutex).

```c
// 1. Wyłączenie możliwości anulowania (Sekcja Krytyczna)
// Argument 1: PTHREAD_CANCEL_DISABLE - ignoruj cancel
// Argument 2: NULL (stary stan nas nie obchodzi)
pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

/* ... ważne operacje na danych, alokacje ... */

// 2. Włączenie możliwości anulowania (powrót do normy)
pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

// 3. Ręczny punkt anulowania (Cancellation Point)
// Wymusza sprawdzenie, czy przyszedł cancel (przydatne w pętlach obliczeniowych)
pthread_testcancel();

```

#### C. Cleanup Handlers (Sprzątanie po śmierci)

Funkcja, która wykona się, gdy wątek zostanie anulowany lub zrobi `pthread_exit`.

```c
void sprzatanie(void* arg) {
    // Np. odblokowanie mutexa lub free()
    printf("Sprzątam zasoby!\n");
}

// W FUNKCJI WĄTKU:
// 1. Rejestracja sprzątacza (push)
pthread_cleanup_push(sprzatanie, &moje_dane);

/* ... kod, który może zostać przerwany (np. sleep, wait) ... */

// 2. Zdjęcie sprzątacza (pop)
// Argument: 1 = zdejmij I wykonaj funkcję; 0 = tylko zdejmij
pthread_cleanup_pop(1); 

```

### Punkty Anulowania (Cancellation Points)

W domyślnym trybie (*deferred*) wątek nie jest zabijany natychmiast po otrzymaniu `pthread_cancel`. Kontynuuje on pracę aż do momentu wywołania specjalnej funkcji systemowej (tzw. **Cancellation Point**), w której sprawdza, czy powinien się zakończyć.

**Najważniejsze funkcje będące punktami anulowania:**


* Operacje wejścia/wyjścia:** `read()`, `write()` – wątek zginie, próbując czytać/pisać.


* Czekanie:** `sleep()`, `nanosleep()`, `pause()`, `select()` – wątek zginie, zamiast spać dalej.


* Ręczne sprawdzenie:** `pthread_testcancel()` – funkcja, którą wpisujemy ręcznie w pętlach obliczeniowych (które nie używają powyższych funkcji), aby wymusić sprawdzenie, czy nadeszło żądanie anulowania.

---

### 5. Mutexy - Synchronizacja Danych

Tylko jeden wątek naraz może być w sekcji chronionej mutexem.

```c
// Inicjalizacja DYNAMICZNA (w main)
pthread_mutex_t mtx;
if (pthread_mutex_init(&mtx, NULL) != 0) ERR("mutex init");

// --- UŻYCIE (W WĄTKACH) ---

// 1. Zamknięcie (Lock) - czeka, jeśli inny wątek trzyma mutex
if (pthread_mutex_lock(&mtx) != 0) ERR("lock failed");

/* ... operacje na zmiennych współdzielonych ... */

// 2. Otwarcie (Unlock) - ZAWSZE musi być wykonane
if (pthread_mutex_unlock(&mtx) != 0) ERR("unlock failed");

// --------------------------

// Niszczenie (w main, gdy nikt już nie używa)
if (pthread_mutex_destroy(&mtx) != 0) ERR("mutex destroy");

```

---

### 6. Obsługa Sygnałów (`sigwait`)

Wątki nie powinny używać `signal()`. Blokujemy sygnały wszędzie i odbieramy je w jednym miejscu.

#### Krok 1: Blokowanie (w Main, PRZED stworzeniem wątków!)

```c
sigset_t mask;
sigemptyset(&mask);       // Wyczyść zbiór
sigaddset(&mask, SIGINT); // Dodaj Ctrl+C
sigaddset(&mask, SIGQUIT);// Dodaj Ctrl+\

// Blokada w całym procesie (nowe wątki to odziedziczą)
// Argument 1: SIG_BLOCK (dodaj do zablokowanych)
// Argument 2: Nowa maska
// Argument 3: Stara maska (NULL)
if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0) ERR("sigmask block");

```

#### Krok 2: Odbieranie (w dedykowanym wątku)

```c
int numer_sygnalu;

// Czeka na sygnał z maski. "Zjada" go z kolejki (nie uruchamia handlera systemowego).
// Argument 1: Zbiór sygnałów do czekania
// Argument 2: Gdzie zapisać numer odebranego sygnału
if (sigwait(&mask, &numer_sygnalu) != 0) ERR("sigwait failed");

if (numer_sygnalu == SIGINT) {
    printf("Odebrano SIGINT! Kończę pracę...\n");
}

```

#  Getclock
 
