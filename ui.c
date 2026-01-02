#include "ui.h"

#include <string.h>

static void draw_header(int width)
{
    mvhline(1, 0, '-', width);
    mvprintw(1, 2, " Process Manager (Network version) ");
    mvprintw(2, 0, "PID      USER            %CPU   %MEM  S  COMMAND");
    mvhline(3, 0, '-', width);
}

static void draw_tabs(ui_context_t *ctx, int width)
{
    mvhline(0, 0, ' ', width);
    mvprintw(0, width - 20, "Tabs: %d | Idx: %d",
             ctx->tab_count, ctx->current_tab_index);

    for (int i = 0; i < ctx->tab_count; ++i) {
        int x = 2 + i * 25;
        if (i == ctx->current_tab_index) {
            attron(A_REVERSE | A_BOLD);
            mvprintw(0, x, " >> %s << ", ctx->tabs[i].hostname);
            attroff(A_REVERSE | A_BOLD);
        } else {
            mvprintw(0, x, " [%s] ", ctx->tabs[i].hostname);
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

    int list_top = 4;
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

        mvprintw(y, 0, "%-8d %-15s %6.2f %6.2f %2c %-s",
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
             "F1:HELP F2/F3:TABS F4:SEARCH F5:STOP F6:TERM F7:KILL F8:CONT F9:REFRESH q:quit");

    refresh();
}

void ui_show_help_screen(const ui_context_t *ctx)
{
    (void)ctx;

    int height, width;
    getmaxyx(stdscr, height, width);

    int box_height = 13;
    int box_width = (width > 70) ? 70 : width - 4;
    if (box_width < 40) {
        box_width = width - 2;
    }

    int start_y = (height - box_height) / 2;
    if (start_y < 0) start_y = 0;
    int start_x = (width - box_width) / 2;
    if (start_x < 0) start_x = 0;

    WINDOW *win = newwin(box_height, box_width, start_y, start_x);
    if (win == NULL) {
        return;
    }

    box(win, 0, 0);
    mvwprintw(win, 1, 2, "Help - Keyboard Shortcuts");
    mvwprintw(win, 3, 2, "F1 : show this help");
    mvwprintw(win, 4, 2, "F2/F3 : change tab");
    mvwprintw(win, 5, 2, "F4 : search a process by name (command)");
    mvwprintw(win, 6, 2, "F5 : send SIGSTOP (pause the process)");
    mvwprintw(win, 7, 2, "F6 : send SIGTERM (graceful termination)");
    mvwprintw(win, 8, 2, "F7 : send SIGKILL (immediate kill)");
    mvwprintw(win, 9, 2, "F8 : send SIGCONT (resume)");
    mvwprintw(win, 10, 2, "F9 : refresh the process list");
    mvwprintw(win, box_height - 2, 2, "Press any key to close help...");

    wrefresh(win);
    wgetch(win);
    delwin(win);
}

void ui_search_process_by_name(ui_context_t *ctx)
{
    if (!ctx || ctx->tab_count == 0 || !ctx->tabs) {
        return;
    }

    machine_tab_t *tab = &ctx->tabs[ctx->current_tab_index];
    if (tab->process_count == 0) {
        return;
    }

    int height, width;
    getmaxyx(stdscr, height, width);

    char query[64];
    memset(query, 0, sizeof(query));
    int len = 0;
    int ch;
    const char *prompt = "Search (command) : ";

    curs_set(1);

    while (1) {
        move(height - 1, 0);
        clrtoeol();
        mvprintw(height - 1, 2, "%s%s", prompt, query);
        move(height - 1, 2 + (int)strlen(prompt) + len);
        refresh();

        ch = getch();
        if (ch == '\n' || ch == '\r') {
            break;
        } else if (ch == 27) {
            query[0] = '\0';
            break;
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
            if (len > 0) {
                len--;
                query[len] = '\0';
            }
        } else if (ch >= 32 && ch <= 126 && len < (int)sizeof(query) - 1) {
            query[len++] = (char)ch;
            query[len] = '\0';
        }
    }

    curs_set(0);

    if (query[0] == '\0') {
        return;
    }

    int found = -1;
    for (int i = 0; i < tab->process_count; ++i) {
        const char *cmd = tab->processes[i].command;
        if (cmd && strstr(cmd, query) != NULL) {
            found = i;
            break;
        }
    }

    if (found != -1) {
        tab->selected_proc_index = found;

        int list_top = 4;
        int max_rows = height - list_top - 1;
        if (max_rows < 1) max_rows = 1;

        if (found < ctx->scroll_offset) {
            ctx->scroll_offset = found;
        } else if (found >= ctx->scroll_offset + max_rows) {
            ctx->scroll_offset = found - max_rows + 1;
        }
        if (ctx->scroll_offset < 0) ctx->scroll_offset = 0;
    } else {
        move(height - 2, 0);
        clrtoeol();
        mvprintw(height - 2, 2,
                 "No process contains \"%s\" in its command.", query);
        refresh();
    }
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
    int list_top = 4;
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
    case KEY_F(1):
    case KEY_F(4):
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
