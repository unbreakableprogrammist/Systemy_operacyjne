#include "header.h"
#include <fcntl.h> 
#include <errno.h>

// ZMIANA: static rozwiązuje problem "multiple definition"
static volatile sig_atomic_t running = 1;
struct WatchMap map = {0};

ssize_t bulk_write(int fd, const char *buf, size_t count) {
    ssize_t c;
    ssize_t len = 0;

    while (count > 0) {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0) return -1;       
        if (c == 0) break;             

        buf  += c;
        len  += c;
        count -= c;
    }
    return len;
}

void add_watch_to_map(int wd, const char *src, const char *dst) {
    if (map.watch_count < MAX_WATCHES) {
        map.watch_map[map.watch_count].wd = wd;
        strncpy(map.watch_map[map.watch_count].src, src, PATH_MAX-1);
        strncpy(map.watch_map[map.watch_count].dst, dst, PATH_MAX-1);
        map.watch_count++;
    } else {
        printf("[CHILD %d] Przekroczono maksymalna liczbe watchow\n", getpid());
    }
}

void file_copy(char *source, char *destination){
    int src_fd = open(source, O_RDONLY);
    if (src_fd < 0) {
        perror("open source failed");
        return; 
    }

    int dst_fd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd < 0) {
        perror("open destination failed");
        close(src_fd); 
        return;
    }

    char buffer[4096];
    ssize_t n;
    while ((n = TEMP_FAILURE_RETRY(read(src_fd, buffer, sizeof(buffer)))) > 0) {
        if (bulk_write(dst_fd, buffer, n) < 0) {
            perror("bulk_write failed");
            break; 
        }
    }
    
    if (n < 0) perror("read source failed");

    close(src_fd);
    close(dst_fd);
    printf("[CHILD %d] Skopiowano plik %s do %s\n", getpid(), source, destination);
}

void handle_link(char* source, char* destination) {
    (void)source; (void)destination;
}

void directory_copy(char *source_path, char *destination_path) {
    struct stat st_dest;
    if (stat(destination_path, &st_dest) == -1) {
        if (mkdir(destination_path, 0755) < 0 && errno != EEXIST) {
            perror("mkdir root destination failed");
            return; 
        }
    }

    DIR *dir = opendir(source_path);
    if (!dir) {
        perror("opendir failed");
        return; 
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if ((strcmp((entry->d_name), ".")) == 0 || (strcmp((entry->d_name), "..")) == 0) {
            continue;
        }

        char next_src[PATH_MAX];
        char next_dst[PATH_MAX];
        snprintf(next_src, PATH_MAX, "%s/%s", source_path, entry->d_name);
        snprintf(next_dst, PATH_MAX, "%s/%s", destination_path, entry->d_name);
        
        struct stat st;
        if (lstat(next_src, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                if (mkdir(next_dst, st.st_mode & 0777) < 0 && errno != EEXIST) {
                    perror("mkdir recursive failed");
                    continue; 
                }
                directory_copy(next_src, next_dst);
            } else if (S_ISREG(st.st_mode)) {
                file_copy(next_src, next_dst);
            } else if (S_ISLNK(st.st_mode)) {
                handle_link(next_src, next_dst);
            }
        } else {
            perror("lstat failed");
        }
    }
    closedir(dir);
}

void add_watches_recursive(int notify_fd, char *source_path, char *destination_path) {
    int wd = inotify_add_watch(notify_fd, source_path, IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_TO | IN_MOVED_FROM | IN_DELETE_SELF);
    
    if (wd < 0) {
        perror("inotify_add_watch failed");
    } else {
        add_watch_to_map(wd, source_path, destination_path);
    }

    DIR *dir = opendir(source_path);
    if (!dir) {
        perror("opendir recursive scan failed");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if ((strcmp((entry->d_name), ".")) == 0 || (strcmp((entry->d_name), "..")) == 0) {
            continue;
        }
        
        char next_src[PATH_MAX];
        char next_dst[PATH_MAX];
        snprintf(next_src, PATH_MAX, "%s/%s", source_path, entry->d_name);
        snprintf(next_dst, PATH_MAX, "%s/%s", destination_path, entry->d_name);
        
        struct stat st;
        if (lstat(next_src, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                add_watches_recursive(notify_fd, next_src, next_dst);
            }
        }
    }
    closedir(dir);
}

