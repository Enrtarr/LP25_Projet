#ifndef NETWORK_H
#define NETWORK_H

#include <stddef.h>

typedef struct {
    char name[64];
    char host[128];
    int  port;
    char username[64];
    char password[64];
    char type[16];
} remotemachine_t;

int add_remote_machine(remotemachine_t **machines,
                       size_t *count,
                       const char *name,
                       const char *host,
                       int port,
                       const char *username,
                       const char *password,
                       const char *type);

int load_remote_config(const char *path,
                       remotemachine_t **machines,
                       size_t *count);

#endif
