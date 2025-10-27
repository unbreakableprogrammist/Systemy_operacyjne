/*
 * Skompiluj ten program:
 * gcc -o program program.c
 * * Uruchom go:
 * ./program
 * * Po uruchomieniu stworzy plik 'przyklad.txt' i wypisze jego zawartość.
 */

#include <stdio.h>    // Dla perror(), printf()
#include <stdlib.h>   // Dla exit()
#include <fcntl.h>    // Dla open() i flag O_RDWR, O_CREAT itd.
#include <unistd.h>   // Dla write(), read(), close()
#include <string.h>   // Dla strlen()

int main() {
    
    const char *nazwa_pliku = "przyklad.txt";
    const char *tekst_do_zapisu = "Hello, world!\nTo jest druga linia.";
    int fd; // To jest nasz deskryptor pliku (File Descriptor)

    // ======================================================
    //   CZĘŚĆ 1: ZAPIS (write)
    // ======================================================
    
    printf("Otwieram plik do zapisu...\n");

    /*
     * Otwieramy plik.
     * O_WRONLY: Otwórz tylko do zapisu.
     * O_CREAT: Stwórz plik, jeśli nie istnieje.
     * O_TRUNC: Wyczyść plik (skróć do 0), jeśli istnieje.
     * 0644: Uprawnienia (rw-r--r--), jeśli plik jest tworzony.
     */
    fd = open(nazwa_pliku, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    
    // Zawsze sprawdzaj, czy się udało. open() zwraca -1 przy błędzie.
    if (fd == -1) {
        perror("Błąd open() przy zapisie");
        exit(EXIT_FAILURE);
    }
    
    // Zapisujemy dane. write() zwraca liczbę zapisanych bajtów.
    ssize_t zapisane_bajty = write(fd, tekst_do_zapisu, strlen(tekst_do_zapisu));
    
    if (zapisane_bajty == -1) {
        perror("Błąd write()");
        close(fd); // Zamknij plik nawet przy błędzie
        exit(EXIT_FAILURE);
    }
    
    printf("Zapisano %ld bajtów.\n", zapisane_bajty);
    
    // Zawsze zamykaj deskryptor po skończonej pracy!
    close(fd);
    
    
    // ======================================================
    //   CZĘŚĆ 2: ODCZYT (read)
    // ======================================================

    printf("\nOtwieram plik do odczytu...\n");
    
    // Bufor, do którego wczytamy dane
    char bufor[100]; 
    
    // Otwieramy ten sam plik, ale tylko do odczytu
    fd = open(nazwa_pliku, O_RDONLY);
    
    if (fd == -1) {
        perror("Błąd open() przy odczycie");
        exit(EXIT_FAILURE);
    }
    
    // Czytamy dane. Prosimy o max 99 bajtów (by zostawić miejsce na '\0')
    // read() zwraca liczbę FAKTYCZNIE wczytanych bajtów.
    ssize_t odczytane_bajty = read(fd, bufor, sizeof(bufor) - 1);
    
    if (odczytane_bajty == -1) {
        perror("Błąd read()");
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    // WAŻNE: read() nie dodaje znaku końca stringu.
    // Musimy zrobić to sami, aby bezpiecznie użyć printf("%s", ...).
    bufor[odczytane_bajty] = '\0';
    
    printf("Odczytano %ld bajtów.\n", odczytane_bajty);
    printf("\n--- Zawartość pliku ---\n%s\n-------------------------\n", bufor);
    
    // Zamykamy plik
    close(fd);

    return 0;
}