#include "hello.h"

void hello(){
    // int liczba;
    // scanf("%d", &liczba); // &liczba - wskaźnik na zmienną liczba
    // printf("Hello World! %d\n", liczba);

    char name[22];
    scanf("%21s", name); // %21s - maksymalnie 21 znaków + \0
    if (strlen(name)>20) ERR("Too long name");
    printf("Hello %s!\n", name);
    
}