#define _XOPEN_SOURCE 500 // lub 600, lub 700
#include <dirent.h>
#include <errno.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAXFD 20

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
int dirs = 0,files =0,links=0,other=0;
int walk(char* path,const struct stat *sb,int type,struct FTW *f){
    if(type==FTW_F)files++;
    else if(type==FTW_D)dirs++;
    else if(type==FTW_SL)links++;
    else if(type==FTW_DP);
    else other++;
    return 0;
}

int main(int argc,char** argv){
    for(int i=1;i<argc;i++){
            /*
            walk dziala tak ze jak jestesmy
            Jeśli wywołasz nftw("MojeRzeczy", walk, ...) bez flagi FTW_DEPTH (czyli domyślnie), kolejność będzie taka:

    nftw znajduje katalog MojeRzeczy.

    nftw wywołuje Twoją funkcję: walk("MojeRzeczy", ..., FTW_D, ...)

    Twój kod wykonuje dirs++ (bo type == FTW_D).

    nftw wchodzi do środka MojeRzeczy.

    nftw znajduje plik_A.txt.

    nftw wywołuje Twoją funkcję: walk("MojeRzeczy/plik_A.txt", ..., FTW_F, ...)

    Twój kod wykonuje files++.

    nftw znajduje Podkatalog.

    nftw wywołuje Twoją funkcję: walk("MojeRzeczy/Podkatalog", ..., FTW_D, ...)

    Twój kod wykonuje dirs++.

    nftw wchodzi do środka Podkatalog.

    nftw znajduje plik_B.txt.

    nftw wywołuje Twoją funkcję: walk("MojeRzeczy/Podkatalog/plik_B.txt", ..., FTW_F, ...)

    Twój kod wykonuje files++.

.   ..i tak dalej, aż skończy.

            Krótko mówiąc: nftw "prowadzi" Cię za rękę do każdego miejsca, a Twoja funkcja walk po prostu "patrzy" co to za miejsce (type) i "odnotowuje" to (np. dodając +1 do licznika).
            */
            if (nftw(argv[i], walk, MAXFD, FTW_PHYS) == 0)
                printf("%s:\nfiles:%d\ndirs:%d\nlinks:%d\nother:%d\n", argv[i], files, dirs, links, other);
            else
                printf("%s: access denied\n", argv[i]);
            dirs = files = links = other = 0;
    }
}
