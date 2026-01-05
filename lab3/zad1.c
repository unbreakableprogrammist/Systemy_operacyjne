#include <math.h>
#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

typedef struct thread_estimation {
    pthread_t tid;
    double estimation;
} thread_estimation;

int main(int argc, char ** argv) {
    if(argc != 3){
        ERR("zla liczba argumentow ");
    }
    int k = stoi(argv[1]);
    int n = stoi(argv[2]);

    /*
    plan na zadanie jest taki , tworzymy sobie tablice struct ( pid , double estymacja pi ) ktora bedzie na heapie , tam watki beda sobie wpisywaly 
    swoje obliczenia , a na koncu policzymy srednia arytmetyczna z wszyskich  
    */

    thread_estimation* tab = (thread_estimation*)malloc(n*sizeof(thread_estimation));
    for(int i=0;i<n;i++){
        
    }
    return 0;
}