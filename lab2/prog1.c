#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))


int main(int argc , char** argv){ // argc - ilosc argumentow , argv - wskaznik na tablice wskaznikow , a tamte wskazniki to poczatek tablicy 

    return EXIT_SUCCESS;
}