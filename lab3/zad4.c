#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

/* 
pomysl jest taki : twrzymuy na poczatku n watkow i odpalamy dla kazdego watek , pozniej tam co sekunde bedzie sie zwiekszal rok az nie dojdzie do 4
watek glowny zapisze sobie wszyskie watki i bedzie co randomowy czas losowal jakiegos i zapisywal ze jest wyrzucony 
wtedy wysylamy pthread cancel a w watkach zrobimy pthread_cleanup_push(decrement_counter, &args); zeby jak bedzie anulowany to zeby wykonal ta funckje 
*/
// to roznie dobrze mozna zrobic na zmiennych globalnych 
typedef struct studenci{
   int uczniowie[5];
   int wyrzuceni[5];
} studenci;

typedef struct do_decrementa{
    studenci* args;
    int year;
} do_decrementa;

pthread_mutex_t mutexy_uczniowie[5];
pthread_mutex_t mutexy_wyrzuceni[5];

void decrement_counter(void* arg){
    do_decrementa* dane = (do_decrementa*) arg;
    pthread_mutex_lock(&mutexy_uczniowie[dane->year]);
    dane->args->uczniowie[dane->year]--;
    pthread_mutex_unlock(&mutexy_uczniowie[dane->year]);
    pthread_mutex_lock(&mutexy_wyrzuceni[dane->year]);
    dane->args->wyrzuceni[dane->year]++;
    pthread_mutex_unlock(&mutexy_wyrzuceni[dane->year]);
    free(dane);
}


void* thread_work(void* arg){
    int year=1;
    studenci* args = (studenci*) arg;
    do_decrementa* dane = malloc(sizeof(do_decrementa));
    dane->args = args;
    dane->year = year;
    // ladujemy pierwszaka
    pthread_mutex_lock(&mutexy_uczniowie[year]);
    args->uczniowie[year]++;
    pthread_mutex_unlock(&mutexy_uczniowie[year]);

    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);// to oznacza ze anulowanie bedzie dopiero w punkcie anulowania
    while(year<4){
        dane->year = year;
        pthread_cleanup_push(decrement_counter, dane);
        sleep(1);
        pthread_cleanup_pop(0); // 0 oznacza ze nie wywolujemy cleanup teraz
        year++;
        pthread_mutex_lock(&mutexy_uczniowie[year]);
        args->uczniowie[year]++;
        pthread_mutex_unlock(&mutexy_uczniowie[year]);
        pthread_mutex_lock(&mutexy_uczniowie[year-1]);
        args->uczniowie[year-1]--;
        pthread_mutex_unlock(&mutexy_uczniowie[year-1]);
    }
    dane->year = year;
    pthread_cleanup_push(decrement_counter, dane);
    sleep(1);
    pthread_cleanup_pop(0); // 0 oznacza ze nie wywolujemy cleanup teraz
    free(dane);
    return NULL;
}

int main(int argc, char **argv) {
    if(argc != 2) ERR("zbyt malo argumentow");
    int n = atoi(argv[1]);
    studenci* argument = malloc(sizeof(studenci));
    struct timespec start,current; // strukturka do mierzenia czasu
    pthread_t tab[100]; // tablica watkow 
    int czy_wyrzucony[100]; // tablica czy ktos byl wyrzucony 
    for(int i=0;i<5;i++){
        argument->uczniowie[i]=0;
        argument->wyrzuceni[i]=0;
        if(pthread_mutex_init(&mutexy_uczniowie[i],NULL)!=0) ERR("pthread_mutex_init");
        if(pthread_mutex_init(&mutexy_wyrzuceni[i],NULL)!=0) ERR("pthread_mutex_init");
    }
    // tworzymy watki i zapisujemy je do tablicy
    for(int i=0;i<n;i++){
        czy_wyrzucony[i]=0;
        if(pthread_create(&tab[i],NULL,thread_work,argument)!=0) ERR("pthread create");
    }
   
    // mierzymy czas
    if(clock_gettime(CLOCK_REALTIME,&start)!=0) ERR("clock gettime"); // zapisujemy czas poczatkowy
    srand(time(NULL)); // inicjalizacja generatora liczb losowych
    while(1){
        if(clock_gettime(CLOCK_REALTIME,&current)!=0) ERR("clock gettime"); // bierzemy aktualny czas
        double elapesed = (current.tv_sec - start.tv_sec) + (current.tv_nsec - start.tv_nsec)/1000000000.0;
        if(elapesed>=4.0) break; // konczymy po 4 sekundach
        // losujemy co jakis czas jakiegos studenta do wyrzucenia
        int random_time = 100 + rand()%201; // losowy czas od 100ms do 400ms
        struct timespec nanosek = {0, random_time * 1000000};
        nanosleep(&nanosek,NULL);
        int random_student = rand()%n;
        while(czy_wyrzucony[random_student]==1){
            random_student = rand()%n;
        }
        czy_wyrzucony[random_student]=1;
        if(pthread_cancel(tab[random_student])!=0) ERR("pthread cancel");
        
    }
    // czekamy na zakonczenie wszystkich watkow
    for(int i=0;i<n;i++){
        if(pthread_join(tab[i],NULL)!=0) ERR("pthread join");
    }
    // wypisujemy wyniki
    for(int i=0;i<5;i++)
    {
        printf("Rok %d: Uczniowie: %d, Wyrzuceni: %d\n", i, argument->uczniowie[i], argument->wyrzuceni[i]);
        if(pthread_mutex_destroy(&mutexy_uczniowie[i])!=0) ERR("pthread_mutex_destroy");
        if(pthread_mutex_destroy(&mutexy_wyrzuceni[i])!=0) ERR("pthread_mutex_destroy");
    }
    free(argument);
    

    return EXIT_SUCCESS;
}