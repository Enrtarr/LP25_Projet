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
    printf("Network process manager (local + remote via ssh).\n\n");
    printf("Options:\n");
    printf("  -h, --help               Show this help and exit.\n");
    printf("      --dry-run            Test local process listing then exit.\n");
    printf("  -c, --remote-config FILE Use remote config file.\n");
    printf("  -s, --remote-server HOST Add one remote server.\n");
    printf("  -u, --username USER      Username for remote server.\n");
    printf("  -p, --password PASS      Password (stockée mais non passée à ssh).\n");
    printf("  -a, --all                Show local and all remote machines.\n");
}

static int list_to_array(process_list *list, process_info_t **out)
{
    if (!list) return 0;

    int count = 0;
    process_elem *cur = list->head;
    while (cur) {
        count++;
        cur = cur->next;
    }

    if (count == 0) {
        *out = NULL;
        return 0;
    }

    process_info_t *arr = malloc(sizeof(process_info_t) * (size_t)count);
    if (!arr) {
        perror("malloc processes");
        *out = NULL;
        return 0;
    }

    cur = list->head;
    int i = 0;
    while (cur && i < count) {
        arr[i] = cur->process;
        i++;
        cur = cur->next;
    }

    *out = arr;
    return count;
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

    tab->process_count = list_to_array(list, &tab->processes);
    free_process_list(list);

    tab->selected_proc_index = 0;
    if (old_selected_pid != -1) {
        for (int i = 0; i < tab->process_count; ++i) {
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

static int refresh_remote_tab(ui_context_t *ctx,
                              int tab_index,
                              const remotemachine_t *m)
{
    if (!ctx || !ctx->tabs || tab_index <= 0 || tab_index >= ctx->tab_count) {
        return -1;
    }
    if (!m) return -1;

    machine_tab_t *tab = &ctx->tabs[tab_index];

    process_list *list = fetch_remote_processes(m);
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

    tab->process_count = list_to_array(list, &tab->processes);
    free_process_list(list);

    tab->selected_proc_index = 0;
    if (old_selected_pid != -1) {
        for (int i = 0; i < tab->process_count; ++i) {
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

static void send_signal_selected(ui_context_t *ctx, int signum, remotemachine_t *remotes)
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

    if (ctx->current_tab_index == 0) {
        /* Onglet 0 : Local -> on utilise kill() système */
        kill(pid, signum);
    } else {
        /* Onglet > 0 : Distant -> on utilise notre nouvelle fonction SSH */
        /* L'onglet 1 correspond à remotes[0], l'onglet 2 à remotes[1], etc. */
        if (remotes) {
            send_remote_signal(&remotes[ctx->current_tab_index - 1], pid, signum);
        }
    }
}

static struct option long_options[] = {
    {"help",          no_argument,       0, 'h'},
    {"dry-run",       no_argument,       0,  1 },
    {"remote-config", required_argument, 0, 'c'},
    {"remote-server", required_argument, 0, 's'},
    {"username",      required_argument, 0, 'u'},
    {"password",      required_argument, 0, 'p'},
    {"all",           no_argument,       0, 'a'},
    {0, 0, 0, 0}
};

int main(int argc, char **argv)
{
    ui_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    remotemachine_t *remotes = NULL;
    size_t remote_count = 0;

    char *conf_path  = NULL;
    char *cli_server = NULL;
    char *cli_user   = NULL;
    char *cli_pass   = NULL;
    int include_all  = 0;
    int dry_run      = 0;

    int opt, opt_index = 0;
    while ((opt = getopt_long(argc, argv, "hc:s:u:p:a", long_options, &opt_index)) != -1) {
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
        case 'a':
            include_all = 1;
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

    /* Initialisation des onglets : au moins la machine locale */
    ctx.tabs = malloc(sizeof(machine_tab_t) * (1 + (include_all ? (int)remote_count : 0)));
    if (!ctx.tabs) {
        perror("malloc tabs");
        free(remotes);
        return EXIT_FAILURE;
    }
    memset(ctx.tabs, 0, sizeof(machine_tab_t) * (1 + (include_all ? (int)remote_count : 0)));

    ctx.tab_count = 1;
    ctx.current_tab_index = 0;
    ctx.running = 1;
    ctx.scroll_offset = 0;

    machine_tab_t *local_tab = &ctx.tabs[0];
    snprintf(local_tab->hostname, sizeof(local_tab->hostname), "Local");
    local_tab->processes = NULL;
    local_tab->process_count = 0;
    local_tab->selected_proc_index = 0;

    if (refresh_local(&ctx) != 0) {
        fprintf(stderr, "Unable to get local process list.\n");
        free(ctx.tabs);
        free(remotes);
        return EXIT_FAILURE;
    }

    /* Onglets distants si demandé */
    if (include_all && remote_count > 0) {
        for (size_t i = 0; i < remote_count; ++i) {
            machine_tab_t *tab = &ctx.tabs[ctx.tab_count];
            const remotemachine_t *m = &remotes[i];

            if (m->name[0] != '\0')
                snprintf(tab->hostname, sizeof(tab->hostname), "%s", m->name);
            else
                snprintf(tab->hostname, sizeof(tab->hostname), "%s", m->host);

            tab->processes = NULL;
            tab->process_count = 0;
            tab->selected_proc_index = 0;

            process_list *rlist = fetch_remote_processes(m);
            if (rlist) {
                tab->process_count = list_to_array(rlist, &tab->processes);
                free_process_list(rlist);
            }

            ctx.tab_count++;
        }
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
            send_signal_selected(&ctx, SIGSTOP, remotes);
            /* Rafraichissement intelligent : si local on refresh local, sinon remote */
            if (ctx.current_tab_index == 0) refresh_local(&ctx);
            else refresh_remote_tab(&ctx, ctx.current_tab_index, &remotes[ctx.current_tab_index - 1]);
            break;

        case KEY_F(6):
            send_signal_selected(&ctx, SIGTERM, remotes);
            if (ctx.current_tab_index == 0) refresh_local(&ctx);
            else refresh_remote_tab(&ctx, ctx.current_tab_index, &remotes[ctx.current_tab_index - 1]);
            break;

        case KEY_F(7):
            send_signal_selected(&ctx, SIGKILL, remotes);
            if (ctx.current_tab_index == 0) refresh_local(&ctx);
            else refresh_remote_tab(&ctx, ctx.current_tab_index, &remotes[ctx.current_tab_index - 1]);
            break;

        case KEY_F(8):
            send_signal_selected(&ctx, SIGCONT, remotes);
            if (ctx.current_tab_index == 0) refresh_local(&ctx);
            else refresh_remote_tab(&ctx, ctx.current_tab_index, &remotes[ctx.current_tab_index - 1]);
            break;
        case KEY_F(9):
            /* Refresh local et remotes */
            refresh_local(&ctx);
            if (include_all && remote_count > 0) {
                for (int t = 1; t < ctx.tab_count; ++t) {
                    refresh_remote_tab(&ctx, t, &remotes[t - 1]);
                }
            }
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
