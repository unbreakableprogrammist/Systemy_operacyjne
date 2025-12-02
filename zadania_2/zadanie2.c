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
volatile sig_atomic_t probability=0;
volatile sig_atomic_t licznik = 0;  // licznik kaszlniec na mnie
volatile sig_atomic_t sygnal_kaszlnal_na_nas=0;
void alarm_handler(int sig){
    parent_running = 0;
}
void siguser1_handler(int sig,siginfo_t* info,void* usless){
    if(ill==1) return;
    pid_t who_coughed;
    if(info == NULL) 
        who_coughed=-1; // blad
    else
        who_coughed=info->si_pid;
    printf("Child of this pid: %d, coughet ad me: %d",who_coughed,getpid());
    srand(time(NULL)*getpid());
    int is_sick = (1+rand()%100);
    if(is_sick<=probability){
        ill=1;
    }
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

    sigset_t mask,oldmask;
    sigemptyset(&mask); //czyscimy
    sigaddset(&mask,SIGUSR1); // blokujemy SIGUSER1
    // SIGBLOCK -> sygnaly z maski beda dodane do blokowanych
    // SIGUNBLOCK -> sygnaly z maski beda usuniete z blokowanych
    // SIGSETMASK -> nadpisz cala maske
    sigprocmask(SIG_BLOCK,&mask,&oldmask); // ustawiamy blokowanie na SIGUSR1
    memset(&sa,0,sizeof(sa));
    sa.sa_sigaction = siguser1_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1,&sa,0);
    while(!ill){
        sigsuspend(&oldmask);
    }
    //struct timespec nowa = {k,0};
    alarm(k);
    //struct sigaction sa;
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = alarm_handler; // robi to samo co alarm , oznacza ze rodzic skonczyl prace
    if(sigaction(SIGALRM,&sa,NULL)<0)
        ERR("sigaction");
    while(parent_running){
        nanosleep(&ts,NULL);
        kill(0,SIGUSR1);
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
    pid_t pierwsze_dziecko;
    for(int i=0;i<n;i++){
        dziecko=fork();
        if(i==0) pierwsze_dziecko=dziecko;
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
    kill(pierwsze_dziecko,SIGUSR1);
    alarm(t);
    while(parent_running){
        // <- tutaj waitpid i jesli ktores dziecko wyjdzie to chore i zabrane przez rodzicow 
    }
    // tutaj waitpid to po zakonczaniu programu wiec dzieci chore albo zdrowe ale w przedszkolu 
    if(kill(0,SIGTERM)<0) ERR("kill");
    exit(EXIT_SUCCESS);
}

