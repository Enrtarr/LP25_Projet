#ifndef PROCESS_H
#define PROCESS_H

#include <stdio.h>

typedef struct {
    int pid;
    char user[32];
    double cpu_usage;
    double mem_usage;
    char state;
    char command[256];
} process_info_t;

typedef struct process_elem {
    process_info_t process;
    struct process_elem *next;
} process_elem;

typedef struct {
    process_elem *head;
} process_list;

/* Liste locale (machine sur laquelle le programme tourne) */
process_list *create_process_list(void);

/* Parse la sortie d’une commande type "ps -eo pid,user,pcpu,pmem,stat,comm" */
process_list *create_process_list_from_stream(FILE *fp);

void free_process_list(process_list *list);

#endif
