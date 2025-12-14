#include "header.h"

// Twoja tablica globalna (zgodnie z Twoim snippetem)
struct WatchEntry watches[MAX_WATCHES];
int watch_count = 0;

// --- FUNKCJE POMOCNICZE DO MAPY ---

// Dodaje wpis do tablicy watches
void add_watch_to_map(int wd, const char *path) {
    if (watch_count < MAX_WATCHES) {
        watches[watch_count].wd = wd;
        watches[watch_count].path = strdup(path); // Kopiujemy string!
        watch_count++;
    }
}

// Znajduje ścieżkę na podstawie numeru wd
char* get_path_by_wd(int wd) {
    for (int i = 0; i < watch_count; i++) {
        if (watches[i].wd == wd) {
            return watches[i].path;
        }
    }
    return NULL;
}

// Usuwa wpis z mapy (np. gdy usunięto folder)
void remove_watch_from_map(int wd) {
    for (int i = 0; i < watch_count; i++) {
        if (watches[i].wd == wd) {
            free(watches[i].path);
            // Nadpisujemy usuwany element ostatnim elementem tablicy (szybkie usuwanie)
            watches[i] = watches[watch_count - 1];
            watch_count--;
            return;
        }
    }
}

// --- REKURENCYJNE DODAWANIE WATCHY ---
// Ta funkcja chodzi po folderach i zakłada "podsłuch" na każdym z nich
void add_watches_recursive(int fd, const char *path) {
    // 1. Dodaj watch na bieżący katalog
    // Obserwujemy: Tworzenie, Usuwanie, Modyfikację, Przenoszenie
    int wd = inotify_add_watch(fd, path, IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_TO | IN_MOVED_FROM | IN_DELETE_SELF);
    
    if (wd < 0) {
        // Jeśli nie udało się dodać watcha (np. brak uprawnień), tylko logujemy błąd
        // perror("inotify_add_watch failed"); 
    } else {
        add_watch_to_map(wd, path);
    }

    // 2. Przejdź przez podkatalogi
    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Pomiń "." i ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) 
            continue;

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat st;
        // Sprawdź czy to katalog (nie podążamy za symlinkami, stąd lstat)
        if (lstat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
            add_watches_recursive(fd, full_path); // Rekurencja
        }
    }
    closedir(dir);
}

// --- GŁÓWNA FUNKCJA (którą zacząłeś pisać) ---

void file_watcher_reccursive(char *source_path, char *destination_path) {
    // 1. Synchronizacja początkowa (Initial Sync)
    printf("[CHILD %d] Rozpoczynam synchronizację początkową: %s -> %s\n", getpid(), source_path, destination_path);
    directory_copy(source_path, destination_path);
    printf("[CHILD %d] Synchronizacja zakończona. Uruchamiam inotify.\n", getpid());

    // 2. Inicjalizacja inotify
    int fd = inotify_init();
    if (fd < 0) {
        perror("inotify_init");
        exit(EXIT_FAILURE);
    }

    // 3. Dodanie watchy na start (dla źródła i wszystkich podfolderów)
    add_watches_recursive(fd, source_path);

    // 4. Pętla obsługi zdarzeń
    char buffer[4096];
    while (1) {
        ssize_t len = read(fd, buffer, sizeof(buffer));
        if (len < 0) {
            if (errno == EINTR) continue; // Przerwanie sygnałem, czytamy dalej
            perror("read");
            break;
        }

        ssize_t i = 0;
        while (i < len) {
            struct inotify_event *event = (struct inotify_event*)&buffer[i];
            
            // Jeśli zdarzenie ma nazwę (niektóre, jak IN_DELETE_SELF, mogą nie mieć)
            if (event->len > 0) {
                // A. Znajdź pełną ścieżkę ŹRÓDŁOWĄ
                char *watched_dir = get_path_by_wd(event->wd);
                
                if (watched_dir) {
                    char full_src_path[PATH_MAX];
                    char full_dst_path[PATH_MAX];

                    // Budowanie pełnej ścieżki źródłowej: watched_dir + nazwa pliku
                    snprintf(full_src_path, sizeof(full_src_path), "%s/%s", watched_dir, event->name);

                    // B. Obliczanie ścieżki DOCELOWEJ
                    // Matematyka ścieżek: Bierzemy ścieżkę pliku, odejmujemy ścieżkę bazową źródła
                    // i doklejamy to do ścieżki bazowej celu.
                    // Przykład: 
                    // src_root: /A, dst_root: /B
                    // event w: /A/sub/plik.txt
                    // relative: /sub/plik.txt
                    // wynik: /B/sub/plik.txt
                    
                    const char *relative_path = watched_dir + strlen(source_path); 
                    snprintf(full_dst_path, sizeof(full_dst_path), "%s%s/%s", destination_path, relative_path, event->name);

                    // C. Obsługa Zdarzeń
                    if (event->mask & IN_ISDIR) {
                        // --- DLA KATALOGÓW ---
                        if (event->mask & IN_CREATE || event->mask & IN_MOVED_TO) {
                            // Nowy katalog: skopiuj go i zacznij obserwować
                            directory_copy(full_src_path, full_dst_path);
                            add_watches_recursive(fd, full_src_path);
                        }
                        else if (event->mask & IN_DELETE || event->mask & IN_MOVED_FROM) {
                            // Usunięty katalog: usuń go z backupu
                            directory_delete(full_dst_path);
                        }
                    } else {
                        // --- DLA PLIKÓW ---
                        if (event->mask & IN_CREATE || event->mask & IN_MODIFY || event->mask & IN_MOVED_TO) {
                            // Nowy/zmieniony plik: kopiuj
                            file_copy(full_src_path, full_dst_path);
                        }
                        else if (event->mask & IN_DELETE || event->mask & IN_MOVED_FROM) {
                            // Usunięty plik: usuń
                            file_delete(full_dst_path);
                        }
                    }
                }
            }
            // Ważne: Jeśli folder, który obserwowaliśmy został usunięty, inotify wyśle IN_IGNORED.
            // Musimy posprzątać mapę.
            if (event->mask & IN_IGNORED) {
                remove_watch_from_map(event->wd);
            }

            i += sizeof(struct inotify_event) + event->len;
        }
    }
    close(fd);
}