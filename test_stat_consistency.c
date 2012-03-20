#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<sys/stat.h>

int max_repeat;

FILE * create_file(char *name) {
    FILE *fp = fopen(name, "a+");
    if(!fp) {
        printf("File creation failed %s\n", name);
        return NULL;
    }
    return fp;
}


void * _check_stat(void *fname){
    char *f_name = (char *)fname;
    FILE *fp = create_file((char *)f_name);
    if(!fp)
        goto out;
    struct stat buf;
    memset(&buf, 0, sizeof(buf));
    if(lstat(f_name, &buf)) {
        printf("Stat failed on %s\n", f_name);
        goto out;
    }
    if(buf.st_mode & S_IWOTH) {
        printf("Others have write perm on %s\n", f_name);
        goto out;
    }
    buf.st_mode |= S_IWOTH;
    if(chmod(f_name, buf.st_mode)) {
        printf("chmod failed on file %s\n", f_name);
        goto out;
    }
    memset(&buf, 0, sizeof(buf));
    if(lstat(f_name, &buf)) {
        printf("Stat failed on %s\n", f_name);
        goto out;
    }
    if(!(buf.st_mode & S_IWOTH)) {
        printf("File %s f_name don't write perm for others, test failed\n",
                 f_name);
        goto out;
    }
    if(chmod(f_name, (buf.st_mode ^ S_IWOTH))){
        printf("Reseting mode failed %s\n", f_name);
        goto out;
    }

out:
    if(fp) 
        fclose(fp);
    //free(f_name);
    //pthread_exit(NULL);
    return NULL;
}
    
void * check_stat(void *fname) {
    int i = 0;
    for(i = 0; i < max_repeat; i++)
        _check_stat(fname);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if(argc < 4) {
        printf("Usage program <path> <max_repeat> <no_of_threads>\n");
        exit(1);
    }
    char *path = (char *)argv[1];
    max_repeat = atoi(argv[2]);
    int num_threads = atoi(argv[3]);
    printf("Threads %d Repeat %d\n", num_threads, max_repeat);
    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    int i = 0, ret = 0;
    for(i = 0; i < num_threads; i++) {
        int len = strlen(path) + strlen("file") + 2 + 1;
        char *f_name = malloc(len);
        memset(f_name, 0, len);
        sprintf(f_name, "%s/file%d", path, i);
        printf("fname %s\n", f_name);
        ret = pthread_create(&threads[i], NULL, check_stat, (void *)f_name);
        if(ret) {
            printf("Thread creation failed %d\n", i);
            goto out;
        }
    }

    for(i = 0; i < num_threads; i++) {
        void *status = NULL;
        ret = pthread_join(threads[i], &status);
        if(ret) {
            printf("Thread join failed %d\n", i);
            goto out;
        }
    }

out:
   if(threads)
       free(threads);
   pthread_exit(NULL);
}
