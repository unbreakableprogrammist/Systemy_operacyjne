#ifndef HELLO_H
#define HELLO_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define ERR(source) (perror(source),\
fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),\
exit(EXIT_FAILURE))

void hello(void);

#endif /* HELLO_H */

/* Header file declaring the hello() function */