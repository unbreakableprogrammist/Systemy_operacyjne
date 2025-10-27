#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(char *pname)
{
    fprintf(stderr, "USAGE:%s -n Name -p OCTAL -s SIZE\n", pname); // Jeśli program nazywa się ./program, ta linia wypisze na konsolę: USAGE:./program -n Name -p OCTAL -s SIZE
    /*
    // exit - natychmiast zatrzymuje program ,
     EXIT_FAILURE: To specjalna stała (zazwyczaj o wartości 1), która jest zwracana do systemu operacyjnego.
     Informuje ona system, że program zakończył się błędem (w tym przypadku: błędem użytkownika, który źle podał argumenty
    */
    exit(EXIT_FAILURE); 
}
/*
    r - plik udostępnia czytanie danych,
    w lub w+ - plik zostaje skrócony do zera (lub stworzony) i udostępnia pisanie danych,
    a lub a+ - plik udostępnia dopisywanie danych do końca jego istniejącej treści.
    r+ - plik udostępnia czytanie oraz pisanie danych.
*/
void make_file(char* name,mode_t perms,ssize_t size,int procent){
    FILE* s1; // to co otworzy nam fopen
    umask(~perms & 0777); // umaska to jakby obiera uprawnienia , wiec musimy zabrac -perma 
    if((s1 = fopen(name,"w+")) == NULL){
        ERR("fopen");
    }
    for(int i=0;i<(size*procent)/100;i++){
        if(fseek(s1,rand()%size,SEEK_SET)) // bierzemy plik , przesuwamy sie o rand%size bajtow ( czyli znakow), od poczatku
            ERR("fseek");
        fprintf(s1, "%c", 'A' + (i % ('Z' - 'A' + 1)));
    }
    if (fclose(s1))
        ERR("fclose");
}

int main(int argc,char** argv){
    int c;
    char* name = NULL;
    mode_t perms=-1; // mode_t to oficjalny, standardowy typ danych POSIX przeznaczony specjalnie do przechowywania uprawnień plików
    ssize_t size = -1; // To jest "podpisany typ rozmiaru" (signed size type).ssize moze byc -1 , w przeciwienstwie do s
    while((c=getopt(argc,argv,"n:p:s:"))!=-1){ // getopt bierze i jesli widzi flage to zapisuje flaga -> co po tym ( taki ala slownik), jak nie ma nic tp zwraca -1
        if(c=='n') name=optarg;
        else if(c=='p') perms = strtol(optarg,(char**)NULL,8); //drugi argument to wskaznik na to poczatek tam gdzie konczy sie liczba
        else if(c=='s') size = strtol(optarg,(char**)NULL,10);
    }
    if ((NULL == name) || ((mode_t)-1 == perms) || (-1 == size)) // tutaj (mode_t) to rzutowanie np
        usage(argv[0]);
    /*
    unlink(name)

    Ta funkcja próbuje usunąć plik o nazwie name.

    Jeśli się uda, zwraca 0 (co w if oznacza fałsz).

    Jeśli się nie uda, zwraca -1 (co w if oznacza prawdę) i ustawia globalną zmienną errno na kod błędu.
    */
    if (unlink(name) && errno != ENOENT) 
        ERR("unlink");
    srand(time(NULL));
    make_file(name, size, perms, 10);
    return EXIT_SUCCESS;
}

