#ifndef HEADER
#define HEADER

#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <sys/inotify.h>

// Bufor na ścieżki - zostawiamy systemowy standard
#define MAX_PATH PATH_MAX 
// Bufor na linię komend - MUSI pomieścić co najmniej dwie ścieżki!
#define MAXLINE (MAX_PATH * 2 + 256) 
// Maksymalna liczba argumentów w komendzie (add p1 p2 p3...)
#define MAXARGS 32
// Ile procesów backupu naraz? (ile razy fork)
#define MAX_BACKUPS 64
// Ile podkatalogów może obserwować jeden proces?
#define MAX_WATCHES 4096 

#define EVENT_BUF_LEN                                                          \
  (64 * (sizeof(struct inotify_event) + NAME_MAX + 1)) // Bufor inotify
#define ERR(source)                                                            \
  (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source),             \
   kill(0, SIGKILL), exit(EXIT_FAILURE))


void file_watcher_reccursive(char *source_path, char *destination_path);

struct wezel {
  pid_t pid;
  char source_path[MAX_PATH];
  char destination_path[MAX_PATH];
  struct wezel *next;
};

// Mapa do przechowywania par: deskryptor -> ścieżka
struct WatchMap {
    int wd;
    char path[PATH_MAX];
};
#endif