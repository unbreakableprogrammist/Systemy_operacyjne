#define _XOPEN_SOURCE 700

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void ms_sleep(unsigned int milli)
{
    struct timespec ts = {milli / 1000, (milli % 1000) * 1000000L};
    nanosleep(&ts, NULL);
}

void usage(int argc, char* argv[])
{
    printf("%s N M\n", argv[0]);
    printf("\t10 <= N <= 20 - number of banner threads\n");
    printf("\t2 <= N <= 8 - number of artillery threads\n");
    exit(EXIT_FAILURE);
}

typedef struct do_chorogwi
{
    int* stop;
    int* mud;
    sem_t* semafor;
    int id;
    pthread_cond_t* cv;
    int* enemy_hp;
    pthread_mutex_t* mtx;
    pthread_barrier_t* szarze;
    sem_t* semafor_tabor;
} do_chorogwi;

typedef struct do_artyleri
{
    int* stop;
    int id;
    pthread_barrier_t* barriera;
    int* enemy_hp;
    pthread_cond_t* cv;
    pthread_mutex_t* mtx;
    int* work;
} do_artyleri;

void* chorowgwie_work(void* argument)
{
    do_chorogwi* arg = (do_chorogwi*)argument;
    // if()
    sem_wait(arg->semafor);
    unsigned int seed = time(NULL) ^ arg->id;
    unsigned int rand_time = 80 + rand_r(&seed) % 171;
    ms_sleep(rand_time);
    sem_post(arg->semafor);
    printf("CAVALRY %d IN POSITION\n", arg->id);

    pthread_mutex_lock(arg->mtx);
    while (*arg->enemy_hp >= 50)
    {
        pthread_cond_wait(arg->cv, arg->mtx);
    }
    pthread_mutex_unlock(arg->mtx);
    printf("CAVALRY %d READY TO CHARGE\n", arg->id);

    // teraz beada szarze
    for (int i = 0; i < 3; i++)
    {
        if (arg->id == 0)
            printf("CHARGEEE %d\n", i);
        ms_sleep(500);
        pthread_barrier_wait(arg->szarze);

        int miss = rand_r(&seed);
        if (miss < 10 && arg->mud)
            miss = 1;
        else
            miss = 0;

        pthread_mutex_lock(arg->mtx);
        if (!miss)
            *arg->enemy_hp -= 1;
        else
            printf("CAVALRY MISSED");
        pthread_mutex_unlock(arg->mtx);
        pthread_barrier_wait(arg->szarze);

        sem_wait(arg->semafor_tabor);
        ms_sleep(100);
        sem_post(arg->semafor_tabor);
        printf("CAVALRY %d LANCE RESTOKED \n", arg->id);

        pthread_barrier_wait(arg->szarze);
    }
    return NULL;
}

void* artillery_work(void* argument)
{
    do_artyleri* arg = (do_artyleri*)argument;
    int pracowac = *arg->work;
    while (pracowac)
    {
        pthread_mutex_lock(arg->mtx);
        int left_hp = *arg->enemy_hp;
        pthread_mutex_unlock(arg->mtx);
        unsigned int seed = time(NULL) ^ arg->id;
        int random_shot = 1 + rand_r(&seed) % 6;
        pthread_mutex_lock(arg->mtx);
        *arg->enemy_hp -= random_shot;
        pthread_mutex_unlock(arg->mtx);
        int wybraniec = pthread_barrier_wait(arg->barriera);
        if (wybraniec == PTHREAD_BARRIER_SERIAL_THREAD)
        {
            pthread_mutex_lock(arg->mtx);
            left_hp = *arg->enemy_hp;
            pthread_mutex_unlock(arg->mtx);
            printf("ARTILLERY : ENEMY HP: %d\n", left_hp);
        }
        if (left_hp < 50)
        {
            pthread_mutex_lock(arg->mtx);
            *arg->work = 0;
            pthread_mutex_unlock(arg->mtx);
        }
        pthread_barrier_wait(arg->barriera);
        pracowac = *arg->work;
        pthread_barrier_wait(arg->barriera);
    }
    pthread_barrier_wait(arg->barriera);
    if (pthread_cond_broadcast(arg->cv) != 0)
        ERR("pthread cond broadcast");
    return NULL;
}

