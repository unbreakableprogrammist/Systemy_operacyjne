#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))
void usage(void)
{
    fprintf(stderr, "USAGE: signals n k p l\n");
    fprintf(stderr, "n - number of children\n");
    fprintf(stderr, "k - Interval before SIGUSR1\n");
    fprintf(stderr, "p - Interval before SIGUSR2\n");
    fprintf(stderr, "l - lifetime of child in cycles\n");

    exit(EXIT_FAILURE);
}

volatile sig_atomic_t last_signal = 0; // ostatni sygnal ( jakie dzieci beda dostawac )

void handler(int sig){
    last_signal=sig;  // dostalismy sygnalem wiec ustawiamy jaki to sygnal 
}

void children_work(int l){
    srand(getpid());
    for(int i=0;i<l;i++){
        int ile_zostalo = 5+rand()%6;
        //printf("Dziecko %d zasypia na %d sekund \n",getpid(),ile_zostalo);
        while(ile_zostalo>0){
            ile_zostalo=sleep(ile_zostalo); // dosypiamy tyle ile potrzebujemy 
            if(ile_zostalo>0){
                if(last_signal==SIGUSR1){
                    printf("Sukces \n");
                }else{
                    printf("Failure \n");
                }
            }
        }

    }
    printf("UMIERAAAAAMMMM %d\n",getpid());
}

int main(int argc, char **argv)
{
    if (argc != 5)
        usage();

    int n = atoi(argv[1]);
    int k = atoi(argv[2]);
    int p = atoi(argv[3]);
    int l = atoi(argv[4]);
    if (n <= 0 || k <= 0 || p <= 0 || l <= 0)
        usage();
    
    struct sigaction sa; // struktura do obslugi sygnalow
    memset(&sa, 0, sizeof(struct sigaction)); // Wyzerowanie struktury 
    sa.sa_handler = handler;
    sigaction(SIGUSR1, &sa, NULL); // Ustawienie obslugi sygnalu SIGUSR1  , czyli bedzie nic nie robil
    sigaction(SIGUSR2, &sa, NULL); // Ustawienie obslugi sygnalu SIGUSR2 ---||----
    
    
    pid_t t;
    for(int i=0;i<n;i++){
        t=fork();
        if(t<0) ERR("fork");
        if(t==0){ // dziecko
            children_work(l);
            exit(EXIT_SUCCESS);
        }
    }
    // 2. NOWOŚĆ: Blokujemy sygnały w Rodzicu PRZED forkiem
    // Dzięki temu Rodzic jest "zabetonowany" i nie słyszy sygnałów, 
    // więc kill(0) nie przerwie jego sleepa.
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigprocmask(SIG_BLOCK, &mask, &oldmask); // Blokujemy USR1 i USR2
    while(n>0){
        sleep(k);
        kill(0,SIGUSR1); // wysylamy sygnal do wszystkich dzieci
        while((t=waitpid(0,NULL,WNOHANG))>0 ){ // sprawdzamy czy jakies dziecko nie umarlo
            n--;
        }
        sleep(p);
        kill(0,SIGUSR2); // wysylamy sygnal do wszystkich dzieci
        while((t=waitpid(0,NULL,WNOHANG))>0 ){ // sprawdzamy czy jakies dziecko nie umarlo
            n--;
        }
    }
}   