#include "backup.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int is_directory_empty(const char *path) {
  DIR *d = opendir(path);
  if (!d)
    return -1;

  struct dirent *dir;
  int count = 0;
  while ((dir = readdir(d)) != NULL) {
    if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
      count++;
      break;
    }
  }
  closedir(d);
  return count == 0;
}

int copy_file(const char *src_path, const char *dst_path) {
  int src_fd = open(src_path, O_RDONLY);
  if (src_fd < 0)
    return -1;

  int dst_fd = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (dst_fd < 0) {
    close(src_fd);
    return -1;
  }

  char buffer[4096];
  ssize_t bytes_read;
  while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
    if (write(dst_fd, buffer, bytes_read) != bytes_read) {
      close(src_fd);
      close(dst_fd);
      return -1;
    }
  }

  // Preserve metadata if possible (simplified here, but important for backup)
  struct stat st;
  if (fstat(src_fd, &st) == 0) {
    fchmod(dst_fd, st.st_mode);
    // utimensat could be used for timestamps
  }

  close(src_fd);
  close(dst_fd);
  return (bytes_read < 0) ? -1 : 0;
}

int copy_recursive(const char *src, const char *dst) {
  DIR *d = opendir(src);
  if (!d)
    return -1;

  // Create destination directory if it doesn't exist
  struct stat st;
  if (stat(dst, &st) == -1) {
    if (mkdir(dst, 0755) == -1) {
      closedir(d);
      return -1;
    }
  }

  struct dirent *dir;
  while ((dir = readdir(d)) != NULL) {
    if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
      continue;

    char src_path[MAX_PATH];
    char dst_path[MAX_PATH];
    snprintf(src_path, sizeof(src_path), "%s/%s", src, dir->d_name);
    snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, dir->d_name);

    struct stat entry_stat;
    if (lstat(src_path, &entry_stat) == -1)
      continue;

    if (S_ISDIR(entry_stat.st_mode)) {
      if (copy_recursive(src_path, dst_path) == -1) {
        closedir(d);
        return -1;
      }
    } else if (S_ISREG(entry_stat.st_mode)) {
      if (copy_file(src_path, dst_path) == -1) {
        closedir(d);
        return -1;
      }
    } else if (S_ISLNK(entry_stat.st_mode)) {
      // Handle symlinks
      char link_target[MAX_PATH];
      ssize_t len = readlink(src_path, link_target, sizeof(link_target) - 1);
      if (len != -1) {
        link_target[len] = '\0';
        symlink(link_target, dst_path);
      }
    }
  }
  closedir(d);
  return 0;
}

int remove_recursive(const char *path) {
  DIR *d = opendir(path);
  if (!d)
    return -1;

  struct dirent *dir;
  while ((dir = readdir(d)) != NULL) {
    if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
      continue;

    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, dir->d_name);

    struct stat st;
    if (lstat(full_path, &st) == -1)
      continue;

    if (S_ISDIR(st.st_mode)) {
      remove_recursive(full_path);
    } else {
      unlink(full_path);
    }
  }
  closedir(d);
  return rmdir(path);
}

int synchronize(const char *src, const char *dst) {
  // 1. If dst doesn't exist, just copy
  struct stat st_dst;
  if (stat(dst, &st_dst) == -1) {
    return copy_recursive(src, dst);
  }

  DIR *d_dst = opendir(dst);
  if (!d_dst)
    return -1;

  // 2. Delete files in dst that are not in src
  struct dirent *dir;
  while ((dir = readdir(d_dst)) != NULL) {
    if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
      continue;

    char src_path[MAX_PATH];
    char dst_path[MAX_PATH];
    snprintf(src_path, sizeof(src_path), "%s/%s", src, dir->d_name);
    snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, dir->d_name);

    struct stat st_src;
    if (lstat(src_path, &st_src) == -1) {
      // Exists in dst but not src -> delete
      struct stat st_entry;
      lstat(dst_path, &st_entry);
      if (S_ISDIR(st_entry.st_mode)) {
        remove_recursive(dst_path);
      } else {
        unlink(dst_path);
      }
    }
  }
  closedir(d_dst);

  // 3. Copy files from src to dst (update or new)
  DIR *d_src = opendir(src);
  if (!d_src)
    return -1;

  while ((dir = readdir(d_src)) != NULL) {
    if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
      continue;

    char src_path[MAX_PATH];
    char dst_path[MAX_PATH];
    snprintf(src_path, sizeof(src_path), "%s/%s", src, dir->d_name);
    snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, dir->d_name);

    struct stat st_src_entry;
    lstat(src_path, &st_src_entry);

    struct stat st_dst_entry;
    if (lstat(dst_path, &st_dst_entry) == -1) {
      // New file/dir
      if (S_ISDIR(st_src_entry.st_mode)) {
        copy_recursive(src_path, dst_path);
      } else if (S_ISREG(st_src_entry.st_mode)) {
        copy_file(src_path, dst_path);
      } else if (S_ISLNK(st_src_entry.st_mode)) {
        char link_target[MAX_PATH];
        ssize_t len = readlink(src_path, link_target, sizeof(link_target) - 1);
        if (len != -1) {
          link_target[len] = '\0';
          symlink(link_target, dst_path);
        }
      }
    } else {
      // Exists, check if update needed
      if (S_ISDIR(st_src_entry.st_mode)) {
        // Recurse
        if (S_ISDIR(st_dst_entry.st_mode)) {
          synchronize(src_path, dst_path);
        } else {
          // Type mismatch: replace file with dir
          unlink(dst_path);
          mkdir(dst_path, 0755);
          synchronize(src_path, dst_path); // Recursively sync content
        }
      } else if (S_ISREG(st_src_entry.st_mode)) {
        if (st_src_entry.st_mtime > st_dst_entry.st_mtime ||
            st_src_entry.st_size != st_dst_entry.st_size) {
          copy_file(src_path, dst_path);
        }
      }
      // Symlinks: check target and update if diff (omitted for brevity, usually
      // remove and re-link)
    }
  }
  closedir(d_src);
  return 0;
}
