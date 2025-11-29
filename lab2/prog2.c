#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t last_signal = 0; // ostatni sygnal ( jakie dzieci beda dostawac )

void usage(void)
{
    fprintf(stderr, "USAGE: signals n k p l\n");
    fprintf(stderr, "n - number of children\n");
    fprintf(stderr, "k - Interval before SIGUSR1\n");
    fprintf(stderr, "p - Interval before SIGUSR2\n");
    fprintf(stderr, "l - lifetime of child in cycles\n");

    exit(EXIT_FAILURE);
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
    
}   