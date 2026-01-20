#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h> 

#include "common.h"

typedef struct ending_thread
{
    int* do_work;
    pthread_mutex_t* mtx;
}ending_thread;

void *ending_thread_work(void* args){
    ending_thread* arg = (ending_thread*)args;
    sigset_t set;
    int sig;
    sigemptyset(&set);
    sigaddset(&set,SIGINT);
    if(sigwait(&set,&sig)!=0) ERR("sigwait");
    if(sig==SIGINT){
        pthread_mutex_lock(arg->mtx);
        *arg->do_work = 0;
        pthread_mutex_unlock(arg->mtx);
    }
    printf("Dostarczono sygnal SIGINT \n");
    return NULL;
}

typedef struct pole{
    int worki;
    int wysolone_iguery;
    pthread_mutex_t mtx; // mtx na worki
    pthread_cond_t cv;
}pole;

typedef struct arg_tragarza{
    int id;
    int N; // liczba pol
    pole* lista_pol;
    int* do_work;
    pthread_mutex_t* do_work_mtx;
    sem_t* semafor;
}arg_tragarza;

void *dostawcy_work(void* arg){
    arg_tragarza* args = (arg_tragarza*)arg;
    unsigned int seed = time(NULL) ^ args->id;

    while(1){
        pthread_mutex_lock(args->do_work_mtx);
        if(*args->do_work == 0){
            pthread_mutex_unlock(args->do_work_mtx);
            break;
        }
        pthread_mutex_unlock(args->do_work_mtx);
        sem_wait(args->semafor);
        msleep(5);
        sem_post(args->semafor);
        int plot_idx = rand_r(&seed) % args->N;
        msleep(5 + plot_idx);
        pole* wylosowane = &args->lista_pol[plot_idx];
        pthread_mutex_lock(&wylosowane->mtx);
        wylosowane->worki +=5;
        pthread_mutex_unlock(&wylosowane->mtx);
        pthread_cond_signal(&args->lista_pol[plot_idx].cv); // budzimy robotnika odpowiedzialnego za pole 
    }
    return NULL;
}
typedef struct arg_robotnik{
    pole* jego_pole;
    int id;
    int* do_work;
    pthread_mutex_t* do_work_mtx;
}arg_robotnik;


void* robotnicy_work(void* arg){
    arg_robotnik* args =(arg_robotnik*)arg;
    pole* jego_pole = args->jego_pole; 
    // robotnik 
    // jesli nie ma workow uzywamy timedwait ( taki wait co sie budzi co iles czasu)
    // jesli jest to bierzemy worek i spimy 15ms
    // timespec potrzebny do trydwaita

   while(1) {
        pthread_mutex_lock(&jego_pole->mtx);
        while(jego_pole->worki == 0) { // jesli mamy 0 workow to ;
            // Sprawdzenie flagi zakończenia sprawdamy czy nie koniec
            pthread_mutex_lock(args->do_work_mtx);
            int keep_working = *args->do_work;
            pthread_mutex_unlock(args->do_work_mtx);
            if (!keep_working) {
                pthread_mutex_unlock(&jego_pole->mtx);
                return NULL;
            }

            // zasypiamy na 100 ms , budzimy sie pozniej
            struct timespec ts = get_cond_wait_time(100); 
            int res = pthread_cond_timedwait(&jego_pole->cv, &jego_pole->mtx, &ts);
            if(res == ETIMEDOUT){
                printf("SERVVS %d: EXSPECTO SALEM\n", args->id); 
            }
        }
        jego_pole->worki--;
        pthread_mutex_unlock(&jego_pole->mtx);
          
        if (jego_pole->wysolone_iguery >= 50) {
            printf("SERVVS %d: Koniec roboty \n", args->id); 
            pthread_mutex_unlock(&jego_pole->mtx);
            break; 
        }
        msleep(15); 
        pthread_mutex_lock(&jego_pole->mtx);
        jego_pole->wysolone_iguery++;
        printf("SERVVS %d: ma wysolone pole:  %d\n", args->id, jego_pole->wysolone_iguery);
      
        pthread_mutex_unlock(&jego_pole->mtx);
        pthread_mutex_lock(args->do_work_mtx);
        int keep_working = *args->do_work;
        pthread_mutex_unlock(args->do_work_mtx);
        if (!keep_working) {
            break;
        }
    }
    return NULL;
}

