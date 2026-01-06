#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

typedef struct studenci{
    int* pierwszaki;
    int* drugaki;
    int* trzeciaki;
    int* inzynierowie;
    int* wyrzuceni1;
    int* wyrzuceni2;
    int* wyrzuceni3;
    int* wyrzuceni_inz;
} studenci;

/* 
pomysl jest taki : twrzymuy na poczatku n watkow i odpalamy dla kazdego 
*/

int main(int argc, char **argv) {
    if(argc != 2) ERR("zbyt malo argumentow");
    int n = atoi(argv[1]);

    return EXIT_SUCCESS;
}