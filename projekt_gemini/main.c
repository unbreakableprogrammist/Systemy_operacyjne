#include "backup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  char command[MAX_COMMAND];
  char *args[MAX_ARGS];
  int arg_count;

  printf("Interactive Backup System\n");
  printf("Type 'help' for a list of commands.\n");

  init_backup_system();

  while (1) {
    printf("> ");
    if (fgets(command, sizeof(command), stdin) == NULL) {
      break;
    }

    // Remove newline
    command[strcspn(command, "\n")] = 0;

    char *token = strtok(command, " ");
    if (token == NULL)
      continue;

    if (strcmp(token, "exit") == 0) {
      break;
    } else if (strcmp(token, "add") == 0) {
      // Parse arguments for add
      char *source = strtok(NULL, " ");
      char *target = strtok(NULL, " ");
      if (source && target) {
        add_backup(source, target);
      } else {
        printf("Usage: add <source> <target>\n");
      }
    } else if (strcmp(token, "list") == 0) {
      list_backups();
    } else if (strcmp(token, "end") == 0) { // 'end' terminates backup
      char *source = strtok(NULL, " ");
      char *target = strtok(NULL, " ");
      if (source && target) {
        remove_backup(source, target);
      } else {
        printf("Usage: end <source> <target>\n");
      }
    } else if (strcmp(token, "restore") == 0) {
      char *source = strtok(NULL, " ");
      char *target = strtok(NULL, " ");
      if (source && target) {
        restore_backup(source, target);
      } else {
        printf("Usage: restore <source> <target>\n");
      }
    } else if (strcmp(token, "help") == 0) {
      printf("Available commands: add, list, end, restore, exit\n");
    } else {
      printf("Unknown command: %s\n", token);
    }
  }

  shutdown_system();
  return 0;
}
