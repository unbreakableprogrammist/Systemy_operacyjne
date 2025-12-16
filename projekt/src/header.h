#ifndef HEADER
#define HEADER
// biblioteki
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Bufor na ścieżki - zostawiamy systemowy standard
#define MAX_PATH PATH_MAX
// Bufor na linię komend - MUSI pomieścić co najmniej dwie ścieżki!
#define MAXLINE (MAX_PATH * 10 + 256)
// Maksymalna liczba argumentów w komendzie (add p1 p2 p3...)
#define MAXARGS 32
// Ile procesów backupu naraz(ile maks dzieci)
#define MAX_BACKUPS 100
// Ile podkatalogów może obserwować jeden proces?
#define MAX_WATCHES 4096

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

// struktura do przechowywania listy procesów + to co obserwuja
struct wezel
{
    pid_t pid;
    char source_path[MAX_PATH];
    char destination_path[MAX_PATH];
    struct wezel *next;
};

// Mapa do przechowywania par: deskryptor -> ścieżka -> kopia
struct Watch
{
    int wd;
    char src[PATH_MAX];
    char dst[PATH_MAX];
};
// mapa do przechowywania par: deskryptor -> ścieżka -> kopia
struct WatchMap
{
    struct Watch watch_map[MAX_WATCHES];
    int watch_count;
};

// funkcje
// main.c
void remove_newline(char *line);
void list_all(struct wezel *head);
void delete_node(struct wezel **head_ref, char *source_path, char *destination_path);
int tokenize(char *line, char *args[]);
void handler(int sig);
int is_subpath(const char *parent, const char *child);
int is_duplicate(struct wezel *head, const char *src, const char *dst);

// file_watcher_reccursive.c
int is_dir_empty(const char *path);
ssize_t bulk_write(int fd, const char *buf, size_t count);
void copy_attributes(const char *source, const char *destination);
void add_watch_to_map(int wd, const char *src, const char *dst);
void file_copy(char *source, char *destination);
void handle_link(char *source, char *destination, char *root_src, char *root_dst);
void directory_copy(char *source_path, char *destination_path);
void add_watches_recursive(int notify_fd, char *source_path, char *destination_path);
struct Watch *get_watch_by_wd(int wd);
void directory_delete(const char *path);
void file_delete(const char *path);
void remove_watch_from_map(int wd);
void child_handler(int sig);
void file_watcher_reccursive(char *source_path, char *destination_path);
void recover(char *source_path, char *destination_path);
#endif
