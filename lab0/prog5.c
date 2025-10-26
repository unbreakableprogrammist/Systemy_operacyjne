#include <stdio.h>
#include <stdlib.h>

void usage(char *pname)
{
    fprintf(stderr, "USAGE:%s name times>0\n", pname);   // stderr - drukuj kanalem od bledow 
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    if (argc != 3)
        usage(argv[0]);
    int j = atoi(argv[2]);
    if (j == 0)
        usage(argv[0]);
    for (int i = 0; i < j; i++)
        printf("Hello %s\n", argv[1]);
    return EXIT_SUCCESS;
}
