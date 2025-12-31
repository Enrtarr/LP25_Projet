#define _POSIX_C_SOURCE 200809L
#include "network.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static int ensure_capacity(remotemachine_t **machines,
                           size_t *cap,
                           size_t needed)
{
    if (needed <= *cap) return 0;

    size_t newcap = (*cap == 0) ? 4 : *cap * 2;
    while (newcap < needed) newcap *= 2;

    remotemachine_t *tmp = realloc(*machines, newcap * sizeof(remotemachine_t));
    if (!tmp) {
        return -1;
    }

    *machines = tmp;
    *cap = newcap;
    return 0;
}

int add_remote_machine(remotemachine_t **machines,
                       size_t *count,
                       const char *name,
                       const char *host,
                       int port,
                       const char *username,
                       const char *password,
                       const char *type)
{
    static size_t cap = 0;

    if (ensure_capacity(machines, &cap, *count + 1) != 0) {
        perror("realloc");
        return -1;
    }

    remotemachine_t *m = &(*machines)[*count];
    memset(m, 0, sizeof(*m));

    if (name)     strncpy(m->name,     name,     sizeof(m->name) - 1);
    if (host)     strncpy(m->host,     host,     sizeof(m->host) - 1);
    m->port = port > 0 ? port : 22;
    if (username) strncpy(m->username, username, sizeof(m->username) - 1);
    if (password) strncpy(m->password, password, sizeof(m->password) - 1);
    if (type)     strncpy(m->type,     type,     sizeof(m->type) - 1);
    else          strncpy(m->type,     "ssh",    sizeof(m->type) - 1);

    (*count)++;
    return 0;
}

int load_remote_config(const char *path,
                       remotemachine_t **machines,
                       size_t *count)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        perror("fopen remote-config");
        return -1;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t n;

    while ((n = getline(&line, &len, f)) != -1) {
        if (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r'))
            line[n - 1] = '\0';

        if (line[0] == '\0' || line[0] == '#')
            continue;

        char *saveptr = NULL;
        char *name     = strtok_r(line, ":", &saveptr);
        char *addr     = strtok_r(NULL, ":", &saveptr);
        char *port_str = strtok_r(NULL, ":", &saveptr);
        char *user     = strtok_r(NULL, ":", &saveptr);
        char *pass     = strtok_r(NULL, ":", &saveptr);
        char *type     = strtok_r(NULL, ":", &saveptr);

        if (!name || !addr || !port_str || !user || !pass || !type)
            continue;

        int port = atoi(port_str);
        add_remote_machine(machines, count, name, addr, port, user, pass, type);
    }

    free(line);
    fclose(f);
    return 0;
}
