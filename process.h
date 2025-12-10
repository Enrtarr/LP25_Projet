#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>

typedef struct {
    int pid;
    char user[32];
    double cpu_usage;
    double mem_usage;
    char command[256];
    char state;
} process_info_t;

typedef struct process_elem {
    process_info_t process;
    struct process_elem *next;
} process_elem;

typedef struct {
    process_elem *head;
} process_list;

/* Crée une liste chainée contenant les processus locaux */
process_list *create_process_list(void);

/* Libère toute la mémoire associée à la liste chainée */
void free_process_list(process_list *list);

#endif

