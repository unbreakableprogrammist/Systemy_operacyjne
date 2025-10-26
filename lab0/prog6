#include <unistd.h> // Dla zmiennej 'environ'
#include <stdio.h>
#include <stdlib.h>

extern char** __environ;  // tablica zmiennych srodowsikowych 

int main(int argc,char** argv){
    int indx = 0;
    while(__environ[indx]){
        printf("%s\n",__environ[indx]);
        indx++;
    }
    
}