#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))
void usage(char *name) {
    fprintf(stderr, "USAGE: %s t k n p\n", name);
    fprintf(stderr, "ARGUMENTS:\n");
    fprintf(stderr, "  t : Simulation duration in seconds (1 - 100)\n");
    fprintf(stderr, "  k : Time until parents pick up sick child in seconds (1 - 100)\n");
    fprintf(stderr, "  n : Number of children (1 - 30)\n");
    fprintf(stderr, "  p : Infection probability in percent (1 - 100)\n");
    exit(EXIT_FAILURE);
}

volatile sig_atomic_t parent_running = 1; //czy nie minela sekunda 
volatile sig_atomic_t ill = 0; // czy dziecko jest chore
volatile sig_atomic_t should_be_ill = 0;  // czy moze zachorowac ( prawdopodobienstwo )
volatile sig_atomic_t licznik = 0;  // licznik kaszlniec na mnie
void alarm_handler(int sig){
    parent_running = 0;
}
void siguser1_handler(int sig){
    if(should_be_ill && !ill)
        ill=1;
    licznik++;
}

void children_work(int p,int k){
    srand(time(NULL)*getpid());
    printf("I have been created ! , My pid is : %d\n",getpid());
    // handlowanie sigtermem
    struct sigaction sa;
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = alarm_handler; // robi to samo co alarm , oznacza ze rodzic skonczyl prace
    if(sigaction(SIGTERM,&sa,NULL)<0)
        ERR("sigaction");
    // losujemy losowy czas i prawdopodobienstwo
    int random_time = 50+ rand()%151;
    struct timespec ts = {0,1000000*random_time};
    // sprawdzamy czy wylosowane prawdopodobienstwo jest lepsze niz podane
    int probability = 1+rand()&101;
    if(probability>=p) should_be_ill = 1; // mowimy ze mozemy zachorowac

    //handlowanie siguser1
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = siguser1_handler; // jesli mozemy byc chorzy i ktos w nas strzeli to chorujemy
    if(sigaction(SIGUSR1,&sa,NULL)<0)
        ERR("sigaction");
    while(parent_running){
        nanosleep(&ts,NULL);
        
    }
}

int main(int argc,char** argv){
    if(argc!=5)
        usage("argc");
    int t = atoi(argv[1]);  // ile sekund dziala program 
    int k = atoi(argv[2]);  // po ile sekundach przyjezdzaja rodzice 
    int n = atoi(argv[3]);  //ilosc dzieci
    int p = atoi(argv[4]);  //prawdopodobienstwo zachorowania
     // teraz nadpiszemy rodzica zeby dzialal t sekund
    struct sigaction sa;
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = alarm_handler;
    if(sigaction(SIGALRM,&sa,NULL)<0)
        ERR("sigaction");
    // forkujemy dzieci 
    pid_t dziecko;
    for(int i=0;i<n;i++){
        dziecko=fork();
        if(dziecko<0) ERR("fork");
        if(dziecko==0){
            children_work(p,k);
            exit(EXIT_SUCCESS);
        }      
    }
    // my bedziemy ignorowac SIGUSR1 bo to tylko do innych dzieci 
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = SIG_IGN ;
    if(sigaction(SIGUSR1,&sa,NULL)<0)
        ERR("sigaction");
    alarm(t);
    while(parent_running){
        // <- tutaj waitpid i jesli ktores dziecko wyjdzie to chore i zabrane przez rodzicow 
    }
    // tutaj waitpid to po zakonczaniu programu wiec dzieci chore albo zdrowe ale w przedszkolu 
    if(kill(0,SIGTERM)<0) ERR("kill");
    exit(EXIT_SUCCESS);
}

