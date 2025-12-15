#include "header.h"

static volatile sig_atomic_t running = 1;
// globalna tablica mapa watch descriptorów , src i dst
struct WatchMap map = {0};

// funkcja sprawdzajaca czy katalog jest pusty , 1 - pusty , 0 - niepusty , -1 - blad
int is_dir_empty(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) return -1; // Pewnie nie istnieje, więc jest "czysty" lub blad opendir
    struct dirent *entry; 
    int has_files = 0; // sprawdzamy czy ma jakies pliki 
    while ((entry = readdir(dir)) != NULL) {
        // Jeśli znajdziemy cokolwiek innego niż "." lub ".." to jest niepusty
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            has_files = 1;
            break;
        }
    }
    closedir(dir);
    return (has_files == 0); // Zwraca 1 jeśli pusty, 0 jeśli znaleziono pliki
}
// przekopiowane z ktoregos rozwiazania z labow
ssize_t bulk_write(int fd, const char *buf, size_t count) {
  ssize_t c;
  ssize_t len = 0;
  while (count > 0) {
    c = TEMP_FAILURE_RETRY(write(fd, buf, count));
    if (c < 0)
      return -1;
    if (c == 0)
      break;

    buf += c;
    len += c;
    count -= c;
  }
  return len;
}
// funkcja dodajaca watch do mapy
void add_watch_to_map(int wd, const char *src, const char *dst) {
  if (map.watch_count < MAX_WATCHES) { // sprawdzamy czy nie przekroczylismy maksymalnej liczby watchow
    map.watch_map[map.watch_count].wd = wd; // dodajemy wd
    strncpy(map.watch_map[map.watch_count].src, src, PATH_MAX - 1); // kopiujemy sciezke src
    strncpy(map.watch_map[map.watch_count].dst, dst, PATH_MAX - 1); // kopiujemy sciezke dst
    map.watch_count++; // zwiększamy liczbe watchy 
  } else {
    printf("[CHILD %d] Przekroczono maksymalna liczbe watchow\n", getpid());
  }
}
// funkcja od kopiowania pliku
void file_copy(char *source, char *destination) {
  int src_fd = open(source, O_RDONLY); // otwieramy plik zrodlowy ale w trybie tylko do odczytu
  if (src_fd < 0) {
    perror("open source failed");  // nie wywolujemy ERR bo nieodczytanie jednego pliku nie powoduje zatrzymania
    return;
  }
  // otwieramy plik docelowy ale w trybie tylko do zapisu , O_TRUNC zeruje plik jesli istnieje
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
  if (n < 0)
    perror("read source failed");

  close(src_fd);
  close(dst_fd);
  //printf("[CHILD %d] Skopiowano plik %s do %s\n", getpid(), source,destination);
}

void handle_link(char *source, char *destination) {
  (void)source;
  (void)destination;
}
// -> tu skonczylem 
void directory_copy(char *source_path, char *destination_path) {
    // struktura na czytanie z katalogu docelowego
    struct stat st_dest;
    // Sprawdzamy stan katalogu docelowego
    if (stat(destination_path, &st_dest) == 0) {
        // Ścieżka istnieje. Sprawdzamy, czy to katalog.
        if (S_ISDIR(st_dest.st_mode)) {
            // Jeśli istnieje, MUSI być pusty 
            // Uwaga: Funkcja wywoływana rekurencyjnie trafia na puste katalogi utworzone
            // chwilę wcześniej przez pętlę while (mkdir), więc tam is_dir_empty zwróci true (OK).
            // Problem będzie tylko przy pierwszym wywołaniu dla niepustego folderu.
            if (!is_dir_empty(destination_path)) {
                fprintf(stderr, "[PID %d] BŁĄD: Katalog docelowy '%s' nie jest pusty!\n", getpid(), destination_path);
                exit(EXIT_FAILURE); // Kończymy proces backupu, bo nie wolno nadpisywać
            }
        } else {
            fprintf(stderr, "[PID %d] BŁĄD: '%s' istnieje, ale nie jest katalogiem.\n", getpid(), destination_path);
            exit(EXIT_FAILURE);
        }
    } else {
        // Nie istnieje -> Tworzymy nowy
        // 0755 to standardowe uprawnienia (rwxr-xr-x)
        if (mkdir(destination_path, 0755) < 0 && errno != EEXIST) {
            perror("mkdir destination failed");
            exit(EXIT_FAILURE);
        }
    }

    // 2. Otwieranie źródła
    DIR *dir = opendir(source_path);
    if (!dir) {
        // Jeśli nie możemy otworzyć źródła, to backup tego fragmentu jest niemożliwy
        perror("opendir source failed");
        return; 
    }

    // 3. Iteracja po plikach
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Pomijamy . i ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char next_src[PATH_MAX];
        char next_dst[PATH_MAX];
        
        // Bezpieczne tworzenie ścieżek
        // Sprawdzamy wynik, żeby nie uciąć ścieżki (buffer overflow protection)
        int n1 = snprintf(next_src, PATH_MAX, "%s/%s", source_path, entry->d_name);
        int n2 = snprintf(next_dst, PATH_MAX, "%s/%s", destination_path, entry->d_name);

        if (n1 >= PATH_MAX || n2 >= PATH_MAX) {
            fprintf(stderr, "Ścieżka zbyt długa, pomijam: %s\n", entry->d_name);
            continue;
        }

        struct stat st;
        // Używamy lstat, żeby nie wchodzić w symlinki (wymóg obsługi symlinków jako plików [cite: 11])
        if (lstat(next_src, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                // REKURENCJA:
                // Najpierw tworzymy pusty katalog w celu...
                if (mkdir(next_dst, st.st_mode & 0777) < 0 && errno != EEXIST) {
                    perror("mkdir recursive failed");
                    continue; 
                }
                // ...a potem wchodzimy do środka (katalog jest pusty, więc walidacja na początku przejdzie)
                directory_copy(next_src, next_dst);
                
            } else if (S_ISREG(st.st_mode)) {
                // Zwykły plik
                file_copy(next_src, next_dst);
                
            } else if (S_ISLNK(st.st_mode)) {
                // Symlink [cite: 11, 12, 13]
                handle_link(next_src, next_dst);
            }
        } else {
            perror("lstat failed inside loop");
        }
    }
    closedir(dir);
}

