#include "header.h"

struct WatchMap map = {0};
ssize_t bulk_write(int fd, const char *buf, size_t count) {
    ssize_t c;
    ssize_t len = 0;

    while (count > 0) {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return -1;       
        if (c == 0)
            break;             

        buf  += c;
        len  += c;
        count -= c;
    }

    return len;
}
void add_watch_to_map(int wd, const char *src, const char *dst) {
    if (map.watch_count < MAX_WATCHES) {
        map.watch_map[map.watch_count].wd = wd;
        strncpy(map.watch_map[map.watch_count].src, src, PATH_MAX);
        strncpy(map.watch_map[map.watch_count].dst, dst, PATH_MAX);
        map.watch_count++;
    }else{
        printf("[CHILD %d] Przekroczono maksymalna liczbe watchow\n", getpid());
    }
}

void file_copy(char *source, char *destination){
   int src_fd = open(source, O_RDONLY); // otwieramy plik z ktorego bedziemy czytac
   if(src_fd<0) ERR("open");
    //Otwieramy plik docelowy
    // O_WRONLY - do zapisu
    // O_CREAT  - utwórz jeśli nie istnieje
    // O_TRUNC  - wyczyść zawartość jeśli istnieje (nadpisz)
    // 0644     - uprawnienia rw-r--r-- (ważne przy O_CREAT!)
    int dst_fd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(dst_fd<0) ERR("open");
    char buffer[4096];
    ssize_t n;
    while((n=TEMP_FAILURE_RETRY(read(src_fd, buffer, sizeof(buffer))))>0){
        if(bulk_write(dst_fd, buffer, n)<0) ERR("bulk_write");
    }
    if(n<0) ERR("read");
    close(src_fd);
    close(dst_fd);
    printf("[CHILD %d] Skopiowano plik %s do %s\n", getpid(), source, destination);
}
void handle_link(char* source,char* destination){

}
void directory_copy(char *source_path, char *destination_path){
    // jesli directory nie istnieje to stworz go
    struct stat st;
    if (stat(destination_path, &st) == -1) {
        // Jeśli nie istnieje, stwórz go
        if (mkdir(destination_path, 0755) < 0) {
            perror("mkdir destination failed");
        }
    }
    DIR *dir = opendir(source_path);
    if (!dir) ERR("opendir");
    struct dirent *entry; // struktura do czytania directory
    while((entry = readdir(dir)) != NULL){
        // nie wchodzimy do samego siebie ani do poprzednika 
        if((strcmp((entry->d_name),".")) == 0 || (strcmp((entry->d_name),"..")) == 0){
            continue;
        }
        char next_src [MAX_PATH];
        char next_dst [MAX_PATH];
        snprintf(next_src, MAX_PATH, "%s/%s", source_path, entry->d_name); // wpisujemy do pliku nasza obecna sciezke
        snprintf(next_dst, MAX_PATH, "%s/%s", destination_path, entry->d_name); // wpisujemy do pliku nasza docelowa sciezke 
        
        struct stat st; // struktura do przechowywania informacji o pliku
        if(lstat(next_src, &st) == 0){
            if (S_ISDIR(st.st_mode)) {
                // utwórz katalog docelowy, jeśli nie istnieje
                if (mkdir(next_dst, st.st_mode & 0777) < 0 && errno != EEXIST) ERR("mkdir");
                directory_copy(next_src, next_dst);
            }else if(S_ISREG(st.st_mode)){
                file_copy(next_src, next_dst);
            }else if(S_ISLNK(st.st_mode)){
                handle_link(next_src, next_dst);
            }
        }else{
            ERR("lstat");
        }
    }
    closedir(dir);
    
}

void add_watches_recursive(int notify_fd, char *source_path, char *destination_path){
    int wd = inotify_add_watch(notify_fd, source_path, IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_TO | IN_MOVED_FROM | IN_DELETE_SELF);
    if(wd<0) ERR("inotify_add_watch");
    add_watch_to_map(wd, source_path, destination_path);
    DIR *dir = opendir(source_path);
    if(!dir) ERR("opendir");
    struct dirent *entry;
    while((entry = readdir(dir)) != NULL){
        if((strcmp((entry->d_name),".")) == 0 || (strcmp((entry->d_name),"..")) == 0){
            continue;
        }
        char next_src [MAX_PATH];
        char next_dst [MAX_PATH];
        snprintf(next_src, MAX_PATH, "%s/%s", source_path, entry->d_name); // wpisujemy do pliku nasza obecna sciezke
        snprintf(next_dst, MAX_PATH, "%s/%s", destination_path, entry->d_name); // wpisujemy do pliku nasza docelowa sciezke 
        
        struct stat st; // struktura do przechowywania informacji o pliku
        if(lstat(next_src, &st) == 0){
            if (S_ISDIR(st.st_mode)) {
                add_watches_recursive(notify_fd, next_src, next_dst);
            }
        }else{
            ERR("lstat");
        }
    }
    closedir(dir);
}


void file_watcher_reccursive(char *source_path, char *destination_path){
    printf("[CHILD %d] Synchronizacja początkowa...\n", getpid());
    directory_copy(source_path, destination_path);
    printf("[CHILD %d] Start monitoringu.\n", getpid());
    
    int notify_fd = inotify_init();
    if(notify_fd<0) ERR("inotify_init");
    add_watches_recursive(notify_fd, source_path, destination_path);
}