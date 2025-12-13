#include "backup.h"
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_JOBS 10

typedef struct {
  char source[MAX_PATH];
  char target[MAX_PATH];
  pthread_t thread_id;
  int active;
} Job;

static Job jobs[MAX_JOBS];
static int job_count = 0;
static pthread_mutex_t job_mutex = PTHREAD_MUTEX_INITIALIZER;

void *backup_thread_routine(void *arg) {
  int job_idx = *(int *)arg;
  free(arg);

  char *source = jobs[job_idx].source;
  char *target = jobs[job_idx].target;

  printf("[Job %d] Starting initial copy from %s to %s...\n", job_idx, source,
         target);

  if (copy_recursive(source, target) != 0) {
    fprintf(stderr, "[Job %d] Copy failed.\n", job_idx);
    // Handle error: maybe mark job as invalid
    return NULL;
  }

  printf("[Job %d] Initial copy complete. Starting monitoring...\n", job_idx);

  monitor_directory(&jobs[job_idx]);

  return NULL;
}

void init_backup_system(void) {
  memset(jobs, 0, sizeof(jobs));
  printf("Backup system initialized.\n");
}

int add_backup(const char *source, const char *target) {
  struct stat st;

  // 1. Validate Source
  if (stat(source, &st) == -1 || !S_ISDIR(st.st_mode)) {
    printf(
        "Error: Source directory '%s' does not exist or is not a directory.\n",
        source);
    return -1;
  }

  // 2. Validate Target
  if (stat(target, &st) == 0) {
    if (!S_ISDIR(st.st_mode)) {
      printf("Error: Target '%s' exists but is not a directory.\n", target);
      return -1;
    }
    if (is_directory_empty(target) ==
        0) { // 0 means not empty based on my logic? Wait, let me check utils.c
      // utils.c: return count == 0; -> returns 1 if empty, 0 if not.
      // So if returns 0, it is NOT empty.
      printf("Error: Target directory '%s' is not empty.\n", target);
      return -1;
    }
  } else {
    // Target doesn't exist, will be created by copy_recursive
  }

  // 3. Add to job list
  pthread_mutex_lock(&job_mutex);
  if (job_count >= MAX_JOBS) {
    printf("Error: Max backup jobs reached.\n");
    pthread_mutex_unlock(&job_mutex);
    return -1;
  }

  int idx = job_count++;
  strncpy(jobs[idx].source, source, MAX_PATH);
  strncpy(jobs[idx].target, target, MAX_PATH);
  jobs[idx].active = 1;

  // 4. Spawn thread
  int *arg = malloc(sizeof(int));
  *arg = idx;
  if (pthread_create(&jobs[idx].thread_id, NULL, backup_thread_routine, arg) !=
      0) {
    printf("Error: Failed to create thread.\n");
    jobs[idx].active = 0;
    job_count--;
    free(arg);
    pthread_mutex_unlock(&job_mutex);
    return -1;
  }

  pthread_mutex_unlock(&job_mutex);
  printf("Backup added: %s -> %s\n", source, target);
  return 0;
}

int remove_backup(const char *source, const char *target) {
  pthread_mutex_lock(&job_mutex);
  for (int i = 0; i < job_count; i++) {
    if (jobs[i].active && strcmp(jobs[i].source, source) == 0 &&
        strcmp(jobs[i].target, target) == 0) {

      jobs[i].active = 0;
      // In a real system, we might need to wake up the thread if it's blocking
      // on read Since we use inotify read(), it blocks. We might need to send a
      // signal or use select/poll with a pipe. For this assignment, checking
      // active flag loop might be delayed until an event happens. But since
      // "commands should be responsive", maybe cancel thread?
      pthread_cancel(jobs[i].thread_id);
      pthread_join(jobs[i].thread_id, NULL); // Wait for cleanup

      // Allow reusing slot? Or just leave inactive.
      // Ideally shift array, but simple mark inactive is fine for small
      // MAX_JOBS.
      printf("Backup monitoring ended for: %s -> %s\n", source, target);

      pthread_mutex_unlock(&job_mutex);
      return 0;
    }
  }
  pthread_mutex_unlock(&job_mutex);
  printf("Error: Backup job not found for %s -> %s\n", source, target);
  return -1;
}

void list_backups(void) {
  pthread_mutex_lock(&job_mutex);
  printf("ID\tSource\t\tTarget\n");
  for (int i = 0; i < job_count; i++) {
    if (jobs[i].active) {
      printf("%d\t%s\t%s\n", i, jobs[i].source, jobs[i].target);
    }
  }
  pthread_mutex_unlock(&job_mutex);
}

void restore_backup(const char *source, const char *target) {
  printf("Restoring backup from %s to %s...\n", target, source);
  // Note: User provides source and target of backup. Restore copies target ->
  // source.
  if (synchronize(target, source) == 0) {
    printf("Restore complete.\n");
  } else {
    printf("Restore failed.\n");
  }
}

