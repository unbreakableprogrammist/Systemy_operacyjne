#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))


/*
pomysl polega na tym , kazdy watek dostaje w miare rowno kolek do zrzucenia ,robimy te watki deteachable , czyli nie musimy ich joinowac 
robimy tez sobie dla kazdej komorki mutexa , ktory bedzie blokowal na czas wpisywania do komorki ze kulka tu spadla 
*/

int deska[11]; // tablica zliczajaca ile wpadlo do kazdej komorki kulek
pthread_mutex_t mutexy[11]; // tablica trzymajaca mutexy 
int ball_pool; // ilosc kulek do zrzucenia
pthread_mutex_t ile_kul;

void throw_ball(int id){
    int position=0;
    int seed = time(NULL) ^ id;
    for(int i=0;i<10;i++){
        int skok = rand_r(&seed) % 2;
        if(skok) position++;
    }
    pthread_mutex_lock(&mutexy[position]);
    deska[position]++;
    pthread_mutex_unlock(&mutexy[position]);
}

void* thread_work(void* arg){
    int id = *((int*)arg);
    free(arg);
    while(1) {
        pthread_mutex_lock(&ile_kul);
        if(ball_pool > 0) {
            ball_pool--; // BIERZEMY KULKĘ OD RAZU!
            pthread_mutex_unlock(&ile_kul);
        } else {
            pthread_mutex_unlock(&ile_kul);
            break; // Nie ma kulek, kończymy pracę
        }
        
        throw_ball(id); // Rzucamy kulkę 
    }
    return NULL;
}

int main(int argc,char **argv){
    if(argc != 3 ) ERR(" nie podano 2 argumentow ");
    int k = atoi(argv[1]);
    int n = atoi(argv[2]);
    ball_pool = n;
    if(pthread_mutex_init(&ile_kul,NULL)) ERR("init mutexa");
    pthread_attr_t atrybuty; // maska ktora mowi czy watek ma byc JOINABLE(trzeba go zjojnowac) , czy DETACHABLE(sam sie joinuje)
    if(pthread_attr_init(&atrybuty)!=0) ERR(" inicjacja atrybutu ");
    if(pthread_attr_setdetachstate(&atrybuty,PTHREAD_CREATE_DETACHED)!=0) ERR("ustawienie atrybutu na tworz detachable");
    // teraz mamy maske na tworzenie atrybutu 
    for(int i=0;i<11;i++){
        deska[i]=0;
        if(pthread_mutex_init(&mutexy[i],NULL)) ERR("init mutex");
    }
    for(int i=0;i<k;i++){
        int* arg = malloc(sizeof(int));
        *arg = i;
        pthread_t tid;
        if(pthread_create(&tid,&atrybuty,thread_work,arg)!=0) ERR("pthread create");
    }
    // sprzatamy po sobie ( maska juz nie potrzebna )
    pthread_attr_destroy(&atrybuty);
    
    int runnig = 1;
    while(runnig){
        sleep(1);
        int suma_kulek = 0;
        for(int i=0; i<11; i++){
            pthread_mutex_lock(&mutexy[i]);
            suma_kulek += deska[i];
            pthread_mutex_unlock(&mutexy[i]);
        }
        if(suma_kulek >= n){
            runnig = 0;
        }
    }
    printf("Nasza deska :\n");
    for(int i=0;i<11;i++){
        printf("%d ",deska[i]);
    }
    printf("\n");
}