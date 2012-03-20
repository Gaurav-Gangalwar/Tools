#include<stdio.h>
#include<pthread.h>
#include<string.h>
#include<stdlib.h>
#include<errno.h>
#include<stat.h>
#define MAX_READ_SIZE 1024
#define MAX_WRITE_SIZE 1024
int file_size, file_count;
enum ops {
    CREAT = 0,
    READ = 1,
    WRITE = 2,
    STAT = 2,
    UNLINK = 3
}

void * read_file(void *path) {
    char *file_path = path;
    int fd = file_open(path, O_DIRECT | O_SYNC | O_RDWR);
    int total_count = 0;
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
    printf("%s : %d bytes read\n", file_path, total_count);
    close(fd);
    return NULL;
}

void * write_file(void *path) {
    char *file_path = path;
    int fd = file_open(file_path, O_DIRECT | O_SYNC | O_RDWR);
    int total_count = 0, count = 0;
    if (!fd) {
        printf("Open failed %s\n", file_path);
        return NULL;
    }
    char buf[MAX_WRITE_SIZE];
    memset(buf, 0, MAX_WRITE_SIZE);
    int write_count = file_size;
    while(write_count) {
        if (write_count <= MAX_WRITE_SIZE) {
            count = write(fd, buf, file_size);
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
            printf("Error write count negative %s write_count %d,"
                   "count %d, total_count %d\n", file_path, write_count,
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
    printf("%s : %d bytes written \n", file_path, total_count);
    return NULL;
}

void * create_file(void *path) {
    char *file_path = path;
    int fd = file_open(file_path, O_CREATE | O_RDWR);
    if (!fd) {
        printf("File open failed %s\n", file_path);
        return NULL;
    }
    close(fd);
    write(path);
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
    return NULL;
}

     
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage prog <op [0(for create),1(for read),2(for write),"
                "3(for stat),4(for unlink)]> <path> <args>\n");
        exit(1);
    }

    int op = atoi(argv[1]);
    char *path = argv[2];
    switch (op) {
        case CREAT:
            if (argc != 5) {
                printf("Usage for create prog 0 <path> <size of file>"
                        " <no. of files>\n");
                exit(1);
            }
            file_size = atoi(argv[3]);
            file_count = atoi(argv[4]);
            do_create(path, file_size, file_count);
            break;
        case READ:
            if (argc != 4) {
                printf("Usage for read prog 1 <path> <no. of files>\n");
                exit(1);
            }
            file_count = atoi(argv[3]);
            do_read(path, file_count);
            break;
        case WRITE:
            if (argc != 5) {
                printf("Usage for write prog 2 <path> <size of files>"
                        "<no. of files> \n");
                exit(1);
            }
            file_size = atoi(argv[3]);
            file_count = atoi(argv[4]);
            do_write(path, file_size, file_count);
            break;
        case STAT:
            if (argc != 4) {
                printf("Usage for stat prog 3 <path> <no of files>\n");
                exit(1);
            }
            file_count = atoi(argv[3]);
            do_stat(path, file_count);
            break;
        case UNLINK:
            if (argc != 4) {
                printf("Usage for unlink prog 4 <path> <no of files>\n");
                exit(1);
            }
            file_count = atoi(argv[3]);
            do_unlink(path, file_count);
            break
        default:
            printf("Invalid op use 0(for create), 1(for read), 2(for write),"
                    "3(for stat), 4(for unlink)\n");
            exit(1);
    }
}

