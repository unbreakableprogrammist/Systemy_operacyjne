#include <stdio.h>
#include <stdlib.h>

// env | grep TIME -> sprawdzanie w shellu czy jest jakas liczba 
int main(){

    char name[22];
  
    int x;
    char* env = getenv("TIMES");
    if(env)
        x = atoi(env);
    else 
        x=1;
    while(fgets(name,21,stdin)!= NULL){  // fgets moze wczytac tez znak nowej lini 
        for(int i=0;i<x;i++){
            printf("Hello %s\n",name);
        }
        if(putenv("RESULT=Done")!=0){
            fprintf(stderr,"putenv failed");
            return EXIT_FAILURE;
        }
         printf("%s\n", getenv("RESULT"));
    }
    if (system("env|grep RESULT") != 0) // system bierze i wpisuje to co w "" w bassha i zwraca co tam odpisali ,, zwraca -1 jesli blad 
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
        

    /*
    int setenv(const char *name, const char *value, int overwrite);

    name — nazwa zmiennej (C-string). Nie może zawierać znaku '=' ani być pusty ("").

    value — wartość zmiennej (C-string). (Nie należy podawać NULL — użyj unsetenv żeby usunąć.)

    overwrite — jeśli 0 i zmienna już istnieje → nic nie robi; jeśli != 0 → nadpisuje.
    */
    
    
}