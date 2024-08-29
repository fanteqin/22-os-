#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

// 用于存储系统调用统计数据
typedef struct {
    const char *syscall_name;
    int call_count;
    double total_time;
} syscall_stat;

// 扩展支持的系统调用
syscall_stat syscalls[] = {
    {"lstat", 0, 0.0},
    {"getdents", 0, 0.0},
    {"read", 0, 0.0},
    {"write", 0, 0.0},
    {"open", 0, 0.0},
    {"close", 0, 0.0},
    // 添加更多系统调用...
};

#define NUM_SYSCALLS (sizeof(syscalls)/sizeof(syscalls[0]))

void update_syscall_stat(const char *syscall_name, double time_taken) {
    for (int i = 0; i < NUM_SYSCALLS; i++) {
        if (strcmp(syscalls[i].syscall_name, syscall_name) == 0) {
            syscalls[i].call_count++;
            syscalls[i].total_time += time_taken;
            return;
        }
    }
}

void print_syscall_stats(double elapsed_time) {
    printf("Time: %.1fs\n", elapsed_time);
    for (int i = 0; i < NUM_SYSCALLS; i++) {
        if (syscalls[i].call_count > 0) {
            double percentage = (syscalls[i].total_time / elapsed_time) * 100.0;
            printf("%s (%d%%)\n", syscalls[i].syscall_name, (int)percentage);
        }
    }
    printf("================\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s COMMAND [ARG]...\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }

    if (pid == 0) { // 子进程
        // 构建执行的命令及其参数
        char *exec_args[argc];
        for (int i = 0; i < argc - 1; i++) {
            exec_args[i] = argv[i + 1];
        }
        exec_args[argc - 1] = NULL;

        // 使用 execvp 执行传入的命令
        if (execvp(exec_args[0], exec_args) == -1) {
            perror("exec failed");
            exit(1);
        }
    } else { // 父进程
        struct timeval start_time, end_time;
        double elapsed_time = 0.0;

        while (1) { // 每秒钟输出一次
            gettimeofday(&start_time, NULL);

            // 模拟获取系统调用时间统计
            sleep(1); // 替换为实际的系统调用监控逻辑
            update_syscall_stat("lstat", 0.05);
            update_syscall_stat("getdents", 0.02);
            update_syscall_stat("read", 0.03);
            update_syscall_stat("write", 0.01);

            gettimeofday(&end_time, NULL);
            elapsed_time = (end_time.tv_sec - start_time.tv_sec) + 
                            (end_time.tv_usec - start_time.tv_usec) / 1000000.0;

            print_syscall_stats(elapsed_time);
        }

        int status;
        pid_t wpid;
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
            if (wpid == -1) {
                perror("waitpid failed");
                exit(1);
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 0;
}

