#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
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

volatile sig_atomic_t sig_count = 0;
void handler(int sig)
{
    sig_count ++;
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
void child_work(int m)
{
    struct timespec ts = {0,m*10000};
    while(1){
        nanosleep(&ts,NULL);
        if(kill(getppid(),SIGUSR1)<0)
            ERR("kill");
    }
    
}
void parent_work(int b, int s, char* name)
{
    int in, out;
    char* buf = malloc(s);
    if(buf==NULL)
        ERR("malloc");
    if((out = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0777)) < 0)
        ERR("open output file");
    if((in = open("/dev/urandom", O_RDONLY)) < 0)
        ERR("open input file");  
    ssize_t count;
    for(int i=0;i<b;++i){
        if((count=read(in,buf,s))<0)
            ERR("read");
        if(count=write(out,buf,count)<0)
            ERR("write");
        if (fprintf(stderr, "Block of %ld bytes transferred. Signals RX:%d\n", count, sig_count) < 0)
            ERR("fprintf");
    }
    close(in);
    close(out);
    free(buf);

}

int main(int argc, char **argv)
{
    if(argc!=5)
        usage(argv[0]);
    int m = atoi(argv[1]); // co ile SIGUSER1
    int b = atoi(argv[2]);
    int s = atoi(argv[3]);
    char* name = argv[4];
    if (m <= 0 || m > 999 || b <= 0 || b > 999 || s <= 0 || s > 999)
        usage(argv[0]);
    
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigaction(SIGUSR1, &sa, NULL);
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    sigaction(SIGCHLD, &sa, NULL);

    pid_t pid = fork();
    if (pid < 0)
        ERR("fork");
    if (pid == 0)
    {
        // child
        child_work(m);
        exit(EXIT_SUCCESS);
    }else{
        // parent
        parent_work(b, s, name);
        while (wait(NULL) > 0)
        {
        }
    }
    return EXIT_SUCCESS;
}