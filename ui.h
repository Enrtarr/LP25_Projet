#ifndef UI_H
#define UI_H

#include <ncurses.h>
#include "process.h"

/* Machine tab structure */
typedef struct {
    char hostname[64];
    process_info_t *processes;
    int process_count;
    int selected_proc_index;
} machine_tab_t;

/* UI context structure */
typedef struct {
    machine_tab_t *tabs;
    int tab_count;
    int current_tab_index;
    int running;
    int scroll_offset;
} ui_context_t;

void ui_init(void);
void ui_clean(void);
void ui_draw(ui_context_t *ctx);
int  ui_input(ui_context_t *ctx);
void ui_show_help_screen(const ui_context_t *ctx);
void ui_search_process_by_name(ui_context_t *ctx);

#endif
