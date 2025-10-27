#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

// perror("source") wypisuje sourde i komunikat bledu , fprint wypisuje FILE i LINIJKE

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
/*
struct stat {
    dev_t     st_dev;
    ino_t     st_ino;
    mode_t    st_mode;
    nlink_t   st_nlink;
    uid_t     st_uid;
    gid_t     st_gid;
    off_t     st_size;
    time_t    st_mtime;
    ...
};

zwykły plik	S_ISREG	-
katalog	S_ISDIR	d
link symboliczny	S_ISLNK	l
urządzenie blokowe	S_ISBLK	b
urządzenie znakowe	S_ISCHR	c
potok FIFO	S_IFIFO	p
gniazdo	S_IFSOCK	s
*/

int main(int argc,char** argv){
    DIR* current = opendir("."); // zwraca NULL przy nie powodzeniu 
    if(current == NULL) ERR("opendir"); // jesli blad
    struct dirent *dir_read;  // tworzymy struktrutke na czytanie z directory 
    int pliki= 0;
    int linki = 0;
    int katalogi =0;
    int other = 0;
    struct stat filestat; // tworzymy struktureke an info o pliku #include <sys/stat.h>
    while((dir_read = readdir(current))!=NULL){  // czytamy pliki z obecnego katalogu , w dir_read jest nazwa i inode
        errno = 0; // zerujemy zeby nie bylo bledu poprzedniego 
        if(lstat(dir_read->d_name,&filestat)==-1){  //czyatamy informacje z obecnego pliku do strukturki filestat
            ERR("lstat");
        }
        mode_t m = filestat.st_mode; // bierzemy atrybut st_mode
        if(S_ISLNK(m))
            linki++;
        else if(S_ISDIR(m))
            katalogi++;
        else if(S_ISREG(m))
            pliki++;
        else 
            other++;
    }
    if (errno != 0)
        ERR("readdir");
    if (closedir(current)) // zamyktamy i  sprawdzamy czy dobrze zamknal
        ERR("closedir");
    printf("Files: %d, Dirs: %d, Links: %d, Other: %d\n", pliki, katalogi, linki, other);
    //closedir(current); //zamyka otwarty folder, zwalnia deskrytpor i dodatkowo 0 - OK , -1 - blad
}