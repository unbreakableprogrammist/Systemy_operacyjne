#include "backup.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void remove_newline(char *line) { line[strcspn(line, "\n")] = 0; }

int tokenize(char *line, char *args[]) {
  int arg_count = 0;
  int in_quote = 0;
  char *src = line;
  char *dst = line;

  while (*src && arg_count < MAX_ARGS) {
    while (*src == ' ' && !in_quote)
      src++;
    if (*src == '\0')
      break;

    args[arg_count++] = dst;
    while (*src) {
      if (*src == '"') {
        in_quote = !in_quote;
        src++;
      } else if (*src == ' ' && !in_quote) {
        src++;
        break;
      } else {
        *dst++ = *src++;
      }
    }
    *dst++ = '\0';
  }
  return arg_count;
}

int is_subdir(const char *parent, const char *child) {
  char p_abs[MAX_PATH];
  char c_abs[MAX_PATH];

  if (realpath(parent, p_abs) == NULL || realpath(child, c_abs) == NULL) {
    // One of them might not exist yet (target), which is fine for "is subdir"
    // usually, but here we want to block if target IS going to be inside
    // source.
    return 0; // Assume safe if realpath fails (e.g. target doesn't exist yet)
  }

  // Check if c_abs starts with p_abs
  if (strstr(c_abs, p_abs) == c_abs) {
    if (c_abs[strlen(p_abs)] == '/' || c_abs[strlen(p_abs)] == '\0') {
      return 1;
    }
  }
  return 0;
}

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

    remove_newline(command);
    if (strlen(command) == 0)
      continue;

    arg_count = tokenize(command, args);
    if (arg_count == 0)
      continue;

    char *token = args[0];

    if (strcmp(token, "exit") == 0) {
      break;
    } else if (strcmp(token, "add") == 0) {
      if (arg_count >= 3) {
        char *source = args[1];
        char *target = args[2];

        // Safety Check: block if target is inside source

        // Let's use realpath for source. For target, we might not have it yet.
        // But let's check basic string containment or wait, add_backup allows
        // creating target. We can just check realpath of source (must exist)
        // and see if target path string starts with it? realpath is safer.

        // Let's use realpath for source. For target, we might not have it yet.
        // But if target is relative "kopia", and source is ".", then target
        // becomes "./kopia", which is inside source.

        // Check string overlap for '.' usage
        if (strcmp(source, ".") == 0) {
          // Source is current dir. Any relative target path is unsafe unless it
          // starts with ../
          if (target[0] != '/') {
            if (strncmp(target, "../", 3) != 0 && strcmp(target, "..") != 0) {
              printf(
                  "Error: Target '%s' is inside source '.' (recursion risk).\n",
                  target);
              continue;
            }
          }
        }

        add_backup(source, target);
      } else {
        printf("Usage: add <source> <target>\n");
      }
    } else if (strcmp(token, "list") == 0) {
      list_backups();
    } else if (strcmp(token, "end") == 0) {
      if (arg_count >= 3) {
        remove_backup(args[1], args[2]);
      } else {
        printf("Usage: end <source> <target>\n");
      }
    } else if (strcmp(token, "restore") == 0) {
      if (arg_count >= 3) {
        restore_backup(args[1], args[2]);
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