int main(int argc, char* argv[])
{
    srand(time(NULL));
    if(argc != 3) usage(argc,argv);
    int N = atoi(argv[1]);
    int Q = atoi(argv[2]);
    if(N<1 || N>20 || Q<1 || Q>10) usage(argc,argv);

    int do_work = 1;
    pthread_mutex_t do_work_mutex = PTHREAD_MUTEX_INITIALIZER;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set,SIGINT);
    if(pthread_sigmask(SIG_BLOCK,&set,NULL)!=0) ERR("pthread sigmask");
    ending_thread arg;
    arg.do_work = &do_work;
    arg.mtx = &do_work_mutex;
    sem_t semafor;
    if(sem_init(&semafor,0,3)!=0) ERR("semafor init");

    pthread_t ending_thread;
    if(pthread_create(&ending_thread,NULL,ending_thread_work,&arg)!= 0) ERR("pthread create");
    pthread_detach(ending_thread);
    
    // robimy pola
    printf("Start symulacji: %d dzialek, %d tragarzy.\n", N, Q);
    pole pola[N]; // inicjalizacja pol
    pthread_t dostawcy[Q];
    arg_tragarza dla_tragarzy[Q];
    for(int i=0;i<N;i++){
        pola[i].wysolone_iguery = 0;
        pola[i].worki = 0;
        if(pthread_mutex_init(&pola[i].mtx, NULL)!=0) ERR("pthread mutex init ");
        if(pthread_cond_init(&pola[i].cv,NULL)!=0) ERR("pthread cond init");
    }

      // robimy robotnikow 
    pthread_t robotnicy[N];
    arg_robotnik dla_robotnikow[N];
    for(int i=0;i<N;i++){
        dla_robotnikow[i].do_work = &do_work;
        dla_robotnikow[i].do_work_mtx = &do_work_mutex;
        dla_robotnikow[i].jego_pole = &pola[i];
        dla_robotnikow[i].id = i;
        if(pthread_create(&robotnicy[i],NULL,robotnicy_work,&dla_robotnikow[i])!=0) ERR("pthread create");
    }

    // robimy tragarzy
    for(int i=0;i<Q;i++){
        dla_tragarzy[i].do_work = &do_work;
        dla_tragarzy[i].do_work_mtx = &do_work_mutex;
        dla_tragarzy[i].lista_pol = pola;
        dla_tragarzy[i].N = N;
        dla_tragarzy[i].id = i;
        dla_tragarzy[i].semafor = & semafor;
        if(pthread_create(&dostawcy[i],NULL,dostawcy_work,&dla_tragarzy[i])!=0) ERR("pthread create");
    }
  
    while(1) {
        int ukonczone_pola = 0;
        for(int i = 0; i < N; i++) {
            pthread_mutex_lock(&pola[i].mtx);
            if(pola[i].wysolone_iguery >= 50) ukonczone_pola++;
            pthread_mutex_unlock(&pola[i].mtx);
        }

        if(ukonczone_pola == N) {
            printf("SCIPIO: VENI, VIDI, VICI, CONSPERSI!\n");
            
            // Kończymy pracę tragarzy
            pthread_mutex_lock(&do_work_mutex);
            do_work = 0; 
            pthread_mutex_unlock(&do_work_mutex);
            break;
        }

        // Sprawdzamy czy ktoś nie wcisnął Ctrl+C w międzyczasie
        pthread_mutex_lock(&do_work_mutex);
        if(do_work == 0) {
            pthread_mutex_unlock(&do_work_mutex);
            break;
        }
        pthread_mutex_unlock(&do_work_mutex);

        msleep(100); // Czekamy chwilę przed kolejnym sprawdzeniem
        if(ukonczone_pola == N || do_work == 0) {
            pthread_mutex_lock(&do_work_mutex);
            do_work = 0; 
            pthread_mutex_unlock(&do_work_mutex);

            // --- ETAP 4: "BROADCAST" DLA SEMAFORA ---
            // Budzimy wszystkich tragarzy, którzy mogą spać na sem_wait
            for(int i = 0; i < Q; i++) {
                sem_post(&semafor);
            }
            break;
        }
    }


    // 2. Skoro ending_thread wrócił, to znaczy że było Ctrl+C i do_work=0
    // Teraz czekamy aż tragarze dokończą pracę i wrócą
    for(int i=0; i<Q; i++){
        pthread_join(dostawcy[i], NULL);
    }

    for(int i=0; i<N; i++){
        pthread_join(robotnicy[i], NULL);
    }

    // Podsumowanie
    int suma = 0;
    for(int i=0; i<N; i++){
        suma += pola[i].worki;
        pthread_mutex_destroy(&pola[i].mtx);
    }
    printf("Koniec. Lacznie dostarczono %d workow soli.\n", suma);

    pthread_mutex_destroy(&do_work_mutex);
    return 0;
}