void add_watches_recursive(int notify_fd, char *source_path,
                           char *destination_path) {
  int wd = inotify_add_watch(notify_fd, source_path,
                             IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_TO |
                                 IN_MOVED_FROM | IN_DELETE_SELF);

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
    if ((strcmp((entry->d_name), ".")) == 0 ||
        (strcmp((entry->d_name), "..")) == 0) {
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

struct Watch *get_watch_by_wd(int wd) {
  for (int i = 0; i < map.watch_count; i++) {
    if (map.watch_map[i].wd == wd) {
      return &map.watch_map[i];
    }
  }
  return NULL;
}

void directory_delete(const char *path) {
  DIR *dir = opendir(path);
  if (!dir)
    return;

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    char full_path[PATH_MAX];
    snprintf(full_path, PATH_MAX, "%s/%s", path, entry->d_name);

    struct stat st;
    if (lstat(full_path, &st) == 0) {
      if (S_ISDIR(st.st_mode))
        directory_delete(full_path);
      else
        unlink(full_path);
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
  if (sigaction(SIGINT, &sa, NULL) < 0)
    ERR("sigaction");
  if (sigaction(SIGTERM, &sa, NULL) < 0)
    ERR("sigaction");

  int notify_fd = inotify_init();
  if (notify_fd < 0)
    ERR("inotify_init");

  add_watches_recursive(notify_fd, source_path, destination_path);

  char buffer[4096 * 4];
  while (running) {
    ssize_t len = read(notify_fd, buffer, sizeof(buffer));
    if (len < 0) {
      if (errno == EINTR) {
        continue;
      }
      // read może zwrócić błąd przy zamykaniu, więc nie zawsze ERR jest
      // konieczne
      perror("read");
      break;
    }
    ssize_t i = 0;
    while (i < len) {
      struct inotify_event *event = (struct inotify_event *)&buffer[i];
      if (event->len > 0) {
        struct Watch *w = get_watch_by_wd(event->wd);
        if (w != NULL) {
          char full_src[PATH_MAX];
          char full_dst[PATH_MAX];

          snprintf(full_src, PATH_MAX, "%s/%s", w->src, event->name);
          snprintf(full_dst, PATH_MAX, "%s/%s", w->dst, event->name);

          if (event->mask & IN_ISDIR) {
            if (event->mask & IN_CREATE || event->mask & IN_MOVED_TO) {
              directory_copy(full_src, full_dst);
              add_watches_recursive(notify_fd, full_src, full_dst);
            } else if (event->mask & IN_DELETE || event->mask & IN_MOVED_FROM) {
              directory_delete(full_dst);
            }
          } else {
            if (event->mask & IN_CREATE || event->mask & IN_MODIFY ||
                event->mask & IN_MOVED_TO) {
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