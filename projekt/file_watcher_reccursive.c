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
  // otwieramy plik docelowy ale w trybie tylko do zapisu , O_TRUNC zeruje plik jesli istnieje , 644 - standardowe uprawnienia
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

void handle_link(char *source, char *destination,char *root_src,char *root_dst) {
    char link_target[PATH_MAX];  // bufor na wskazanie linka 
    char new_target[PATH_MAX];  // bufor na nowy wskaznik linka

    // readlink czyta na co wskazuje symlink i kopiuje sicezke do linktarget
    ssize_t len = readlink(source, link_target, PATH_MAX - 1);
    if(len<0){
        perror("readlink failed");
        return;
    }
    link_target[len] = '\0'; // ostani znak to koniec 
    snprintf(new_target, sizeof(new_target), "%s", link_target); // na razie po prostu przepisujemy
    if(new_target[0]=='/' && strncmp(link_target, root_src, strlen(root_src)) == 0) { // jesli zaczynamy sie od / i nasza sciezka na ktora wskazujemy jest podobna do folderu skad przyszlismy to musimy zmienic
        char *suffix = link_target + strlen(root_src); // obliczamy suffix sciezki ( to co sie powtarza po src)   
        int n = snprintf(new_target, PATH_MAX, "%s%s", root_dst, suffix);
        if (n >= PATH_MAX) {
            fprintf(stderr, "Błąd: Nowa ścieżka linku jest zbyt długa\n");
            return;
        }     
    }
    // usuwamy stary link jesli istnieje
    if (unlink(destination) < 0 && errno != ENOENT) {
        perror("unlink destination failed");
    }

    // tworzymy nowy link
    if (symlink(new_target, destination) < 0) {
        perror("symlink creation failed");
    }
    
    
}

void directory_copy(char *source_path, char *destination_path) {
    // struktura na czytanie z katalogu docelowego
    struct stat st_dest;
    // Sprawdzamy stan katalogu docelowego
    if (stat(destination_path, &st_dest) == 0) { //  pobieramy informacje o destination_path i zapisujemy do st_dest , 0 = succes
        // Ścieżka istnieje wiec sprawdzamy czy jest katalogiem 
        if (S_ISDIR(st_dest.st_mode)) {
            // jesli nie jest pusty to zwracamy blad
            if (!is_dir_empty(destination_path)) {
                fprintf(stderr, "[PID %d] BŁĄD: Katalog docelowy '%s' nie jest pusty!\n", getpid(), destination_path);
                exit(EXIT_FAILURE); // Kończymy proces backupu, bo nie wolno nadpisywać
            }
            // jesli jest pusty to luz
        } else {
            // w takim wypadku sciezka podana to nie byl katalog
            fprintf(stderr, "[PID %d] BŁĄD: '%s' istnieje, ale nie jest katalogiem.\n", getpid(), destination_path);
            exit(EXIT_FAILURE);
        }
    } else {
        // Nie istnieje -> Tworzymy nowy
        // 0755 to standardowe uprawnienia dla katalogu bo wykonanie = wejscie do katalogu
        if (mkdir(destination_path, 0755) < 0 && errno != EEXIST) { // EEXIST = katalog juz istnieje
            perror("mkdir destination failed");
            exit(EXIT_FAILURE);
        }
    }

    // Otwieranie źródła
    DIR *dir = opendir(source_path);
    if (!dir) {
        // Jeśli nie możemy otworzyć źródła, to backup tego fragmentu jest niemożliwy
        perror("opendir source failed");
        return; 
    }

    // Iteracja po plikach
    
    // struktura do czytania plikow
    struct dirent *entry;
    // dopoki nie dojdzie do konca katalogu ( NULL )
    while ((entry = readdir(dir)) != NULL) {
        // Pomijamy . i ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        // bufory na nastepne sciezki
        char next_src[PATH_MAX];
        char next_dst[PATH_MAX];
        
        // Bezpieczne tworzenie ścieżek
        // Sprawdzamy wynik, żeby nie uciąć ścieżki (buffer overflow protection)
        int n1 = snprintf(next_src, PATH_MAX, "%s/%s", source_path, entry->d_name); // zklejamy poprzednia z nazwa obecnego pliku/katalogu
        int n2 = snprintf(next_dst, PATH_MAX, "%s/%s", destination_path, entry->d_name); // tak samo sklejamy z docelowym

        if (n1 >= PATH_MAX || n2 >= PATH_MAX) {
            fprintf(stderr, "Ścieżka zbyt długa, pomijam: %s\n", entry->d_name);
            continue;
        }

        struct stat st;
        if (lstat(next_src, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                // Najpierw tworzymy pusty katalog w dst
                if (mkdir(next_dst, st.st_mode & 0777) < 0 && errno != EEXIST) {
                    perror("mkdir recursive failed");
                    continue; 
                }
                // a potem wchodzimy do środka (katalog jest pusty, więc walidacja na początku przejdzie)
                directory_copy(next_src, next_dst);
                
            } else if (S_ISREG(st.st_mode)) {
                // Zwykły plik
                file_copy(next_src, next_dst);
                
            } else if (S_ISLNK(st.st_mode)) {
                // Symlinki
                handle_link(next_src, next_dst,source_path,destination_path);
            }
        } else {
            perror("lstat failed inside loop");
        }
    }
    closedir(dir);
}

void add_watches_recursive(int notify_fd, char *source_path,char *destination_path) {
    int wd = inotify_add_watch(notify_fd, source_path,IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_TO | IN_MOVED_FROM | IN_DELETE_SELF);

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
        file_delete(full_path);
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
      perror("read");
      break;
    }
    ssize_t i = 0;
    while (i < len) {
      struct inotify_event *event = (struct inotify_event *)&buffer[i]; // bierzemy jedno wydarzenie z bufora ( moze byc tam wiele )
      if (event->len > 0) {
        struct Watch *w = get_watch_by_wd(event->wd);
        if (w != NULL) {
          char full_src[PATH_MAX];
          char full_dst[PATH_MAX];

          snprintf(full_src, PATH_MAX, "%s/%s", w->src, event->name);
          snprintf(full_dst, PATH_MAX, "%s/%s", w->dst, event->name);

          if (event->mask & IN_ISDIR) { // czy event dotyczy katalogu
            if (event->mask & IN_CREATE || event->mask & IN_MOVED_TO) { // jesli dotyczy tworzenia katalogu lub przeniesienia -> koopiuj z source do destination
              directory_copy(full_src, full_dst);
              add_watches_recursive(notify_fd, full_src, full_dst);
            } else if (event->mask & IN_DELETE || event->mask & IN_MOVED_FROM) { // jesli dotyczy usuwania katalogu lub przeniesienia -> usuń z destination
              directory_delete(full_dst);
            }
          } else {
            if (event->mask & IN_CREATE || event->mask & IN_MODIFY ||
                event->mask & IN_MOVED_TO) { // jesli dotyczy tworzenia pliku lub modyfikacji lub przeniesienia -> kopiuj z source do destination
              file_copy(full_src, full_dst);
            } else if (event->mask & IN_DELETE || event->mask & IN_MOVED_FROM) { // jesli dotyczy usuwania pliku lub przeniesienia -> usuń z destination
              file_delete(full_dst);
            }
          }
        }
      }
      // jesli dostalismy informacje o usunieciu monitora to oznacza ze katalog zostal usuniety
      if (event->mask & IN_IGNORED) {
        remove_watch_from_map(event->wd);
      }
      i += sizeof(struct inotify_event) + event->len;
    }
  }
  close(notify_fd);
  printf("[CHILD %d] Koniec pracy.\n", getpid());
}

