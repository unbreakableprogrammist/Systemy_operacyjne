#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define ERR(source) (perror(source),\
fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),\
exit(EXIT_FAILURE))

/*
./prog Ala ma kota jeslit to uruchomimy to : 
Liczba argumentów: 4
argv[0] = ./prog
argv[1] = Ala
argv[2] = ma
argv[3] = kota


teraz cze,y char** argv bo : 
podwójny wskaźnik pozwala nam robić tablicę napisów, bo napis to też tablica, 
czyli char** argv to wskaźnik na tablicę wskaźników, i te wskaźniki wskazują na pierwszy znak w każdym słowie
*/

int main(int argc , char** argv){  
    for(int i=0;i<argc;i++){
        printf("%s",argv[i]);
        printf("\n");
    }
}