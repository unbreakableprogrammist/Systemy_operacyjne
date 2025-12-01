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

volatile sig_atomic_t parent_running = 1;
volatile sig_atomic_t ill = 0;
volatile sig_atomic_t should_be_ill = 0;
void alarm_handler(int sig){
    parent_running = 0;
}

void children_work(){
    printf("I have been created ! , My pid is : %d\n",getpid());
    struct timespec ts = {0,100000000};

    while(parent_running){
        nanosleep(&ts,NULL);
        printf("Living la vida loca [PID]: %d\n",getpid());
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
            children_work();
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
    }
    if(kill(0,SIGTERM)<0) ERR("kill");
    exit(EXIT_SUCCESS);
}

