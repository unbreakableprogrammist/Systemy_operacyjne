#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define MAX_DIR_INPUT 10
#define MAX_EXT 10
#define MAX_SUBDIRS 50
#define MAX_DIR_LEN 200

void list_dir(char* path,char* ext,int depth)
{
    if (depth ==0)
        return;
    DIR* dp = opendir(path);
    if (dp==NULL)
        ERR("opendir");
    printf("path: %s\n", path);
    errno = 0;
    struct dirent *de = readdir(dp);
    struct stat filestat;
    char subdirs[MAX_SUBDIRS][MAX_DIR_LEN];

    int i=0;
    while (de != NULL)
    {
        char* temp = (char*)calloc(strlen(path) + 2 + strlen(de->d_name),sizeof(char));
        if (temp == NULL)
            ERR("malloc");
        strcpy(temp,path);
        strcat(temp,"/");
        strcat(temp,de->d_name);
        if (lstat(temp,&filestat)==-1)
            ERR("lstat");
        errno=0;
        free(temp);
        if (errno!=0)
            ERR("free");
        char* fext = strchr(de->d_name,'.');
        if (S_ISDIR(filestat.st_mode))
        {
            if (strcmp(de->d_name,"..")==0 || strcmp(de->d_name,".")==0)
            {
                de = readdir(dp);
                continue;
            }
            strcpy(subdirs[i],path);
            strcat(subdirs[i],"/");
            strcat(subdirs[i++],de->d_name);
            if (i == MAX_SUBDIRS-1)
                break;
        }
        if (ext==NULL || (fext != NULL && strcmp(fext+1,ext)==0))
        {
            printf("%s %ld\n",de->d_name, filestat.st_size);
        }

        de = readdir(dp);
    }
    if (errno !=0)
        ERR("readdir");
    if (closedir(dp))
        ERR("closedir");
}

int main(int argc, char** argv)
{
    int c;
    char* dirs[MAX_DIR_INPUT];
    char* ext = NULL;
    int d=1;
    int i=0;
    while ((c=getopt(argc,argv,"p:e:d:")) != -1)
    {
        switch (c)
        {
        case 'p':
            dirs[i++] = optarg;
            break;
        case 'e':
            ext = optarg;
            break;
        case 'd':
            d = atoi(optarg);
            break;
        default:
            ERR("getopt");
        }
    }
    for (int j=0;j<i;j++)
    {
        list_dir(dirs[j],ext,d);
    }
    return EXIT_SUCCESS;
}
