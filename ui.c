#include "ui.h"
#include <string.h>

static void draw_header(int width)
{
    mvhline(0, 0, '-', width);
    mvprintw(0, 2, " Process Manager (Local version) ");
    mvhline(2, 0, '-', width);
    mvprintw(1, 0, "PID      USER            %%CPU   %%MEM   S  COMMAND");
}

static void draw_tabs(ui_context_t *ctx, int width)
{
    mvhline(0, 0, ' ', width);
    for (int i = 0; i < ctx->tab_count; ++i) {
        int x = 2 + i * 15;
        if (i == ctx->current_tab_index) {
            attron(A_REVERSE);
            mvprintw(0, x, "[%s]", ctx->tabs[i].hostname);
            attroff(A_REVERSE);
        } else {
            mvprintw(0, x, " %s ", ctx->tabs[i].hostname);
        }
    }
}

void ui_init(void)
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
}

void ui_clean(void)
{
    endwin();
}

void ui_draw(ui_context_t *ctx)
{
    int height, width;
    getmaxyx(stdscr, height, width);
    clear();

    draw_tabs(ctx, width);
    draw_header(width);

    if (ctx->tab_count == 0) {
        mvprintw(4, 2, "No tabs.");
        refresh();
        return;
    }

    machine_tab_t *tab = &ctx->tabs[ctx->current_tab_index];

    int list_top = 3;
    int max_rows = height - list_top - 1;
    if (max_rows < 1) max_rows = 1;

    int start = ctx->scroll_offset;
    int end = start + max_rows;
    if (end > tab->process_count) end = tab->process_count;

    for (int i = start; i < end; ++i) {
        int y = list_top + (i - start);
        process_info_t *p = &tab->processes[i];

        if (i == tab->selected_proc_index) {
            attron(A_REVERSE);
        }

        mvprintw(y, 0, "%-8d %-15s %6.2f %6.2f %2c  %-s",
                 p->pid,
                 p->user,
                 p->cpu_usage,
                 p->mem_usage,
                 p->state ? p->state : ' ',
                 p->command);

        if (i == tab->selected_proc_index) {
            attroff(A_REVERSE);
        }
    }

    mvhline(height - 1, 0, '-', width);
    mvprintw(height - 1, 2,
             "F5:STOP  F6:TERM  F7:KILL  F8:CONT  F9:REFRESH      q:quit");

    refresh();
}

int ui_input(ui_context_t *ctx)
{
    int ch = getch();
    if (ctx->tab_count == 0) {
        if (ch == 'q' || ch == KEY_F(10)) {
            ctx->running = 0;
        }
        return 0;
    }

    machine_tab_t *tab = &ctx->tabs[ctx->current_tab_index];

    int height, width;
    getmaxyx(stdscr, height, width);
    int list_top = 3;
    int max_rows = height - list_top - 1;
    if (max_rows < 1) max_rows = 1;

    switch (ch) {
        case 'q':
        case KEY_F(10):
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
                if (tab->selected_proc_index >= ctx->scroll_offset + max_rows) {
                    ctx->scroll_offset++;
                }
            }
            break;

        case KEY_UP:
            if (tab->selected_proc_index > 0) {
                tab->selected_proc_index--;
                if (tab->selected_proc_index < ctx->scroll_offset) {
                    ctx->scroll_offset--;
                    if (ctx->scroll_offset < 0) ctx->scroll_offset = 0;
                }
            }
            break;

        case KEY_NPAGE:
            tab->selected_proc_index += max_rows;
            if (tab->selected_proc_index >= tab->process_count) {
                tab->selected_proc_index = tab->process_count - 1;
            }
            ctx->scroll_offset = tab->selected_proc_index - max_rows + 1;
            if (ctx->scroll_offset < 0) ctx->scroll_offset = 0;
            break;

        case KEY_PPAGE:
            tab->selected_proc_index -= max_rows;
            if (tab->selected_proc_index < 0) tab->selected_proc_index = 0;
            ctx->scroll_offset = tab->selected_proc_index;
            if (ctx->scroll_offset < 0) ctx->scroll_offset = 0;
            break;

        case KEY_F(5):
        case KEY_F(6):
        case KEY_F(7):
        case KEY_F(8):
        case KEY_F(9):
            return ch;

        default:
            break;
    }

    return 0;
}

