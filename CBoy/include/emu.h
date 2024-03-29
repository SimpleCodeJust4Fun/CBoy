#pragma once

#include <common.h>

typedef struct {
    bool paused;
    bool running;
    bool die;
    u64 ticks;
    u32 prev_frame;
} emu_context;

int emu_run();

emu_context *emu_get_context();

void emu_cycles(int cpu_cycles);
