#include "header.h"

void recover(char *source_path, char *destination_path)
{
    struct stat sb;
    if (stat(source_path, &sb) == -1)
    {
        perror("stat");
        return;
    }
    if (!S_ISDIR(sb.st_mode))
    {
        perror("recover");
        return;
    }
    directory_delete(source_path);
    directory_copy(destination_path, source_path);
    exit(EXIT_SUCCESS);
}
