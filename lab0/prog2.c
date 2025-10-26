#include "hello.h"

int main(){

    char name[22];
    scanf("%21s", name); // %21s - maksymalnie 21 znaków + \0
    if (strlen(name)>20) ERR("Too long name");
    printf("Hello %s!\n", name);

}