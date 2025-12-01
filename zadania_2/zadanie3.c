#define _GNU_SOURCE
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
#define MAX_CHILDREN 100000008
#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))
void usage(char *name) {
    fprintf(stderr,"Problem with argv argument : %s",name);
    exit(EXIT_FAILURE);
}

volatile sig_atomic_t working_parent = 1; // dopoki nie dostaniemy SIGINT
volatile sig_atomic_t working_child = 0;
volatile sig_atomic_t child_indx = 0;
volatile sig_atomic_t number_of_children = 0;
pid_t dzieci[MAX_CHILDREN];

void child_user1_handler(int sig){
    working_child = 1;
}
void child_user2_handler(int sig){
    working_child = 0;
}
void sigint_handler(int sig){
    working_parent = 0;
}
void parent_user1_handler(int sig){
    kill(dzieci[child_indx],SIGUSR2);
    child_indx = (child_indx+1)%number_of_children;
    kill(dzieci[child_indx],SIGUSR1);
}
void bulk_write(int fd,char* bufor,int size){
    ssize_t c=0;;
    while(size>0){
        c=TEMP_FAILURE_RETRY(write(fd,bufor,size));
        size-=c;
        bufor+=c;
    }
}

void children_work(int indx){
    // handlowanie siguser1
    struct sigaction sa;
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = child_user1_handler;
    if(sigaction(SIGUSR1,&sa,NULL)<0)
        ERR("sigaction");
    // handlowanie siguser2
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = child_user2_handler;
    if(sigaction(SIGUSR2,&sa,NULL)<0)
        ERR("sigaction");
    int licznik=0;
    srand(time(NULL)*getpid());
    int randomowa=0;
    randomowa = 100+rand()%101;
    struct timespec ts = {0,randomowa * 1000000};
    while(working_parent){
        if(working_child){
            while(working_child){
                if(working_parent == 0) break; // zabezpieczenie
                nanosleep(&ts,NULL);
                licznik++;
                printf("My pid is : %d , and my counter is : %d\n",getpid(),licznik);
            }
        }else{
            pause(); // pause dziala dopoki nie dostaniemy jakiego koliwek sygnalu -> wtedy sie wybudza
        }
    }
    char name[20];
    snprintf(name,sizeof(name),"%d.txt",getpid());
    char buffer[64];
    int length = snprintf(buffer, sizeof(buffer), "%d\n", licznik);
    int fd = open(name,O_WRONLY|O_CREAT,0777);
    if(fd<0)
        ERR("open");
    bulk_write(fd,buffer,length);
    printf("Zapisane :) \n"); 
    close(fd);
}

int main(int argc,char** argv){
    if(argc<2)
        usage("argv");
    int N = atoi(argv[1]);
    number_of_children = N;
    // handlowanie sigint
    struct sigaction sa;
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = sigint_handler;
    if(sigaction(SIGINT,&sa,NULL));
    // tworzymy dzieci
    for(int i=0;i<N;i++){
        pid_t t = fork();
        if(t<0)
            ERR("fork");
        if(t==0){
            children_work(i);
            exit(EXIT_SUCCESS);
        }else{
            dzieci[i]=t;
        }
    }
    // handlowanie siguser1
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = parent_user1_handler;
    if(sigaction(SIGUSR1,&sa,NULL)<0)
        ERR("sigaction");
    while(working_parent){
    }
    printf("parrend stopped working \n");
    for(int i=0;i<N;i++){
        kill(dzieci[i],SIGINT);
    }
    while(wait(NULL)>0){
    }
}