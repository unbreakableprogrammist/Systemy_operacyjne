#ifndef BACKUP_H
#define BACKUP_H

#include <stddef.h>
#include <time.h>

#define MAX_PATH 4096
#define MAX_COMMAND 512
#define MAX_ARGS 3

// Structure to represent a backup job
typedef struct {
  char source_path[MAX_PATH];
  char target_path[MAX_PATH];
  // Add other fields as needed, e.g., thread ID or status
} BackupJob;

// Global list of active backup jobs (you might want a linked list or dynamic
// array) For simplicity, using a fixed size array or declare it in main.c and
// pass around void add_backup(const char *source, const char *target); void
// remove_backup(const char *source, const char *target); # or by ID

// Function prototypes
void init_backup_system(void);
int add_backup(const char *source, const char *target);
int remove_backup(const char *source,
                  const char *target); // Return 0 on success
void list_backups(void);
void restore_backup(const char *source, const char *target);
void shutdown_system(void);

// Utility prototypes
int is_directory_empty(const char *path);
int copy_recursive(const char *src, const char *dst);
int copy_file(const char *src, const char *dst);
int remove_recursive(const char *path);
int synchronize(const char *src, const char *dst);

// Monitor thread function or helper
void *monitor_directory(void *arg);

#endif // BACKUP_H
