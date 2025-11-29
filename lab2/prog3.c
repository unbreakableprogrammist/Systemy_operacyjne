#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))
void usage(char *name)
{
    fprintf(stderr, "USAGE: %s m  p\n", name);
    fprintf(stderr,
            "m - number of 1/1000 milliseconds between signals [1,999], "
            "i.e. one milisecond maximum\n");
    fprintf(stderr, "p - after p SIGUSR1 send one SIGUSER2  [1,999]\n");
    exit(EXIT_FAILURE);
}

volatile sig_atomic_t last_signal;
void handler(int sig)
{
    last_signal = sig;
}
void sigchld_handler(int sig)  // podmieniamy SIGCHILD zeby nie tworzyc zombie
{
    pid_t pid;
    while (1)
    {
        pid = waitpid(0, NULL, WNOHANG);
        if (pid == 0)
            return;
        if (pid <= 0)
        {
            if (errno == ECHILD)
                return;
            ERR("waitpid");
        }
    }
}
void child_work(int m, int p)
{
    // tutaj musimy co m sekund wyslac SIGUSR1 do rodzica
    // a co p wyslac SIGUSR2
    struct timespec ts = {0,m*10000}; // 0 sekund i 1000*m nanosekund
    int counbter = 0;
    while(1)
    {
        for(int i=0;i<p;i++){
            nanosleep(&ts,NULL);
            kill(getppid(),SIGUSR1);
        }
        nanosleep(&ts,NULL);
        kill(getppid(),SIGUSR2);
        counbter++;
        printf("[%d] sent %d SIGUSR2\n", getpid(), counbter);
    }
    

}
void parent_work(sigset_t oldmask)
{
    int counter=0;
    while(1){
        last_signal = 0;
        while (last_signal != SIGUSR2)  // ignorujemy SIGUSR1
            sigsuspend(&oldmask);  // odblokowujemy sygnaly i czekamy na sygnal ( teraz jakikoliwiek sygnal bedzie odebrany to sigsuspend sie zakonczy)
        counter++;
        printf("[PARENT] received %d SIGUSR2\n",counter);
    }
}

int main(int argc, char **argv)
{
    if (argc != 3)
        usage(argv[0]);
    int m = atoi(argv[1]);  // ile milliseconds
    int p = atoi(argv[2]);  // co ktory p przesylamy SIGUSR2
    if (m < 1 || m > 999 || p < 1 || p > 999)
        usage(argv[0]);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    sigaction(SIGCHLD, &sa, NULL);
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    // w mask blokujemy sygnaly SIGUSR1 i SIGUSR2
    sigset_t mask,oldmask;
    sigemptyset(&mask);
    sigaddset(&mask,SIGUSR1);
    sigaddset(&mask,SIGUSR2);
    sigprocmask(SIG_BLOCK,&mask,&oldmask);
    pid_t pid = fork();
    if (pid < 0)
        ERR("fork");
    if (pid == 0)
    {
        // child
        child_work(m, p);
        exit(EXIT_SUCCESS);
    }else{
        // parent
        parent_work(oldmask);
    }
    return EXIT_SUCCESS;
}