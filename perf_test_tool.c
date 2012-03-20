#define _GNU_SOURCE
#include<fcntl.h>
#include<dirent.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<errno.h>
#include<sys/stat.h>
#include<unistd.h>
#include<stddef.h>
#define MAX_READ_SIZE 65536
#define MAX_WRITE_SIZE 65536
long file_size;
int file_count, op, process_count;
enum ops {
    CREAT = 0,
    READ = 1,
    WRITE = 2,
    STAT = 3,
    UNLINK = 4
};

int file_open(char *path, int mode) {
    int fd = open(path, mode);
    if (!fd)
        printf("Open failed %s %s\n", path, strerror(errno));
    return fd;
}

void * read_file(void *path) {
    char *file_path = path;
    int fd = file_open(path, O_SYNC | O_RDWR);
    long total_count = 0;
    if (!fd) {
        printf("Open failed %s\n", file_path);
        return NULL;
    }
    char buf[MAX_READ_SIZE];
    while (1) {
        int count = read(fd, buf, MAX_READ_SIZE);
        if (count <= 0) {
            if ((count < 0)) {
                printf("Error in reading %s %s\n", file_path, strerror(errno));
                if (errno == EAGAIN)
                    continue;
            }
            break;
        }
        total_count += count;
    }
    printf("%s : %ld bytes read\n", file_path, total_count);
    close(fd);
    return NULL;
}

void * write_file(void *path) {
    char *file_path = path;
    int fd = file_open(file_path, O_SYNC | O_RDWR);
    long total_count = 0;
    int count = 0;
    if (!fd) {
        printf("Open failed %s\n", file_path);
        return NULL;
    }
    char buf[MAX_WRITE_SIZE];
    memset(buf, 0, MAX_WRITE_SIZE);
    long write_count = file_size;
    while(write_count) {
        if (write_count <= MAX_WRITE_SIZE) {
            count = write(fd, buf, write_count);
            if (count > 0)
                total_count += count;
            if (count <= 0) {
                printf("Error while writing %s %s\n", file_path, strerror(errno));
                if ((count < 0) && (errno == EAGAIN))
                    continue;
            }
            break;
        }
        count = write(fd, buf, MAX_WRITE_SIZE);
        if (count > 0) {
            total_count += count;
            write_count -= count;
        }
        if (write_count < 0) {
            printf("Error write count negative %s write_count %ld,"
                   "count %d, total_count %ld\n", file_path, write_count,
                   count, total_count);
            exit(1);
        }
        if (count <= 0) {
            printf("Error while writing %s %s\n", file_path, strerror(errno));
            if ((count < 0) && (errno == EAGAIN))
                continue;
            break;
        }
    }
    printf("%s : %ld bytes written \n", file_path, total_count);
    return NULL;
}

void * create_file(void *path) {
    char *file_path = path;
    int fd = file_open(file_path, O_CREAT | O_RDWR);
    if (!fd) {
        printf("File open failed %s\n", file_path);
        return NULL;
    }
    close(fd);
    if (file_size)
        write_file(path);
    printf("Created %s\n", file_path);
    return NULL;
}

void * stat_file(void *path) {
    struct stat buf;
    char *file_path = path;
    if (lstat((char *)file_path, &buf)) {
        printf("Stat failed on %s %s\n", file_path, strerror(errno));
        return NULL;
    }
    printf("Stat done on %s\n", file_path);
    return NULL;
}

void * unlink_file(void *path) {
    char *file_path = path;
    if (unlink(file_path)) {
        printf("Error in unlink %s %s\n", file_path, strerror(errno));
        return NULL;
    }
    printf("Unlink done %s\n", file_path);
    return NULL;
}


void * do_op(char *path) {

    switch (op) {
        case READ:
            read_file(path);
            break;
        case WRITE:
            write_file(path);
            break;
        case STAT:
            stat_file(path);
            break;
        case UNLINK:
            unlink_file(path);
            break;
        default:
            printf("Invalid op use 0(for create), 1(for read), 2(for write),"
                    "3(for stat), 4(for unlink)\n");
            exit(1);
    }
}

void _process_entries(char *name, char *parent) {
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
            if ((entry->d_type != DT_DIR) || (op == STAT)) {
                do_op(entry_path);
                process_count++;
                if (file_count && (process_count >= file_count)) {
                    printf("Files processed %d\n", process_count);
                    exit(1);
                }
            }
            free(entry_path);
        }
        if (entry->d_type == DT_DIR) {
            if (!strcmp(entry->d_name, ".") ||
                !strcmp(entry->d_name, "..")) {
                memset(entry, 0, entry_len);
                readdir_r(dp, entry, &result);
                continue;
            }
            _process_entries(entry->d_name, path);
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



void * process_entries(void *name) {
    _process_entries((char *)name, NULL);
}

void create_files(char *path) {
    int i = 0;
    char *name = "file";
    int len = strlen(path) + 10 + 1;
    char *buf = malloc(len);
    if (!buf) {
        printf("malloc failed %s %d\n", path, len);
        return;
    }
    for (i = 0; i < file_count; i++) {
        memset(buf, 0, len);
        sprintf(buf, "%s/%s%d", path, name, i);
        create_file(buf);
    }
    printf("Files created %d\n", i);
    free(buf);
}
       


     
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage prog <op [0(for create),1(for read),2(for write),"
                "3(for stat),4(for unlink)]> <path> <args>\n");
        exit(1);
    }

    op = atoi(argv[1]);
    char *path = argv[2];
    switch (op) {
        case CREAT:
            if (argc != 5) {
                printf("Usage for create prog 0 <path> <size of file>"
                        " <no. of files>\n");
                exit(1);
            }
            file_size = atol(argv[3]);
            file_count = atoi(argv[4]);
            create_files(path);
            goto out;
            break;
        case READ:
            if (argc != 4) {
                printf("Usage for read prog 1 <path> <no. of files>\n");
                exit(1);
            }
            file_count = atoi(argv[3]);
            break;
        case WRITE:
            if (argc != 5) {
                printf("Usage for write prog 2 <path> <size of files>"
                        "<no. of files> \n");
                exit(1);
            }
            file_size = atol(argv[3]);
            file_count = atoi(argv[4]);
            break;
        case STAT:
            if (argc != 4) {
                printf("Usage for stat prog 3 <path> <no of files>\n");
                exit(1);
            }
            file_count = atoi(argv[3]);
            break;
        case UNLINK:
            if (argc != 4) {
                printf("Usage for unlink prog 4 <path> <no of files>\n");
                exit(1);
            }
            file_count = atoi(argv[3]);
            break;
        default:
            printf("Invalid op use 0(for create), 1(for read), 2(for write),"
                    "3(for stat), 4(for unlink)\n");
            exit(1);
    }
    process_entries(path);
out:
    return 0;
}

