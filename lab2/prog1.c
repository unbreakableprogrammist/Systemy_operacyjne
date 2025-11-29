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
void usage(char *name)
{
    fprintf(stderr, "USAGE: %s 0<n\n", name);
    exit(EXIT_FAILURE);
}

int children_work(){
    srand(time(NULL)*getpid()); // tworzy ziarno na podstawie czasu i PIDu do losowania
    int time = 5 + rand()%6; // losuje czas od 5 do 10 sekund
    sleep(time);
    printf("Dziecko o PID: %d, czekało %d sekund\n", getpid(), time);
    exit(EXIT_SUCCESS);
}

int main(int argc , char** argv){ // argc - ilosc argumentow , argv - wskaznik na tablice wskaznikow , a tamte wskazniki to poczatek tablicy 
    if(argc != 2) 
        usage(argv[0]);
    int n = atoi(argv[1]);
    if(n <= 0)
        usage(argv[0]);
    pid_t p;
    int* dzieci = (int*)malloc(n*sizeof(int));
    for(int i=0;i<n;i++){
        p=fork();
        if(p<0) ERR("fork:");
        if(p==0) //jestesmy w dziecku ( bo dziecko ma ta sama funkcje)
            children_work();
        dzieci[i]=p;
    }
    int ile_dzieci =0;
    while(1){
        ile_dzieci = 0; // Zerujemy licznik na początku sprawdzenia
        
        for(int i=0; i<n; i++){
            // Jeśli PID w tablicy to 0, znaczy że już to dziecko obsłużyliśmy wcześniej
            if (dzieci[i] == 0) continue; 

            // waitpid z WNOHANG sprawdza status bez blokowania
            int status;
            pid_t result = waitpid(dzieci[i], &status, WNOHANG);

            if (result == 0) {
                // Dziecko nadal działa
                ile_dzieci++;
            } else if (result > 0) {
                // Dziecko zakończyło się (zostało właśnie posprzątane)
                // Możemy oznaczyć w tablicy, że już nie musimy go sprawdzać
                // np. ustawiając id na 0 (jeśli dodasz taką logikę) lub po prostu nie zliczamy go
                printf("Dziecko PID %d zakończyło się.\n", result);
                dzieci[i] = 0; // Oznaczamy jako "obsłużone"
            } else {
                // Błąd waitpid
                perror("waitpid");
            }
        }
        
        printf("Liczba żywych dzieci: %d\n", ile_dzieci);
        
        if(ile_dzieci == 0) break;
        
        sleep(3);
    }
    return EXIT_SUCCESS;
}