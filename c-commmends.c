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