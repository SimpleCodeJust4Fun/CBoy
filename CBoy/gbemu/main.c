#include <stdio.h>
#include <emu.h>
#include <cart.h>
#include <cpu.h>
#include <ui.h>
#include <timer.h>
#include <dma.h>
#include <ppu.h>

#include <pthread.h>
#include <unistd.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static emu_context ctx;

bool ui_initialized;

emu_context *emu_get_context() {
    return &ctx;
}

void loop() {
    if (!ui_initialized) {
        ui_init();
        ui_initialized = 1;
    }
    printf("looping in loop func\n");

    //usleep(1000);
    ui_handle_events();

    printf("ctx.prev_frame: %u\n", ctx.prev_frame);
    printf("ppu_get_context()->current_frame: %u\n", ppu_get_context()->current_frame);

    if(ctx.running) {
        if (ctx.paused) {
            delay(10);
        }

        if (!cpu_step()) {
            printf("CPU Stopped\n");
        }
    }


    if (ctx.prev_frame != ppu_get_context()->current_frame) {
        // update ui only when frame changes
        printf("detecting frame diff loop func\n");
        ui_update();
    }

    // ui_update();

    ctx.prev_frame = ppu_get_context()->current_frame;
}

int main() {
    // Set a temporary main loop.
    if (!tetris_load()) {
        printf("Failed to load Tetris ROM\n");
        return -2;
    }

    printf("Tetris loaded..\n");

    timer_init();
    cpu_init();
    ppu_init();

    ctx.running = true;
    ctx.paused = false;
    ctx.ticks = 0;

    ctx.prev_frame = 0;

    
    #ifdef EMSCRIPTEN
        emscripten_set_main_loop(loop, 0, 1);
    #else
        while(!ctx.die) {
            loop();
        }
    #endif

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