typedef struct ending_signal
{
    int* stop;
    int* mud;
} ending_signal;

void* signal_thread_work(void* argument)
{
    ending_signal* es = (ending_signal*)argument;
    sigset_t set;
    int sig;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGUSR1);
    if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0)
        ERR("sigmask");

    while (1)
    {
        if (sigwait(&set, &sig) != 0)
            ERR("sigwait");
        if (sig == SIGINT)
        {
            break;
        }
        if (sig == SIGUSR1)
        {
            *es->mud = 1;
        }
    }
    // to oznacza ze byl sigint
    *es->stop = 1;
    return NULL;
}

int main(int argc, char* argv[])
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGUSR1);
    if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0)
        ERR("sigmask");
    if (argc != 3)
        ERR("invalid number od arguments");
    int N = atoi(argv[1]);
    int M = atoi(argv[2]);

    int stop = 0;
    int mud = 0;
    ending_signal do_watku;
    do_watku.mud = &stop;
    do_watku.mud = &mud;
    pthread_t signal_thread;
    if (pthread_create(&signal_thread, NULL, signal_thread_work, &do_watku) != 0)
        ERR("pthread create");
    pthread_detach(signal_thread);

    sem_t semafor;
    sem_t semafor_tabor;
    pthread_cond_t cv;
    pthread_mutex_t mtx;
    pthread_barrier_t barriera;
    pthread_barrier_t szarze;
    if (pthread_barrier_init(&barriera, NULL, M) != 0)
        ERR("barrier init");
    if (pthread_barrier_init(&szarze, NULL, N) != 0)
        ERR("barrier init");
    if (pthread_mutex_init(&mtx, NULL) != 0)
        ERR("pthread mutex init ");
    if (pthread_cond_init(&cv, NULL) != 0)
        ERR("cond variable init");
    if (sem_init(&semafor, 0, 3) < 0)
        ERR("sem_init");
    if (sem_init(&semafor_tabor, 0, 4) < 0)
        ERR("sem_init");
    do_chorogwi args[N];
    pthread_t chorogwie[N];
    int enemy_hp = 100;
    for (int i = 0; i < N; i++)
    {
        args[i].id = i;
        args[i].semafor = &semafor;
        args[i].cv = &cv;
        args[i].mtx = &mtx;
        args[i].enemy_hp = &enemy_hp;
        args[i].szarze = &szarze;
        args[i].semafor_tabor = &semafor_tabor;
        args[i].mud = &mud;
        args[i].stop = &stop;
        if (pthread_create(&chorogwie[i], NULL, chorowgwie_work, &args[i]) != 0)
            ERR("pthread create");
    }

    sleep(1);  // zeby ladniej sie wypisywalo :) , najepierw kawaleria dociera , pozniej artyleria strzela , na koncu
               // artyleria charguje
    //  teraz artyleria
    do_artyleri arta[M];
    pthread_t artyleria[M];
    int work_artylery = 1;
    for (int i = 0; i < M; i++)
    {
        arta[i].barriera = &barriera;
        arta[i].id = i;
        arta[i].cv = &cv;
        arta[i].enemy_hp = &enemy_hp;
        arta[i].mtx = &mtx;
        arta[i].work = &work_artylery;
        arta[i].stop = &stop;
        // arta[i].stop = &stop;
        if (pthread_create(&artyleria[i], NULL, artillery_work, &arta[i]) != 0)
            ERR("pthread create");
    }

    // joinowanie watkow
    for (int i = 0; i < N; i++)
    {
        pthread_join(chorogwie[i], NULL);
    }

    for (int i = 0; i < M; i++)
    {
        pthread_join(artyleria[i], NULL);
    }
    printf("BATTLE ENDED. ENEMY HEALTH : %d\n", enemy_hp);
    if (enemy_hp <= 0)
    {
        printf("VENI VIDI VICI\n");
    }

    if (pthread_barrier_destroy(&szarze) != 0)
        ERR("pthread barrier");
    if (pthread_cond_destroy(&cv) != 0)
        ERR("cond destroy");
    if (pthread_mutex_destroy(&mtx) != 0)
        ERR("mutex destroy");
    if (sem_destroy(&semafor) != 0)
        ERR("semafor destroy");
    return EXIT_SUCCESS;
}
