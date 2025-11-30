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

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))
void usage(char *name) {
    fprintf(stderr, "--------------------------------------------------------\n");
    fprintf(stderr, "USAGE: %s <n1> [n2] [n3] ...\n", name);
    fprintf(stderr, "--------------------------------------------------------\n");
    fprintf(stderr, "ARGUMENTS:\n");
    fprintf(stderr, "  n : Integer digits in range [0-9].\n");
    fprintf(stderr, "      A separate child process is created for each argument.\n");
    fprintf(stderr, "\nDESCRIPTION:\n");
    fprintf(stderr, "  The program spawns children that wait for SIGUSR1 signals.\n");
    fprintf(stderr, "  Each child writes a generated file named PID.txt based on\n");
    fprintf(stderr, "  the received digit and random block size.\n");
    fprintf(stderr, "\nEXAMPLE:\n");
    fprintf(stderr, "  %s 1 5 0\n", name);
    fprintf(stderr, "--------------------------------------------------------\n");
    exit(EXIT_FAILURE);
}

volatile sig_atomic_t licznik = 0; // zmienna ktora w dzieciach patrzy ile dostaje strzalow sygnalem USER1
volatile sig_atomic_t running = 1;  // zmienna ktora bedzie pomagala w zarzadzaniu sygnalem alarm 
void handler(int sig){
    licznik++;
}
void alarm_handler(int sig){
    running = 0; // ustawiamy flage na 0 aby zakonczyc petle}
}
ssize_t bulk_write(int fd, char *buf, size_t count) // wpisujemy to pliku o filedeskrytporze fd , rzeczy z buffora , count rzeczy
{
    ssize_t c;
    ssize_t len = 0;

    do
    {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return c;               // błąd write

        buf   += c;
        len   += c;
        count -= c;
    } while (count > 0);

    return len;
}
void children_work(int n){
    srand(time(NULL)*getpid()); // kazde dziecko ma inny seed
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    if (sigaction(SIGUSR1, &sa, NULL) < 0)
        ERR("sigaction:");
    
    int s = (10+rand()%90 + 1)*1024; // 10kb - 100 kb
    char* buffer = malloc(s);
    if(buffer == NULL)
        ERR("malloc:");
    memset(buffer, '0'+n, s); // wypelniamy bufor odpowiednia cyfra
    // pomysl polega na tym , sleep ma dokladnosc co do sekundy wiec \
    pierwszym wybudzeniu zostaje - sekund , pause pauzuje program \
    ( tak jak sleep az do sygnalu) tyle ze ono po prostu wraca do petli poki alarm nie zadzwonil
    alarm(1);
    while(running){
        pause(); // czekamy na sygnal
    }
    // teraz mamy wszystko co potrzebne aby zapisac plik
    char filename[20];  // to jest na stosie wiec luz , tablica statyczna z nazwa pliku
    snprintf(filename,sizeof(filename),"%d.txt",getpid());  // fajny sposob wpisywania do tablicy
    int fd=(open(filename,O_WRONLY | O_CREAT | O_APPEND,0777));
    if(fd == -1)
        ERR("open");
    int spam;
    for(int i=0;i<licznik;i++){
        spam = bulk_write(fd,buffer,s);
        write(fd,"\n",1);
    }
    close(fd);
    free(buffer); 
    printf("Child PID: %d, digit: %d, block size: %d , ile siguser1 : %d\n", getpid(), n, s,licznik);
}

int main(int argc, char* argv[])
{
    if (argc < 2)  // musimy dostac przynajmniej jeden argument
        usage(argv[0]);
    int n;
    // teraz tworzymu ze my bedziemy ignorowac SIGUSER1
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    if(sigaction(SIGUSR1, &sa, NULL) < 0)
        ERR("sigaction:");
    // teraz nadpiszemy SIGALRM aby dawal znac jak sekunda minela 
    struct sigaction sa_alarm;
    memset(&sa_alarm, 0, sizeof(sa_alarm));
    sa_alarm.sa_handler = alarm_handler;
    if(sigaction(SIGALRM, &sa_alarm, NULL) < 0)
        ERR("sigaction:");

    // tworzymy dzieci 
    for(int i=1;i<argc;i++){
        n = atoi(argv[i]);
        pid_t pid = fork();
        if (pid < 0)
            ERR("fork:");
        if (pid == 0) { // dziecko
            children_work(n);
            exit(EXIT_SUCCESS);
        }
    }
    nanosleep(&(struct timespec){0,10000000}, NULL); // dajemy chwile na ustawienie sie dzieci
    // teraz czekamy sekunde i w while co 10ms wysylamy sygnal do kazdego dziecka
    alarm(1);
    struct timespec ts = {0, 10000000};
    while(running){
        kill(0, SIGUSR1); // wysylamy sygnal do wszystkich procesow w grupie
        nanosleep(&ts, NULL); // czekamy 10ms
    }
    
    while(wait(NULL) > 0){ // czyscimy zombie
    }
    return EXIT_SUCCESS;
}