void shutdown_system(void) {
  printf("System shutting down. Waiting for threads...\n");
  // Should join threads here
}

#ifdef __linux__
#include <sys/inotify.h>
#endif
#include <dirent.h> // For opendir, readdir
#include <limits.h>

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

// Structure to map watch descriptors to paths
typedef struct WatchNode {
  int wd;
  char path[MAX_PATH];
  struct WatchNode *next;
} WatchNode;

void add_watch_recursive(int fd, const char *path, WatchNode **head) {
// Add watch for current directory
#ifdef __linux__
  int wd = inotify_add_watch(fd, path,
                             IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM |
                                 IN_MOVED_TO | IN_CLOSE_WRITE);
  if (wd == -1) {
    perror("inotify_add_watch");
  } else {
    // Add to list
    WatchNode *node = malloc(sizeof(WatchNode));
    node->wd = wd;
    strncpy(node->path, path, MAX_PATH);
    node->next = *head;
    *head = node;
    // printf("Watching %s (wd: %d)\n", path, wd);
  }
#endif

  // Recurse into subdirectories
  DIR *d = opendir(path);
  if (!d)
    return;

  struct dirent *dir;
  while ((dir = readdir(d)) != NULL) {
    if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
      continue;

    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, dir->d_name);

    struct stat st;
    if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
      add_watch_recursive(fd, full_path, head);
    }
  }
  closedir(d);
}

char *get_path_by_wd(WatchNode *head, int wd) {
  WatchNode *current = head;
  while (current) {
    if (current->wd == wd)
      return current->path;
    current = current->next;
  }
  return NULL;
}

void *monitor_directory(void *arg) {
  Job *job = (Job *)arg;

#ifdef __linux__
  int fd = inotify_init();
  if (fd < 0) {
    perror("inotify_init");
    return NULL;
  }

  WatchNode *watches = NULL;
  add_watch_recursive(fd, job->source, &watches);

  char buffer[EVENT_BUF_LEN];
  while (job->active) {
    int length = read(fd, buffer, EVENT_BUF_LEN);
    if (length < 0) {
      perror("read");
      break;
    }

    int i = 0;
    while (i < length) {
      struct inotify_event *event = (struct inotify_event *)&buffer[i];

      if (event->len) {
        char *src_dir = get_path_by_wd(watches, event->wd);
        if (src_dir) {
          char full_src_path[MAX_PATH];
          char rel_path[MAX_PATH];
          char full_dst_path[MAX_PATH];

          snprintf(full_src_path, sizeof(full_src_path), "%s/%s", src_dir,
                   event->name);

          // Calculate relative path to construct destination path
          // This is tricky if we just have src_dir.
          // We need to know where src_dir is relative to job->source.
          // Or simpler: full_dst_path = job->target + (full_src_path -
          // job->source)

          if (strncmp(full_src_path, job->source, strlen(job->source)) == 0) {
            const char *rel = full_src_path + strlen(job->source);
            snprintf(full_dst_path, sizeof(full_dst_path), "%s%s", job->target,
                     rel);
          } else {
            // Should not happen if paths are consistent
            i += EVENT_SIZE + event->len;
            continue;
          }

          if (event->mask & IN_CREATE) {
            if (event->mask & IN_ISDIR) {
              mkdir(full_dst_path, 0755);
              add_watch_recursive(fd, full_src_path, &watches);
            } else {
              // File created, wait for CLOSE_WRITE or just copy now?
              // Usually better to wait for CLOSE_WRITE for files to ensure
              // content is there. But touch creates empty file.
              copy_file(full_src_path, full_dst_path);
            }
          } else if (event->mask & IN_DELETE) {
            // remove file or dir
            if (event->mask & IN_ISDIR) {
              rmdir(full_dst_path); // ensure empty
                                    // Remove watch?
            } else {
              unlink(full_dst_path);
            }
          } else if (event->mask & IN_CLOSE_WRITE) {
            if (!(event->mask & IN_ISDIR)) {
              copy_file(full_src_path, full_dst_path);
            }
          } else if (event->mask & IN_MOVED_FROM) {
            // Rename start
            // This needs cookie handling for perfect rename support
            // For simplicity, treat as delete
            if (event->mask & IN_ISDIR) {
              rmdir(full_dst_path); // brute force, might fail if not empty
            } else {
              unlink(full_dst_path);
            }
          } else if (event->mask & IN_MOVED_TO) {
            // Rename end -> treat as create
            if (event->mask & IN_ISDIR) {
              mkdir(full_dst_path, 0755);
              copy_recursive(full_src_path, full_dst_path);
              add_watch_recursive(fd, full_src_path, &watches);
            } else {
              copy_file(full_src_path, full_dst_path);
            }
          }
        }
      }
      i += EVENT_SIZE + event->len;
    }
  }
  close(fd);
// Free watches list...
#else
  printf(
      "Inotify not supported on this OS. Monitoring simulation for %s -> %s\n",
      job->source, job->target);
  while (job->active) {
    sleep(10);
  }
#endif

  return NULL;
}
