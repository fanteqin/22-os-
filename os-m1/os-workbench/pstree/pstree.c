#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/types.h>

typedef struct ProcessNode {
    pid_t pid;
    pid_t ppid;
    char comm[256];
    struct ProcessNode* children;
    struct ProcessNode* next_sibling;
} ProcessNode;

ProcessNode* find_process(ProcessNode* root, pid_t pid) {
    if (root == NULL) return NULL;
    if (root->pid == pid) return root;

    ProcessNode* found = find_process(root->children, pid);
    if (found) return found;

    return find_process(root->next_sibling, pid);
}

ProcessNode* insert_process(ProcessNode* root, pid_t pid, pid_t ppid, const char* comm) {
    ProcessNode* node = malloc(sizeof(ProcessNode));
    node->pid = pid;
    node->ppid = ppid;
    strcpy(node->comm, comm);
    node->children = NULL;
    node->next_sibling = NULL;

    if (root == NULL) {
        return node;
    }

    // 如果父节点还没有被创建，则新节点直接作为根节点的兄弟节点
    if (ppid == 0) {
        node->next_sibling = root;
        return node;
    }

    ProcessNode* parent = find_process(root, ppid);
    if (parent == NULL) {
        node->next_sibling = root;
        return node;
    }

    // 插入为父节点的第一个子节点
    if (parent->children == NULL) {
        parent->children = node;
    }
    else {
        // 否则插入为父节点的最后一个子节点的兄弟节点
        ProcessNode* sibling = parent->children;
        while (sibling->next_sibling != NULL) {
            sibling = sibling->next_sibling;
        }
        sibling->next_sibling = node;
    }

    return root;
}


void print_tree(ProcessNode* root, int depth, int is_last) {
    if (root == NULL) return;

    // 输出当前节点的缩进和连接符号
    for (int i = 0; i < depth - 1; i++) {
        printf("|   ");
    }

    if (depth > 0) {
        if (is_last) {
            printf("└── ");
        }
        else {
            printf("├── ");
        }
    }

    printf("%s(%d)\n", root->comm, root->pid);

    // 获取子节点和兄弟节点
    ProcessNode* child = root->children;
    while (child != NULL) {
        print_tree(child, depth + 1, child->next_sibling == NULL);
        child = child->next_sibling;
    }
}

int main() {
    ProcessNode* root = NULL;

    DIR* proc = opendir("/proc");
    struct dirent* entry;

    while ((entry = readdir(proc)) != NULL) {
        if (isdigit(entry->d_name[0])) {
            pid_t pid = atoi(entry->d_name);
            char filename[256];
            snprintf(filename, sizeof(filename), "/proc/%d/stat", pid);

            FILE* file = fopen(filename, "r");
            if (file == NULL) continue;

            int ppid;
            char comm[256];
            fscanf(file, "%*d %s %*c %d", comm, &ppid);
            fclose(file);

            root = insert_process(root, pid, ppid, comm);
        }
    }

    closedir(proc);

    // 查找并打印 PID 为 1 的根节点
    ProcessNode* init = find_process(root, 1);
    if (init != NULL) {
        print_tree(init, 0, 1);
    }
    else {
        printf("无法找到系统根进程\n");
    }

    return 0;
}


