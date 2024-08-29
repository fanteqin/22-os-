#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "fat32.h"  // 替换为你的头文件名

#define BMP_HEADER 0x4D42  // 'BM' in little-endian
#define CLUSTER_SIZE 512  // 通常为512字节，具体应根据FAT32头文件读取

void execute_sha1sum(const char *filename) {
    char command[256];
    snprintf(command, sizeof(command), "sha1sum %s", filename);
    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        perror("popen failed");
        exit(1);
    }

    char sha1[41];
    if (fgets(sha1, sizeof(sha1), fp) != NULL) {
        printf("%s %s\n", strtok(sha1, " "), filename);
    }

    pclose(fp);
}

int is_bmp_header(u8 *buffer) {
    return (buffer[0] == 0x42 && buffer[1] == 0x4D);  // 检查是否为 'BM'
}

void recover_bmp(int fd, u32 cluster_offset, u32 file_size, int bmp_count) {
    //const char *temp_dir = "/tmp";  // 临时目录
    char filename[64];
    snprintf(filename, sizeof(filename), "%d.bmp", bmp_count);
    int out_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (out_fd < 0) {
        perror("Failed to open output file");
        return;
    }

    u8 buffer[CLUSTER_SIZE];
    u32 bytes_remaining = file_size;
    lseek(fd, cluster_offset, SEEK_SET);

    while (bytes_remaining > 0) {
        int bytes_to_read = bytes_remaining > CLUSTER_SIZE ? CLUSTER_SIZE : bytes_remaining;
        int bytes_read = read(fd, buffer, bytes_to_read);
        if (bytes_read <= 0) {
            perror("Failed to read from image");
            break;
        }

        write(out_fd, buffer, bytes_read);
        bytes_remaining -= bytes_read;
    }

    close(out_fd);
    execute_sha1sum(filename);
}

void process_image(const char *file) {
    int fd = open(file, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open image file");
        exit(1);
    }

    struct fat32hdr hdr;
    read(fd, &hdr, sizeof(hdr));

    u32 root_dir_cluster = hdr.BPB_RootClus;
    u32 cluster_size = hdr.BPB_BytsPerSec * hdr.BPB_SecPerClus;
    u8 buffer[CLUSTER_SIZE];
    int bmp_count = 0;

        while (1) {
        u32 cluster_offset = root_dir_cluster * cluster_size;
        lseek(fd, cluster_offset, SEEK_SET);
        int read_bytes = read(fd, buffer, CLUSTER_SIZE);

        if (read_bytes != CLUSTER_SIZE) {
            perror("Failed to read cluster");
            break;
        }

        //printf("Reading cluster at offset: %u\n", cluster_offset);

        if (is_bmp_header(buffer)) {
            //printf("BMP header found at cluster offset: %u\n", cluster_offset);
            struct fat32dent *entry = (struct fat32dent *) buffer;
            u32 file_size = entry->DIR_FileSize;
            recover_bmp(fd, cluster_offset, file_size, bmp_count++);
        }

        // Move to the next cluster
        root_dir_cluster++;

        // 如果已经超出FAT表或其他合理边界，则应停止
        //if (root_dir_cluster >= some_reasonable_boundary) {
         //   break;
        //}
    }


    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <FAT32 image file>\n", argv[0]);
        exit(1);
    }

    process_image(argv[1]);
    return 0;
}