#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

extern char **environ;

void usage(char *pname)
{
    fprintf(stderr, "USAGE:%s [VARN_NAME VARN_VALUE] ... \n", pname);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    if (argc % 2 != 1) // blad jak nie ma parzystej liczby zmiennych 
        usage(argv[0]);

    for (int i = argc - 1; i > 0; i -= 2) // idziemy po argumentach 
    {
        if (setenv(argv[i - 1], argv[i], 1))  // bierzemy nazwe argv[i-1] i ustawiamy jako argv[i] i 1 dlatego ze zawsze nadpisujemy (  0 jesli tylko podmienamyu jesli zmiennej nie ma )
        {
            if (EINVAL == errno){ // sprawdzamy czy jesli cos jest zle 
                printf("opis błędu: %s\n", strerror(errno));
                ERR("setenv - variable name contains '='");
            }
            ERR("setenv");
        }
    }

    int index = 0;
    while (environ[index])
        printf("%s\n", environ[index++]);
    return EXIT_SUCCESS;
}