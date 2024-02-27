#include <stdio.h>
#include <emu.h>
#include <cart.h>
#include <cpu.h>
#include <ui.h>
#include <timer.h>
#include <dma.h>
#include <ppu.h>

#include <unistd.h>
/* 
  Emu components:

  |Cart|
  |CPU|
  |Address Bus|
  |PPU|
  |Timer|

*/

static emu_context ctx;

emu_context *emu_get_context() {
    return &ctx;
}

int emu_run(int argc, char **argv) {
    if (argc > 1) {
        printf("Parameter not supported\n");
        return -1;
    }

    if (!tetris_load()) {
        printf("Failed to load Tetris ROM\n");
        return -2;
    }

    printf("Tetris loaded..\n");

    
    timer_init();
    cpu_init();
    ppu_init();
    ui_init();

    ctx.running = true;
    ctx.paused = false;
    ctx.ticks = 0;
    u32 prev_frame = 0;
    while(ctx.running && !ctx.die) {
        if (ctx.paused) {
            delay(10);
            continue;
        }

        if (!cpu_step()) {
            printf("CPU Stopped\n");
            return 0;
        }
    // }

    // while (!ctx.die) {
        usleep(1000);
        ui_handle_events();
        ui_update();

        prev_frame = ppu_get_context()->current_frame;
    }

    return 0;
}

void emu_cycles(int cpu_cycles) {
    for (int i=0; i<cpu_cycles; i++) {
        for (int n=0; n<4; n++) {
            ctx.ticks++;
            timer_tick();
            ppu_tick();
        }
        dma_tick();
    }
}
