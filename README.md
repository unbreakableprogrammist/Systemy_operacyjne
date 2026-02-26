# 💻 Systemy Operacyjne (SOP) - Baza Wiedzy i Kody

Witaj w repozytorium poświęconym zmaganiom z przedmiotem **Systemy Operacyjne**! 

Jeśli tu trafiłeś, to pewnie już wiesz, że **te laboratoria potrafią być bardzo trudne** i nie wybaczają błędów (szczególnie Segmentation Fault 😅). Znajdziesz tutaj prawie wszystkie kody potrzebne do przetrwania zajęć, rozwiązania zadań praktycznych oraz notatki, które nie raz potrafią uratować życie przed kolokwium. 🚀

## 🛠 Co tu znajdziesz? (Główne zagadnienia)
Repozytorium w 97% opiera się na niskopoziomowym programowaniu systemowym w języku **C**. Rozwiązania obejmują tematykę taką jak:

- 📁 **Operacje na plikach:** Niskopoziomowe czytanie i zapisywanie danych, operacje na deskryptorach plików i systemie plików.
- 🚦 **Sygnały (Signals):** Obsługa sygnałów POSIX (wysyłanie, odbieranie, maskowanie, tworzenie własnych handlerów).
- ⚙️ **Procesy (Processes):** Klonowanie (`fork`), zarządzanie procesami potomnymi, czekanie na zakończenie (`wait`) oraz szeroko pojęta komunikacja międzyprocesowa (IPC - potoki, pamięć współdzielona).
- 🧵 **Wątki (Threads):** Tworzenie wątków (pthreads) oraz ich synchronizacja za pomocą muteksów i zmiennych warunkowych.

## 📂 Struktura repozytorium
Repozytorium jest uporządkowane chronologicznie i tematycznie:

* 📁 **`lab0` - `lab4`** oraz **`lab4_punktowane`** — Kody realizowane i omawiane na kolejnych laboratoriach.
* 📁 **`zadania_1` - `zadania_4`** — Autorskie rozwiązania trudniejszych zestawów zadań.
* 📁 **`projekt`** — Implementacja większego projektu zaliczeniowego.
* 📝 **`notatki`** — Zbiór najważniejszych informacji teoretycznych, ściągawek i wyjaśnień trudniejszych koncepcji z SOP-ów.

## 🚀 Jak uruchamiać kody?
Większość zadań wymaga środowiska opartego na systemie Linux/UNIX. Do wygodnej kompilacji polecam używać dołączonych do niektórych folderów plików **Makefile**.

1. Sklonuj repozytorium na swój dysk:
   ```bash
   git clone [https://github.com/unbreakableprogrammist/Systemy_operacyjne.git](https://github.com/unbreakableprogrammist/Systemy_operacyjne.git)
Przejdź do interesującego Cię katalogu (np. lab2).

Skompiluj kod używając polecenia make lub bezpośrednio z kompilatora GCC:

Bash
gcc nazwa_pliku.c -o nazwa_programu -Wall
# W przypadku wątków pamiętaj o fladze -pthread!
Uruchom program:

Bash
./nazwa_programu
⚠️ Pamiętaj!
Kody zostały udostępnione w celach edukacyjnych jako pomoc w nauce i referencja. Zachęcam do samodzielnej analizy i zrozumienia, jak działają konkretne funkcje systemowe — samo "kopiuj-wklej" może zemścić się na wejściówkach! 😉

Niech Twoje procesy potomne zawsze kończą się bez zostawania zombie, a muteksy nigdy nie prowadzą do zakleszczeń (deadlocków)! Powodzenia! 🍀
