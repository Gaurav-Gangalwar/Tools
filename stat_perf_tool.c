#define _GNU_SOURCE
#include<fcntl.h>
#include<dirent.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<unistd.h>
#include<stddef.h>
#include<pthread.h>
#include<time.h>
#include<stdio.h>

int do_write;
long files_stat;
long files_setattr;
pthread_mutex_t lock;
void _print_entries(char *name, char *parent) {
    DIR *dp = NULL;
    char *path;
    int alloc = 0;
    int entry_len = 0, path_len = 0;
    struct stat buf; 
    //printf("name %s par %s\n", name, parent);
    if (parent) {
        path_len = strlen(parent) + pathconf(parent, _PC_NAME_MAX) + 1;
        path = malloc(path_len);
        memset(path, 0, path_len);
        sprintf(path, "%s/%s", parent, name);
        alloc = 1;
    } else {
        path = name;
    }

    dp = opendir(path);
    if (!dp) {
        printf("Dir %s\n", path);
        perror("Opendir failed");
        exit(1);
    }
    entry_len = offsetof(struct dirent, d_name) + pathconf(path, _PC_NAME_MAX) + 1;
    struct dirent *entry = malloc(entry_len);
    memset(entry, 0, entry_len);
    struct dirent *result;
    readdir_r(dp, entry, &result);
    while (result) {
        if(strcmp(entry->d_name, ".") &&
            strcmp(entry->d_name, "..")) {
            int entry_path_len = strlen(path) + strlen(entry->d_name) + 2;
            char *entry_path = malloc(entry_path_len);
            memset(entry_path, 0, entry_path_len);
            if (!entry_path) {
                printf("No memory \n");
                exit(1);
            }
            sprintf(entry_path, "%s/%s", path, entry->d_name);
            memset(&buf, 0, sizeof(buf));
            /*int fd = open(entry_path, O_DIRECT | O_RDWR | O_SYNC);
            if(!fd) {
                printf("File open failed %s\n", entry_path);
                exit(1);
            }*/
            if (lstat(entry_path, &buf)) {
                printf("Stat failed on path %s\n", entry_path);
                exit(1);
            } else {
                if (do_write && !chmod(entry_path, buf.st_mode)) {
                    pthread_mutex_lock(&lock);
                    files_setattr++;
                    files_stat++;
                    pthread_mutex_unlock(&lock);
                    //printf("Stat and Chmod done %s\n", entry_path);
               } else {
                    if (!do_write) {
                        pthread_mutex_lock(&lock);
                        files_stat++;
                        pthread_mutex_unlock(&lock);
                        //printf("Stat done on %s\n", entry_path);
                    } else {
                        printf("Chmod failed on %s\n", entry_path);
                        exit(1);
                    }
                }
            }
            //close(fd);
            free(entry_path);
        }
        if (entry->d_type == DT_DIR) {
            if (!strcmp(entry->d_name, ".") ||
                !strcmp(entry->d_name, "..")) {
                memset(entry, 0, entry_len);
                readdir_r(dp, entry, &result);
                continue;
            }
            _print_entries(entry->d_name, path);
        }
        //printf("%s\n", entry->d_name);
        memset(entry, 0, entry_len);
        readdir_r(dp, entry, &result);
    }
    closedir(dp);
    if (alloc)
        free(path);
    if (entry)
        free(entry);
}



void * print_entries(void *name) {
    _print_entries((char *)name, NULL);
    pthread_exit(NULL);
}


int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage program <path> <do_write> <num_threads>\n");
        exit(1);
    }
    int i = 0;
    do_write = atoi(argv[2]);
    int num_threads = atoi(argv[3]);
    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    pthread_mutex_init(&lock, NULL);
    for (i = 0; i < num_threads; i++) {
        int ret = pthread_create(&threads[i], NULL, print_entries, (void *)argv[1]);
        if (ret) {
            printf("Thread creation failed %d\n", i);
            exit(1);
        }
    }

    for (i = 0; i < num_threads; i++) {
        void *status;
        int ret = pthread_join(threads[i], &status);
        if (ret) {
            printf("Thread join faied %d\n", i);
            exit(1);
        }
    }
    printf("Files stat %ld Files setattr %ld\n", files_stat, files_setattr);
    pthread_exit(NULL);
    //print_entries(argv[1], NULL);
}
