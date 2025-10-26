#include "hello.h"

int main(){
    // int liczba;
    // scanf("%d", &liczba); // &liczba - wskaźnik na zmienną liczba
    // printf("Hello World! %d\n", liczba);

    char name[22];
    // scanf("%21s", name); // %21s - maksymalnie 21 znaków + \0
    // if (strlen(name)>20) ERR("Too long name");
    // printf("Hello %s!\n", name);

    while(fgets(name,21,stdin)!= NULL){  // fgets moze wczytac tez znak nowej lini 
        printf("Hello %s",name);
        
    }

    /*
    mozna to odpalic albo make prog3 
    albo ./prog3 < dane.txt 
    albo cat dane.txt | ./prog3:
    Operator |

    To jest potok (pipe).
    Przekierowuje standardowe wyjście programu po lewej stronie (cat)
    do standardowego wejścia (stdin) programu po prawej (./prog3).

    Czyli:
    stdout(cat) ───────→ stdin(prog3)
    */
    
}