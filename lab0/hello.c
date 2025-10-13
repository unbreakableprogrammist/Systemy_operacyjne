#include <stdio.h>
#include "hello.h"

void hello(){
    int liczba;
    scanf("%d", &liczba); // &liczba - wskaźnik na zmienną liczba
    printf("Hello World! %d\n", liczba);
}