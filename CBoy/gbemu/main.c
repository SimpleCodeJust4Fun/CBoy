#include <stdio.h>
#include <emu.h>
#include <cart.h>
#include <cpu.h>
#include <ui.h>
#include <timer.h>
#include <dma.h>
#include <ppu.h>
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

    ui_handle_events();

    while (ctx.running && ctx.prev_frame == ppu_get_context()->current_frame) {
        
        if (ctx.paused) {
            delay(10);
        }

        if (!cpu_step()) {
            printf("CPU Stopped\n");
            break;
        }
    }

    ui_update();
    ctx.prev_frame = ppu_get_context()->current_frame;
}

int main() {
    if (!cart_load()) {
        printf("Failed to load Tetris ROM\n");
        return -2;
    }

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