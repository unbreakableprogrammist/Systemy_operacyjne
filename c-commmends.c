#include <stdio.h>
#include <stdlib.h>

int main() {
    // 1. Variables
    int a = 10;           // Integer variable
    float b = 5.5f;       // Floating-point variable
    char c = 'X';         // Character variable

    printf("a = %d, b = %.2f, c = %c\n", a, b, c);

    // 2. If statement
    if (a > 5) {
        printf("a is greater than 5\n");
    } else {
        printf("a is not greater than 5\n");
    }

    // 3. Loops
    // For loop: prints numbers from 0 to 4
    for (int i = 0; i < 5; i++) {
        printf("For loop i = %d\n", i);
    }

    // While loop: prints numbers from 5 to 1
    int j = 5;
    while (j > 0) {
        printf("While loop j = %d\n", j);
        j--;
    }

    // 4. Pointers
    int x = 20;
    int *ptr = &x; // ptr stores the address of x
    printf("Value of x = %d, Address of x = %p, Value via ptr = %d\n", x, (void*)ptr, *ptr);

    // 5. malloc (dynamic memory allocation)
    int *arr = (int*)malloc(5 * sizeof(int)); // allocate memory for 5 integers
    if (arr == NULL) {
        printf("Memory allocation failed\n");
        return 1;
    }
    // Initialize and print the array
    for (int k = 0; k < 5; k++) {
        arr[k] = k * 2;
        printf("arr[%d] = %d\n", k, arr[k]);
    }
    free(arr); // free the allocated memory

    return 0;
}

/*
Explanation:
- Variables: Used to store data of different types.
- If statement: Used for conditional execution.
- Loops: Used to repeat code (for known or unknown number of times).
- Pointers: Store memory addresses, allow indirect access to variables.
- malloc: Allocates memory dynamically at runtime; must be freed with free().
*/


// fscanf - wczytywanie z pliku fscanf(pliki, "formaty", &zmienna1, &zmienna2, ...);

int main(void) {
    FILE *fp;
    int a;
    float b;
    char slowo[20];

    fp = fopen("dane.txt", "r");   // otwarcie pliku do odczytu
    if (fp == NULL) {
        perror("Nie można otworzyć pliku");
        return 1;
    }

    fscanf(fp, "%d %f %s", &a, &b, slowo);

    printf("Wczytano: %d, %.2f, %s\n", a, b, slowo);

    fclose(fp);
    return 0;
}

/*
Co się dzieje krok po kroku:

1️⃣ fopen("dane.txt", "r") — otwiera plik do czytania i zwraca wskaźnik do FILE.
2️⃣ fscanf(fp, "%d %f %s", &a, &b, slowo) — czyta dane z pliku zgodnie z formatem.
3️⃣ Dane trafiają do zmiennych a, b, slowo.
4️⃣ fclose(fp) — zamyka plik.

*/


// fprintf - zapis do pliku fprintf(plik, "formaty", zmienna1, zmienna2, ...);
int main(void) {
    FILE *fp;
    int a = 42;
    float b = 3.14;

    fp = fopen("wyniki.txt", "w"); // "w" = write, czyli zapis
    if (fp == NULL) {
        perror("Błąd otwarcia pliku");
        return 1;
    }

    fprintf(fp, "Wynik: %d, liczba pi: %.2f\n", a, b);

    fclose(fp);
    return 0;
}

/*
Co się dzieje:
1️⃣ fopen("wyniki.txt", "w") — tworzy (lub nadpisuje) plik do zapisu.
2️⃣ fprintf(fp, ...) — pisze dane do pliku, zamiast na ekran.
3️⃣ fclose(fp) — zamyka plik i zapisuje dane na dysk.
*/


// perror - wyświetlanie błędów systemowych

#include <errno.h>

int main(void) {
    FILE *fp = fopen("nie_istnieje.txt", "r");
    if (fp == NULL) {
        perror("Błąd otwarcia pliku");
        return 1;
    }
    fclose(fp);
    return 0;
}


