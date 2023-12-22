#pragma once

#include <common.h>
#include<instructions.h>

typedef struct {
    u8 a;
    u8 f;
    u8 b;
    u8 c;
    u8 d;
    u8 e;
    u8 h;
    u8 l;
    u16 sp;
    u16 pc;
} cpu_registers;

typedef struct {
    cpu_registers regs;

    // currrent fetch from the register
    u16 fetched_data;
    u16 mem_dest;
    bool dest_is_mem;
    u8 cur_opcode;
    instruction *cur_inst;

    // two status of CPU
    bool halted; // In x86 architecture, HLT (halt) is an assembly instruction which 
                 // halts the CPU until the next external interrupt is fired.
    
    bool stepping;
} cpu_context;

void cpu_init();
bool cpu_step();

u16 cpu_read_reg(reg_type rt);