#include "process.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

#define MAX_CMD_LEN 256
#define MAX_BUF_LEN 128

static process_info_t get_process_info(struct dirent *dir)
{
    process_info_t process;
    memset(&process, 0, sizeof(process));

    int pid = (int)strtol(dir->d_name, NULL, 10);
    process.pid = pid;

    FILE *file;
    char bash_line[MAX_BUF_LEN];

    // USER
    snprintf(bash_line, sizeof(bash_line),
             "ps -p %d -o user= 2>/dev/null", pid);
    file = popen(bash_line, "r");
    if (file != NULL) {
        if (fgets(process.user, sizeof(process.user), file) != NULL) {
            process.user[strcspn(process.user, "\n")] = '\0';
        }
        pclose(file);
    } else {
        strncpy(process.user, "unknown", sizeof(process.user));
        process.user[sizeof(process.user)-1] = '\0';
    }

    // %CPU
    snprintf(bash_line, sizeof(bash_line),
             "ps -p %d -o pcpu= 2>/dev/null", pid);
    file = popen(bash_line, "r");
    if (file != NULL) {
        char buf[32];
        if (fgets(buf, sizeof(buf), file) != NULL) {
            process.cpu_usage = strtod(buf, NULL);
        }
        pclose(file);
    }

    // %MEM
    snprintf(bash_line, sizeof(bash_line),
             "ps -p %d -o pmem= 2>/dev/null", pid);
    file = popen(bash_line, "r");
    if (file != NULL) {
        char buf[32];
        if (fgets(buf, sizeof(buf), file) != NULL) {
            process.mem_usage = strtod(buf, NULL);
        }
        pclose(file);
    }

    // COMMAND
    snprintf(bash_line, sizeof(bash_line),
             "ps -p %d -o comm= 2>/dev/null", pid);
    file = popen(bash_line, "r");
    if (file != NULL) {
        if (fgets(process.command, sizeof(process.command), file) != NULL) {
            process.command[strcspn(process.command, "\n")] = '\0';
        }
        pclose(file);
    } else {
        strncpy(process.command, "unknown", sizeof(process.command));
        process.command[sizeof(process.command)-1] = '\0';
    }

    // STATE (le premier caractère)
    snprintf(bash_line, sizeof(bash_line),
             "ps -p %d -o stat= 2>/dev/null", pid);
    file = popen(bash_line, "r");
    if (file != NULL) {
        char buf[8];
        if (fgets(buf, sizeof(buf), file) != NULL) {
            process.state = buf[0];
        }
        pclose(file);
    } else {
        process.state = 'u';
    }

    return process;
}

process_list *create_process_list(void)
{
    DIR *proc = opendir("/proc");
    if (proc == NULL) {
        perror("opendir /proc");
        return NULL;
    }

    process_list *list = malloc(sizeof(process_list));
    if (!list) {
        perror("malloc process_list");
        closedir(proc);
        return NULL;
    }
    list->head = NULL;

    struct dirent *entry;
    while ((entry = readdir(proc)) != NULL) {
        if (strspn(entry->d_name, "0123456789") != strlen(entry->d_name)) {
            continue;
        }

        process_elem *elem = malloc(sizeof(process_elem));
        if (!elem) {
            perror("malloc process_elem");
            continue;
        }

        elem->process = get_process_info(entry);
        elem->next = NULL;

        if (!list->head) {
            list->head = elem;
        } else {
            process_elem *cur = list->head;
            while (cur->next) {
                cur = cur->next;
            }
            cur->next = elem;
        }
    }

    closedir(proc);
    return list;
}

void free_process_list(process_list *list)
{
    if (!list) return;

    process_elem *cur = list->head;
    while (cur) {
        process_elem *next = cur->next;
        free(cur);
        cur = next;
    }
    free(list);
}

