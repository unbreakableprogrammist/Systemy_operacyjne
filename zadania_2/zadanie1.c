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
int main(int argc, char* argv[])
{
    if (argc < 2)  // musimy dostac przynajmniej jeden argument
        usage(argv[0]);
    int n;
    for(int i=1;i<=argc;i++){
        n = atoi(argv[i]);
    }
    return EXIT_SUCCESS;
}