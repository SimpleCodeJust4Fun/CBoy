#include <stdio.h>
#include <emu.h>
#include <cart.h>
#include <cpu.h>
#include <ui.h>
#include <timer.h>
#include <dma.h>
#include <ppu.h>

//TODO add windows thread
#include <pthread.h>
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

void *cpu_run(void *p) {
    timer_init();
    cpu_init();
    ppu_init();

    ctx.running = true;
    ctx.paused = false;
    ctx.ticks = 0;

    while(ctx.running) {
        if (ctx.paused) {
            delay(10);
            continue;
        }

        if (!cpu_step()) {
            printf("CPU Stopped\n");
            return 0;
        }
    }

    return 0;
}

int emu_run() {
    if (!tetris_load()) {
        printf("Failed to load Tetris ROM\n");
        return -2;
    }

    printf("Tetris loaded..\n");

    ui_init();

    pthread_t t1;

    if (pthread_create(&t1, NULL, cpu_run, NULL)) {
        fprintf(stderr, "Failed to create thread (Failed to start main CPU thread)\n");
        return -1;
    }

    u32 prev_frame = 0;

    while (!ctx.die) {
        usleep(1000);
        ui_handle_events();

        if (prev_frame != ppu_get_context()->current_frame) {
            // update ui only when frame changes
            ui_update();
        }
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