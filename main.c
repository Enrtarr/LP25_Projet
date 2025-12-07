#include "ui.h"
#include <stdlib.h>
#include <stdio.h>

void create_test_data(machine_tab_t *tab, int count, const char *name) {
    sprintf(tab->hostname, "%s", name);
    tab->process_count = count;
    tab->processes = malloc(sizeof(process_info_t) * count);
    tab->selected_proc_index = 0;

    for (int i = 0; i < count; ++i) {
        tab->processes[i].pid = i;
        sprintf(tab->processes[i].user, "root");
        tab->processes[i].cpu_usage = 1;
        tab->processes[i].mem_usage = 2;
        tab->processes[i].state = 'R';
        sprintf(tab->processes[i].command, "Process %d", i);
    }
}

int main(void) {
    ui_context_t ctx;
    
    ctx.running = 1;
    ctx.tab_count = 2;
    ctx.current_tab_index = 0;
    ctx.scroll_offset = 0;
    ctx.tabs = malloc(sizeof(machine_tab_t) * 2);

    create_test_data(&ctx.tabs[0], 10, "Local");
    create_test_data(&ctx.tabs[1], 10, "Machine2");

    ui_init();

    while (ctx.running) {
        ui_draw(&ctx);
        ui_input(&ctx);
    }

    ui_clean();
    
    free(ctx.tabs[0].processes);
    free(ctx.tabs[1].processes);
    free(ctx.tabs);
    return 0;
}