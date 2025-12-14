#include "header.h"

struct WatchMap watches[MAX_WATCHES];
int watch_count = 0;

void file_watcher_reccursive(char *source_path, char *destination_path){
    int fd = inotify_init();
    if (fd < 0) { ERR("inotify_init"); exit(1); }
}
