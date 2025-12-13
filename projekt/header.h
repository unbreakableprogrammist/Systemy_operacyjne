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

#define MAX_BACKUPS 100 // Maksymalna liczba aktywnych kopii
#define MAX_PATH PATH_MAX
#define EVENT_BUF_LEN                                                          \
  (64 * (sizeof(struct inotify_event) + NAME_MAX + 1)) // Bufor inotify
#define ERR(source)                                                            \
  (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source),             \
   kill(0, SIGKILL), exit(EXIT_FAILURE))

#define MAXLINE 1024
#define MAXARGS 64
void file_watcher_reccursive(char *source_path, char *destination_path);
#endif