struct Watch* get_watch_by_wd(int wd) {
    for (int i = 0; i < map.watch_count; i++) {
        if (map.watch_map[i].wd == wd) {
            return &map.watch_map[i];
        }
    }
    return NULL;
}

void directory_delete(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        
        char full_path[PATH_MAX];
        snprintf(full_path, PATH_MAX, "%s/%s", path, entry->d_name);
        
        struct stat st;
        if (lstat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) directory_delete(full_path);
            else unlink(full_path);
        }
    }
    closedir(dir);
    rmdir(path);
    printf("[CHILD %d] Usunięto katalog: %s\n", getpid(), path);
}

void file_delete(const char *path) {
    unlink(path);
    printf("[CHILD %d] Usunięto plik: %s\n", getpid(), path);
}

void remove_watch_from_map(int wd) {
    for (int i = 0; i < map.watch_count; i++) {
        if (map.watch_map[i].wd == wd) {
            map.watch_map[i] = map.watch_map[map.watch_count - 1];
            map.watch_count--;
            return;
        }
    }
}

void child_handler(int sig) {
    (void)sig;
    running = 0;
}

void file_watcher_reccursive(char *source_path, char *destination_path) {
    
    printf("[CHILD %d] Synchronizacja początkowa...\n", getpid());
    directory_copy(source_path, destination_path);
    printf("[CHILD %d] Start monitoringu.\n", getpid());
    
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = child_handler; // Używamy lokalnego handlera dla dziecka
    if (sigaction(SIGINT, &sa, NULL) < 0) ERR("sigaction");
    if (sigaction(SIGTERM, &sa, NULL) < 0) ERR("sigaction");

    int notify_fd = inotify_init();
    if (notify_fd < 0) ERR("inotify_init");
    
    add_watches_recursive(notify_fd, source_path, destination_path);
    
    char buffer[4096 * 4];
    while (running) {
        ssize_t len = read(notify_fd, buffer, sizeof(buffer));
        if(len < 0){
            if(errno == EINTR){ 
                continue;
            }
            // read może zwrócić błąd przy zamykaniu, więc nie zawsze ERR jest konieczne
            perror("read"); 
            break;
        }
        ssize_t i = 0;
        while(i < len){
            struct inotify_event *event = (struct inotify_event *)&buffer[i];
            if(event->len > 0){
                struct Watch *w = get_watch_by_wd(event->wd);
                if(w != NULL){
                    char full_src[PATH_MAX];
                    char full_dst[PATH_MAX];

                    snprintf(full_src, PATH_MAX, "%s/%s", w->src, event->name);
                    snprintf(full_dst, PATH_MAX, "%s/%s", w->dst, event->name);
                    
                    if (event->mask & IN_ISDIR) {
                        if (event->mask & IN_CREATE || event->mask & IN_MOVED_TO) {
                            directory_copy(full_src, full_dst);
                            add_watches_recursive(notify_fd, full_src, full_dst);
                        } else if(event->mask & IN_DELETE || event->mask & IN_MOVED_FROM){
                            directory_delete(full_dst);
                        }
                    } else {
                        if (event->mask & IN_CREATE || event->mask & IN_MODIFY || event->mask & IN_MOVED_TO) {
                            file_copy(full_src, full_dst);
                        } else if (event->mask & IN_DELETE || event->mask & IN_MOVED_FROM) {
                            file_delete(full_dst);
                        }
                    }
                }
            }
            if (event->mask & IN_IGNORED) {
                remove_watch_from_map(event->wd);
            }
            i += sizeof(struct inotify_event) + event->len;
        }
    }
    close(notify_fd);
    printf("[CHILD %d] Koniec pracy.\n", getpid());
}