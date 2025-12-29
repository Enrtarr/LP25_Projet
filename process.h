#ifndef PROCESS_H
#define PROCESS_H

#include <stdio.h>

/* Process information structure */
typedef struct {
    int pid;
    char user[32];
    double cpu_usage;
    double mem_usage;
    char state;
    char command[256];
} process_info_t;

/* Linked list element for process information */
typedef struct process_elem {
    process_info_t process;
    struct process_elem *next;
} process_elem;

/* Linked list for process information */
typedef struct {
    process_elem *head;
} process_list;

process_list *create_process_list(void);
process_list *create_process_list_from_stream(FILE *fp);
void free_process_list(process_list *list);

#endif
