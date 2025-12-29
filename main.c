#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "ui.h"
#include "process.h"

// In english please
static void print_help(const char *prog)
{
    printf("Usage: %s [options]\n", prog);
    printf("Local-only process manager (process_manager).\n\n");
    printf("Options:\n");
    printf("  -h, --help        Show this help and exit.\n");
    printf("  --dry-run         Test local process listing then exit.\n");
}

// Refresh the local process list and update the UI context
static int refresh_local(ui_context_t *ctx)
{
    if (!ctx || ctx->tab_count == 0 || !ctx->tabs) {
        return -1;
    }

    machine_tab_t *tab = &ctx->tabs[0];
    process_list *list = create_process_list();
    if (!list) {
        return -1;
    }

    int old_selected_pid = -1;
    if (tab->processes &&
        tab->process_count > 0 &&
        tab->selected_proc_index >= 0 &&
        tab->selected_proc_index < tab->process_count) {
        old_selected_pid = tab->processes[tab->selected_proc_index].pid;
    }

    free(tab->processes);
    tab->processes = NULL;
    tab->process_count = 0;

    int count = 0;
    process_elem *cur = list->head;
    while (cur) {
        count++;
        cur = cur->next;
    }

    if (count == 0) {
        free_process_list(list);
        tab->selected_proc_index = 0;
        ctx->scroll_offset = 0;
        return 0;
    }

    tab->processes = malloc(sizeof(process_info_t) * (size_t)count);
    if (!tab->processes) {
        perror("malloc processes");
        free_process_list(list);
        return -1;
    }

    tab->process_count = count;
    cur = list->head;
    int i = 0;
    while (cur && i < count) {
        tab->processes[i] = cur->process;
        i++;
        cur = cur->next;
    }

    free_process_list(list);
    tab->selected_proc_index = 0;

    if (old_selected_pid != -1) {
        for (i = 0; i < tab->process_count; ++i) {
            if (tab->processes[i].pid == old_selected_pid) {
                tab->selected_proc_index = i;
                break;
            }
        }
        if (tab->selected_proc_index >= tab->process_count) {
            tab->selected_proc_index = tab->process_count - 1;
        }
        if (tab->selected_proc_index < 0) {
            tab->selected_proc_index = 0;
        }
    }

    ctx->scroll_offset = 0;
    return 0;
}

// Initialize UI context for local machine
static int init_local_context(ui_context_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));

    ctx->tabs = malloc(sizeof(machine_tab_t));
    if (!ctx->tabs) {
        perror("malloc tabs");
        return -1;
    }

    ctx->tab_count = 1;
    ctx->current_tab_index = 0;
    ctx->running = 1;
    ctx->scroll_offset = 0;

    machine_tab_t *tab = &ctx->tabs[0];
    memset(tab, 0, sizeof(*tab));
    snprintf(tab->hostname, sizeof(tab->hostname), "Local");
    tab->processes = NULL;
    tab->process_count = 0;
    tab->selected_proc_index = 0;

    if (refresh_local(ctx) != 0) {
        fprintf(stderr, "Unable to get local process list.\n");
        free(ctx->tabs);
        ctx->tabs = NULL;
        ctx->tab_count = 0;
        return -1;
    }

    return 0;
}

// Send a signal to the selected process in the UI context
static void send_signal_selected(ui_context_t *ctx, int signum)
{
    if (!ctx || ctx->tab_count == 0 || !ctx->tabs) {
        return;
    }

    machine_tab_t *tab = &ctx->tabs[ctx->current_tab_index];
    if (tab->process_count == 0 ||
        tab->selected_proc_index < 0 ||
        tab->selected_proc_index >= tab->process_count) {
        return;
    }

    int pid = tab->processes[tab->selected_proc_index].pid;
    if (kill(pid, signum) == -1) {
        perror("kill");
    }
}


int main(int argc, char **argv)
{
    ui_context_t ctx;

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_help(argv[0]);
            return EXIT_SUCCESS;
        } else if (strcmp(argv[1], "--dry-run") == 0) {
            process_list *list = create_process_list();
            if (!list) {
                fprintf(stderr, "Failed to get process list.\n");
                return EXIT_FAILURE;
            }
            free_process_list(list);
            printf("Local process listing: OK\n");
            return EXIT_SUCCESS;
        } else {
            fprintf(stderr, "Unknown option: %s\n\n", argv[1]);
            print_help(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (init_local_context(&ctx) != 0) {
        return EXIT_FAILURE;
    }

    ui_init();

    // Keybind gestion loop
    while (ctx.running) {
        ui_draw(&ctx);
        int action = ui_input(&ctx);
        switch (action) {
        case KEY_F(1):
            ui_show_help_screen(&ctx);
            break;
        case KEY_F(4):
            ui_search_process_by_name(&ctx);
            break;
        case KEY_F(5):
            send_signal_selected(&ctx, SIGSTOP);
            refresh_local(&ctx);
            break;
        case KEY_F(6):
            send_signal_selected(&ctx, SIGTERM);
            refresh_local(&ctx);
            break;
        case KEY_F(7):
            send_signal_selected(&ctx, SIGKILL);
            refresh_local(&ctx);
            break;
        case KEY_F(8):
            send_signal_selected(&ctx, SIGCONT);
            refresh_local(&ctx);
            break;
        case KEY_F(9):
            refresh_local(&ctx);
            break;
        default:
            break;
        }
    }

    ui_clean();

    if (ctx.tabs) {
        for (int i = 0; i < ctx.tab_count; ++i) {
            free(ctx.tabs[i].processes);
        }
        free(ctx.tabs);
    }

    return EXIT_SUCCESS;
}
