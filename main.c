#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>

#include "ui.h"
#include "process.h"
#include "network.h"

static void print_help(const char *prog)
{
    printf("Usage: %s [options]\n", prog);
    printf("Local-only process manager (process_manager) with early network options.\n\n");
    printf("Options:\n");
    printf("  -h, --help             Show this help and exit.\n");
    printf("      --dry-run          Test local process listing then exit.\n");
    printf("  -c, --remote-config F  Load remote servers from config file.\n");
    printf("  -s, --remote-server H  Add one remote server (host name).\n");
    printf("  -u, --username USER    Username for remote server.\n");
    printf("  -p, --password PASS    Password stored but not yet used.\n");
}

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

static struct option long_options[] = {
    {"help",          no_argument,       0, 'h'},
    {"dry-run",       no_argument,       0,  1 },
    {"remote-config", required_argument, 0, 'c'},
    {"remote-server", required_argument, 0, 's'},
    {"username",      required_argument, 0, 'u'},
    {"password",      required_argument, 0, 'p'},
    {0, 0, 0, 0}
};

int main(int argc, char **argv)
{
    ui_context_t ctx;
    remotemachine_t *remotes = NULL;
    size_t remote_count = 0;
    char *conf_path = NULL;
    char *cli_server = NULL;
    char *cli_user = NULL;
    char *cli_pass = NULL;

    int dry_run = 0;
    int opt, opt_index = 0;

    while ((opt = getopt_long(argc, argv, "hc:s:u:p:", long_options, &opt_index)) != -1) {
        switch (opt) {
        case 'h':
            print_help(argv[0]);
            return EXIT_SUCCESS;
        case 1:
            dry_run = 1;
            break;
        case 'c':
            conf_path = optarg;
            break;
        case 's':
            cli_server = optarg;
            break;
        case 'u':
            cli_user = optarg;
            break;
        case 'p':
            cli_pass = optarg;
            break;
        default:
            print_help(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (dry_run) {
        process_list *list = create_process_list();
        if (!list) {
            fprintf(stderr, "Failed to get process list.\n");
            return EXIT_FAILURE;
        }
        free_process_list(list);
        printf("Local process listing: OK\n");
        return EXIT_SUCCESS;
    }

    if (conf_path) {
        load_remote_config(conf_path, &remotes, &remote_count);
    }

    if (cli_server) {
        add_remote_machine(&remotes, &remote_count,
                           cli_server, cli_server, 22,
                           cli_user, cli_pass, "ssh");
    }

    if (init_local_context(&ctx) != 0) {
        free(remotes);
        return EXIT_FAILURE;
    }

    ui_init();

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

    free(remotes);
    return EXIT_SUCCESS;
}
