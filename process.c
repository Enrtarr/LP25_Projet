#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include "process.h"

#define MAX_CMD_LEN 256
#define MAX_USER_LEN 32
#define MAX_STATE_LEN 32
#define MAX_BUF_LEN 100

process_info_t get_process_info(struct dirent *dir) {
    process_info_t process = {0};

    int PID = strtol(dir->d_name, NULL, 10);
    process.pid = PID;

    char bash_user[MAX_BUF_LEN];
    snprintf(bash_user, sizeof(bash_user), "ps aux | awk '$2 == %d {print $1}'", PID);
    FILE *file = popen(bash_user, "r");
    if (!file) {
        perror("popen failed for user");
        strncpy(process.user, "unknown", sizeof(process.user));
    } else {
        if (fgets(process.user, sizeof(process.user), file)) {
            process.user[strcspn(process.user, "\n")] = '\0';
        } else {
            strncpy(process.user, "unknown", sizeof(process.user));
        }
        pclose(file);
    }

    char bash_cpu[MAX_BUF_LEN];
    snprintf(bash_cpu, sizeof(bash_cpu), "ps aux | awk '$2 == %d {print $3}'", PID);
    file = popen(bash_cpu, "r");
    if (!file) {
        perror("popen failed for CPU usage");
        process.cpu_usage = 0.0;
    } else {
        char cpu[8];
        if (fgets(cpu, sizeof(cpu), file)) {
            process.cpu_usage = strtod(cpu, NULL);
        } else {
            process.cpu_usage = 0.0;
        }
        pclose(file);
    }

    char bash_mem[MAX_BUF_LEN];
    snprintf(bash_mem, sizeof(bash_mem), "ps aux | awk '$2 == %d {print $4}'", PID);
    file = popen(bash_mem, "r");
    if (!file) {
        perror("popen failed for memory usage");
        process.mem_usage = 0.0;
    } else {
        char mem[8];
        if (fgets(mem, sizeof(mem), file)) {
            process.mem_usage = strtod(mem, NULL);
        } else {
            process.mem_usage = 0.0;
        }
        pclose(file);
    }

    char bash_command[MAX_CMD_LEN];
    snprintf(bash_command, sizeof(bash_command), "ps aux | awk '$2 == %d {print $11}'", PID);
    file = popen(bash_command, "r");
    if (!file) {
        perror("popen failed for command");
        strncpy(process.command, "unknown", sizeof(process.command));
    } else {
        if (fgets(process.command, sizeof(process.command), file)) {
            process.command[strcspn(process.command, "\n")] = '\0';
        } else {
            strncpy(process.command, "unknown", sizeof(process.command));
        }
        pclose(file);
    }

    char bash_state[MAX_BUF_LEN];
    snprintf(bash_state, sizeof(bash_state), "ps aux | awk '$2 == %d {print $8}'", PID);
    file = popen(bash_state, "r");
    if (!file) {
        perror("popen failed for state");
        process.state = 'u';
    } else {
        if (fgets(&process.state, 2, file) == NULL) {
            process.state = 'u';
        }
        pclose(file);
    }

    return process;
}

process_list *create_process_list() {
    process_list *list = malloc(sizeof(process_list));
    if (!list) {
        perror("malloc failed");
        return NULL;
    }
    list->head = NULL;

    DIR *proc = opendir("/proc");
    if (proc == NULL) {
        perror("Failed to open /proc");
        free(list);
        return NULL;
    }
    
    struct dirent *current = NULL;
    while ((current = readdir(proc)) != NULL) {
        if (strlen(current->d_name) == strspn(current->d_name, "0123456789")) {
            process_elem *new_elem = malloc(sizeof(process_elem));
            if (!new_elem) {
                perror("malloc failed for new element");
                continue;
            }
            new_elem->process = get_process_info(current);
            new_elem->next = NULL;
            
            if (list->head == NULL) {
                list->head = new_elem;
            } else {
                process_elem *tmp = list->head;
                while (tmp->next != NULL) {
                    tmp = tmp->next;
                }
                tmp->next = new_elem;
            }
        }
    }
    
    closedir(proc);
    return list;
}

void free_process_list(process_list *list) {
    process_elem *current = list->head;
    while (current != NULL) {
        process_elem *next = current->next;
        free(current);
        current = next;
    }
    free(list);
}

int main() {
    process_list *list = create_process_list();
    if (list) {
        process_elem *current = list->head;
        while (current != NULL) {
            printf("PID: %d, User: %s, CPU Usage: %.2f%%, Memory Usage: %.2f%%, Command: %s, State: %c\n",
                   current->process.pid, current->process.user, current->process.cpu_usage,
                   current->process.mem_usage, current->process.command, current->process.state);
            current = current->next;
        }
        free_process_list(list);
    }
    
    return 0;
}

