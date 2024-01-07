#include <cpu.h>
#include <emu.h>
#include <bus.h>
#include <stack.h>

// process CPU instructions

void cpu_set_flags(cpu_context *ctx, char z, char n, char h, char c) {
    if (z != -1) {
        BIT_SET(ctx->regs.f, 7, z);
    }

    if (n != -1) {
        BIT_SET(ctx->regs.f, 6, n);
    }

    if (h != -1) {
        BIT_SET(ctx->regs.f, 5, h);
    }

    if (c != -1) {
        BIT_SET(ctx->regs.f, 4, c);
    }
}

static void proc_none(cpu_context *ctx) {
    printf("INVALID INSTRUCTION!\n");
    exit(-7);
}

static void proc_nop(cpu_context *ctx) {

}

static void proc_ld(cpu_context *ctx) {
    // special case 1 for memory destination
    if (ctx->dest_is_mem) {
        //LD (BC), A for instance...
        if (ctx->cur_inst->reg_2 >= RT_AF) {
            //if 16 bit register...
            emu_cycles(1);
            bus_write16(ctx->mem_dest, ctx->fetched_data);
        } else {
            bus_write(ctx->mem_dest, ctx->fetched_data);
        }
        emu_cycles(1);
        
        return;
    }
    
    // special case 2 for F8, set h and c flag
    if (ctx->cur_inst->mode == AM_HL_SPR) {
        u8 hflag = (cpu_read_reg(ctx->cur_inst->reg_2) & 0xF) + 
            (ctx->fetched_data & 0xF) >= 0x10;

        u8 cflag = (cpu_read_reg(ctx->cur_inst->reg_2) & 0xFF) + 
            (ctx->fetched_data & 0xFF) >= 0x100;

        cpu_set_flags(ctx, 0, 0, hflag, cflag);
        cpu_set_reg(ctx->cur_inst->reg_1, 
            cpu_read_reg(ctx->cur_inst->reg_2) + (char)ctx->fetched_data);

        return;
    }
    cpu_set_reg(ctx->cur_inst->reg_1, ctx->fetched_data);
}

static void proc_ldh(cpu_context *ctx) {
    if (ctx->cur_inst->reg_1 == RT_A) {
        cpu_set_reg(ctx->cur_inst->reg_1, bus_read(0xFF00 | ctx->fetched_data));
    } else {
        bus_write(0xFF00 | ctx->fetched_data, ctx->regs.a);
    }

    emu_cycles(1);
}

static void proc_xor(cpu_context *ctx) {
    // TODO
    ctx->regs.a ^= ctx->fetched_data & 0xFF;

    // according to reference html for xor inst
    cpu_set_flags(ctx, ctx->regs.a, 0, 0, 0);
}

static bool check_cond(cpu_context *ctx) {
    bool z = CPU_FLAG_Z;
    bool c = CPU_FLAG_C;

    switch(ctx->cur_inst->cond) {
        case CT_NONE: return true;
        case CT_C: return c;
        case CT_NC: return !c;
        case CT_Z: return z;
        case CT_NZ: return !z;
    }

    return false;
}

// for general jump-type instructions
static void goto_addr(cpu_context *ctx, u16 addr, bool pushpc) {
    if (check_cond(ctx)) {
        if (pushpc) {
            emu_cycles(2); // 16 bit addr for 2 cycle
            stack_push16(ctx->regs.pc);
        }
        ctx->regs.pc = addr;
        emu_cycles(1);
    }
}

static void proc_jp(cpu_context *ctx) {
    goto_addr(ctx, ctx->fetched_data, false); // jp inst need no pc push
}

static void proc_jr(cpu_context *ctx) {
    char rel = (char)(ctx->fetched_data & 0xFF); //relative value for jumping, (char) for both + and - jump
    u16 addr = ctx->regs.pc + rel;
    goto_addr(ctx, addr, false); // jp inst need no pc push
}

static void proc_call(cpu_context *ctx) {
    goto_addr(ctx, ctx->fetched_data, true); // jp inst need no pc push
}

static void proc_rst(cpu_context *ctx) {
    goto_addr(ctx, ctx->cur_inst->param, true); // jp inst need no pc push
}

static void proc_ret (cpu_context *ctx) {
    if (ctx->cur_inst->cond != CT_NONE) {
        emu_cycles(1);
    }

    if (check_cond(ctx)) {
        u16 low = stack_pop();
        emu_cycles(1);
        u16 high = stack_pop();
        emu_cycles(1);

        u16 n = (high << 8) | low;
        ctx->regs.pc = n;

        emu_cycles(1);
    }
}

// return from interrupt
static void proc_reti(cpu_context *ctx) {
    ctx->int_master_enabled = true;
    proc_ret(ctx);
}

static void proc_pop(cpu_context *ctx) {
    u16 low = stack_pop();
    emu_cycles(1);
    u16 high = stack_pop();
    emu_cycles(1);

    u16 n = (high << 8) | low;

    cpu_set_reg(ctx->cur_inst->reg_1, n);

    // special for AF
    if (ctx->cur_inst->reg_1 == RT_AF) {
        cpu_set_reg(ctx->cur_inst->reg_1, n & 0xFFF0);
    }
}

static void proc_push(cpu_context *ctx) {
    u16 high = (cpu_read_reg(ctx->cur_inst->reg_1) >> 8) & 0xFF;
    emu_cycles(1);
    stack_push(high);

    u16 low = cpu_read_reg(ctx->cur_inst->reg_2) & 0xFF;
    emu_cycles(1);
    stack_push(low);

    emu_cycles(1);
}

static void proc_di(cpu_context *ctx) {
    // DI inst will disable interrupt
    ctx->int_master_enabled = false;
}

// like a hashmap in array form
static IN_PROC processors[] = {
    [IN_NONE] = proc_none,
    [IN_NOP] = proc_nop,
    [IN_LD] = proc_ld,
    [IN_LDH] = proc_ldh,
    [IN_JP] = proc_jp,
    [IN_DI] = proc_di,
    [IN_PUSH] = proc_push,
    [IN_POP] = proc_pop,
    [IN_JR] = proc_jr,
    [IN_CALL] = proc_call,
    [IN_RET] = proc_ret,
    [IN_RST] = proc_rst,
    [IN_RETI] = proc_reti,
    [IN_XOR] = proc_xor

};

IN_PROC inst_get_processor(in_type type) {
    return processors[type];
}