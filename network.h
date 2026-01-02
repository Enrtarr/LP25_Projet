#ifndef NETWORK_H
#define NETWORK_H

#include "process.h"
#include <stddef.h>

typedef struct {
    char name[64];
    char host[128];
    int  port;
    char username[64];
    char password[64];
    char type[16];
} remotemachine_t;

int load_remote_config(const char *path,
                       remotemachine_t **machines,
                       size_t *count);

int add_remote_machine(remotemachine_t **machines,
                       size_t *count,
                       const char *name,
                       const char *host,
                       int port,
                       const char *username,
                       const char *password,
                       const char *type);

int send_remote_signal(const remotemachine_t *m, int pid, int signum);

process_list *fetch_remote_processes(const remotemachine_t *m);

#endif
