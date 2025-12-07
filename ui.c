#include "ui.h"
#include <string.h>

void ui_init(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
}

void ui_clean(void) {
    endwin();
}

void draw_tabs(ui_context_t *ctx) {
    int x = 2;


    for (int i = 0; i < ctx->tab_count; ++i) {

        if (i == ctx->current_tab_index) {
            attron(A_REVERSE);
        }

        mvprintw(2, x, " [%s] ", ctx->tabs[i].hostname);

        if (i == ctx->current_tab_index) {
            attroff(A_REVERSE);
        }
        x += strlen(ctx->tabs[i].hostname) + 6;
    }
}

void draw_proc_list(ui_context_t *ctx) {
    int row = 4;
    int max_rows = LINES - 6;
    machine_tab_t *tab = &ctx->tabs[ctx->current_tab_index];

    mvprintw(row, 2, "%-8s %-10s %-6s %-6s %-6s %s", "PID", "USER", "CPU", "MEM", "STATE", "COMMAND");
    row++;

    for (int i = 0; i < max_rows; ++i) {
        int idx = i + ctx->scroll_offset;
        
        if (idx >= tab->process_count) {
            break;
        }

        process_info_t *p = &tab->processes[idx];

        if (idx == tab->selected_proc_index) {
            attron(A_REVERSE);
            mvhline(row, 0, ' ', COLS); 
        }

        mvprintw(row, 2, "%-8d %-10s %-6.1f %-6.1f %-6c %s",
                 p->pid, p->user, p->cpu_usage, p->mem_usage, p->state, p->command);

        if (idx == tab->selected_proc_index) {
            attroff(A_REVERSE);
        }
        row++;
    }
}

void draw_footer(void) {
    int y = LINES - 1;
    mvprintw(y, 0, "F1:Help  F2/F3:Tabs  F5:Pause  F6:Stop  F7:Kill  F8:Restart  F10:Quit");
}

void ui_draw(ui_context_t *ctx) {
    erase();
    
    draw_tabs(ctx);
    draw_proc_list(ctx);
    draw_footer();

    refresh();
}

int ui_input(ui_context_t *ctx) {
    int ch = getch();
    machine_tab_t *tab = &ctx->tabs[ctx->current_tab_index];

    switch (ch) {
        case KEY_F(10):
        case 'q':
            ctx->running = 0;
            break;

        case KEY_F(2):
            ctx->current_tab_index++;
            if (ctx->current_tab_index >= ctx->tab_count) {
                ctx->current_tab_index = 0;
            }
            ctx->scroll_offset = 0;
            break;

        case KEY_F(3):
            ctx->current_tab_index--;
            if (ctx->current_tab_index < 0) {
                ctx->current_tab_index = ctx->tab_count - 1;
            }
            ctx->scroll_offset = 0;
            break;

        case KEY_DOWN:
            if (tab->selected_proc_index < tab->process_count - 1) {
                tab->selected_proc_index++;
                if (tab->selected_proc_index >= ctx->scroll_offset + (LINES - 7)) {
                    ctx->scroll_offset++;
                }
            }
            break;

        case KEY_UP:
            if (tab->selected_proc_index > 0) {
                tab->selected_proc_index--;
                if (tab->selected_proc_index < ctx->scroll_offset) {
                    ctx->scroll_offset--;
                }
            }
            break;
            
        case KEY_F(5): 
            return KEY_F(5);

        case KEY_F(6): 
            return KEY_F(6);

        case KEY_F(7): 
            return KEY_F(7);
            
        case KEY_F(8): 
            return KEY_F(8);
    }
    return 0;
}