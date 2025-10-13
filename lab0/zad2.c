#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define ERR(source) (perror(source),\
fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),\
exit(EXIT_FAILURE))

int main(){
    char name[22];
    scanf("%21s", name); // %21s - maksymalnie 21 znaków + \0
    if (strlen(name)>20) ERR("Too long name");
    printf("Hello %s!\n", name);
    return EXIT_SUCCESS;
}