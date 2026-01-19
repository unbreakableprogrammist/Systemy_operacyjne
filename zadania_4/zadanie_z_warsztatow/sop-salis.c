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
    pthread_mutex_t mtx;
}pole;

typedef struct arg_tragarza{
    int id;
    int N; // liczba pol
    pole* lista_pol;
    int* do_work;
    pthread_mutex_t* do_work_mtx;
}arg_tragarza;

void *dostawcy_work(void* arg){
    arg_tragarza* args = (arg_tragarza*)arg;
    unsigned int seed = time(NULL) ^ pthread_self();

    while(1){
        pthread_mutex_lock(args->do_work_mtx);
        if(*args->do_work == 0){
            pthread_mutex_unlock(args->do_work_mtx);
            break;
        }
        pthread_mutex_unlock(args->do_work_mtx);

        int plot_idx = rand_r(&seed) % args->N;
        msleep(5 + plot_idx);
        pole* wylosowane = &args->lista_pol[plot_idx];
        pthread_mutex_lock(&wylosowane->mtx);
        wylosowane->worki +=5;
        pthread_mutex_unlock(&wylosowane->mtx);
    }
}

int main(int argc, char* argv[])
{
    srand(time(NULL));
    if(argc != 3) usage(argv);
    int N = atoi(argv[1]);
    int Q = atoi(argv[2]);
    if(N<1 || N>20 || Q<1 || Q>10) usage(argv);

    int do_work = 1;
    pthread_mutex_t do_work_mutex = PTHREAD_MUTEX_INITIALIZER;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set,SIGINT);
    if(pthread_sigmask(SIG_BLOCK,&set,NULL)!=0) ERR("pthread sigmask");
    ending_thread arg;
    arg.do_work = &do_work;
    arg.mtx = &do_work_mutex;

    pthread_t ending_thread;
    if(pthread_create(&ending_thread,NULL,ending_thread_work,&arg)!= 0) ERR("pthread create");
    
    // tereaz tragarze

    printf("Start symulacji: %d dzialek, %d tragarzy.\n", N, Q);
    pole pola[N];
    pthread_t dostawcy[Q];
    arg_tragarza dla_tragarzy[Q];
    for(int i=0;i<N;i++){
        pola[i].wysolone_iguery = 0;
        pola[i].worki = 0;
        if(pthread_mutex_init(&pola[i].mtx, NULL)!=0) ERR("pthread mutex init ");
    }

    for(int i=0;i<Q;i++){
        dla_tragarzy[i].do_work = &do_work;
        dla_tragarzy[i].do_work_mtx = &do_work_mutex;
        dla_tragarzy[i].lista_pol = pola;
        dla_tragarzy[i].N = N;
        dla_tragarzy[i].id = i;
        
        if(pthread_create(&dostawcy[i],NULL,dostawcy_work,&dla_tragarzy[i])!=0) ERR("pthread create");
    }

    pthread_join(ending_th, NULL);

    // 2. Skoro ending_thread wrócił, to znaczy że było Ctrl+C i do_work=0
    // Teraz czekamy aż tragarze dokończą pracę i wrócą
    for(int i=0; i<Q; i++){
        pthread_join(dostawcy[i], NULL);
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
