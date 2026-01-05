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

void* thrower(void* arg){
    int id = *((int*)arg);
    free(arg);
}

int main(int argc,char **argv){
    if(argc != 3 ) ERR(" nie podano 2 argumentow ");
    int k = atoi(argv[1]);
    int n = atoi(argv[2]);
    ball_pool = n;
    
    pthread_attr_t atrybuty; // maska ktora mowi czy watek ma byc JOINABLE(trzeba go zjojnowac) , czy DETACHABLE(sam sie joinuje)
    if(pthread_attr_init(&atrybuty)!=0) ERR(" inicjacja atrybutu ");
    if(pthread_attr_setdetachstate(&atrybuty,PTHREAD_CREATE_DETACHED)!=0) ERR("ustawienie atrybutu na tworz detachable");
    // teraz mamy maske na tworzenie atrybutu 
    for(int i=0;i<11;i++){
        deska[i]=0;
        pthread_mutex_init(&mutexy[i],NULL);
    }
    for(int i=0;i<k;i++){
        int* arg = malloc(sizeof(int));
        *arg = i;
        pthread_t tid;
        if(pthread_create(tid,&atrybuty,thrower,arg)!=0) ERR("pthread create");
    }
    // sprzatamy po sobie ( maska juz nie potrzebna )
    pthread_attr_destroy(&atrybuty);
    while(1){
        sleep(1);
        pt
    }
}