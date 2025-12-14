#include "header.h"

// teraz jestesmy w procesie dziecko dla folderu source_path , destination_path
// musimy zalozyc inotify i zaczac obserwowac folder i podfoldery
// potem musimy przekopiowac wszystkie pliki z source_path do destination_path
// a potem zaczac obserwowac zdarzenia
struct WatchMap watches[MAX_WATCHES];
int watch_count = 0;

void file_watcher_reccursive(char *source_path, char *destination_path){
    int fd = inotify_init();
    if (fd < 0) { ERR("inotify_init"); exit(1); }
    
}
