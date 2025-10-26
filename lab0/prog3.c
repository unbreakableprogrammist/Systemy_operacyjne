#include "hello.h"

int main(){
    // int liczba;
    // scanf("%d", &liczba); // &liczba - wskaźnik na zmienną liczba
    // printf("Hello World! %d\n", liczba);

    char name[22];
    // scanf("%21s", name); // %21s - maksymalnie 21 znaków + \0
    // if (strlen(name)>20) ERR("Too long name");
    // printf("Hello %s!\n", name);

    while(fgets(name,21,stdin)!= NULL){
        printf("Hello %s",name);
        
    }
    
}