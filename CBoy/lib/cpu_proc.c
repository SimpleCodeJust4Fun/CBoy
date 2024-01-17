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

reg_type rt_lookup[] = {
    RT_B,
    RT_C,
    RT_D,
    RT_E,
    RT_H,
    RT_L,
    RT_HL,
    RT_A,
};


reg_type decode_reg(u8 reg) {
    if (reg > 0b111) {
        return RT_NONE;
    }

    return rt_lookup[reg];
}

// for prefix cb insts
static void proc_cb(cpu_context *ctx) {
    u8 op = ctx->fetched_data;
    reg_type reg = decode_reg(op & 0b111); // low 3 bits for bit operation register (eg. D in `RES 2,D`)
    u8 bit = (op >> 3) & 0b111; // middle 3 bits for bit operation number (eg. 2 in `RES 2,D`)
    u8 bit_op = (op >> 6) & 0b11; // top 2 bits indicates INST (eg. RES in `RES 2,D`)
    u8 reg_val = cpu_read_reg8(reg);

    emu_cycles(1);
    
    if (reg == RT_HL) {
        emu_cycles(2);
    }

    switch(bit_op) {
        case 1:
            // BIT (whether the bit is set on that register, if bit set, we set zero flag)
            cpu_set_flags(ctx, !(reg_val & (1 << bit)), 0, 1, -1);
            return;

        case 2:
            // RES (reset)
            reg_val &= ~(1 << bit);
            cpu_set_reg8(reg, reg_val);
            return;

        case 3:
            // SET
            reg_val |= (1 << bit);
            cpu_set_reg8(reg, reg_val);
            return;
    }

    bool flagC = CPU_FLAG_C;

    switch(bit) {
        case 0: {
            //RLC
            bool setC = false;
            u8 result = (reg_val << 1) & 0xFF;

            if ((reg_val & (1 << 7)) != 0) {
                result |= 1;
                setC = true;
            }

            cpu_set_reg8(reg, result);
            cpu_set_flags(ctx, result == 0, false, false, setC);
        } return;

        case 1: {
            //RRC
            u8 old = reg_val;
            reg_val >>= 1;
            reg_val |= (old << 7);

            cpu_set_reg8(reg, reg_val);
            cpu_set_flags(ctx, !reg_val, false, false, old & 1);
        } return;

        case 2: {
            //RL
            u8 old = reg_val;
            reg_val <<= 1;
            reg_val |= flagC;

            cpu_set_reg8(reg, reg_val);
            cpu_set_flags(ctx, !reg_val, false, false, !!(old & 0x80));
        } return;

        case 3: {
            //RR
            u8 old = reg_val;
            reg_val >>= 1;

            reg_val |= (flagC << 7);

            cpu_set_reg8(reg, reg_val);
            cpu_set_flags(ctx, !reg_val, false, false, old & 1);
        } return;

        case 4: {
            //SLA
            u8 old = reg_val;
            reg_val <<= 1;

            cpu_set_reg8(reg, reg_val);
            cpu_set_flags(ctx, !reg_val, false, false, !!(old & 0x80));
        } return;

        case 5: {
            //SRA
            u8 u = (int8_t)reg_val >> 1;
            cpu_set_reg8(reg, u);
            cpu_set_flags(ctx, !u, 0, 0, reg_val & 1);
        } return;

        case 6: {
            //SWAP
            reg_val = ((reg_val & 0xF0) >> 4) | ((reg_val & 0xF) << 4);
            cpu_set_reg8(reg, reg_val);
            cpu_set_flags(ctx, reg_val == 0, false, false, false);
        } return;

        case 7: {
            //SRL
            u8 u = reg_val >> 1;
            cpu_set_reg8(reg, u);
            cpu_set_flags(ctx, !u, 0, 0, reg_val & 1);
        } return;

        fprintf(stderr, "ERROR: INVALID CB: %02X", op);
        NO_IMPL
    }

}


static void proc_rlca(cpu_context *ctx) {
    u8 u = ctx->regs.a;
    bool c = (u >> 7) & 1; // whether c should be set
    u = (u << 1) | c;
    ctx->regs.a = u;

    cpu_set_flags(ctx, 0, 0, 0, c);
}

static void proc_rrca(cpu_context *ctx) {
    u8 b = ctx->regs.a & 1;
    ctx->regs.a >>= 1;
    ctx->regs.a |= (b << 7);

    cpu_set_flags(ctx, 0, 0, 0, b);
}

static void proc_rla(cpu_context *ctx) {
    u8 u = ctx->regs.a;
    u8 cf = CPU_FLAG_C;
    u8 c = (u >> 7) & 1; // whether c should be set
 
    ctx->regs.a = (u << 1) | cf;
    cpu_set_flags(ctx, 0, 0, 0, c);
}

static void proc_rra(cpu_context *ctx) {
    u8 carry = CPU_FLAG_C;
    u8 new_c = ctx->regs.a & 1;

    ctx->regs.a >>= 1;
    ctx->regs.a |= (carry << 7);

    cpu_set_flags(ctx, 0, 0, 0, new_c);
}

static void proc_daa(cpu_context *ctx) {
    u8 u = 0;
    int cf = 0;

    if (CPU_FLAG_H || (!CPU_FLAG_N && (ctx->regs.a & 0xF) > 9)) {
        u = 6;
    }

    if (CPU_FLAG_C || (!CPU_FLAG_N && ctx->regs.a > 0x99)) {
        u |= 0x60;
        cf = 1;
    }

    ctx->regs.a += CPU_FLAG_N ? -u : u;

    cpu_set_flags(ctx, ctx->regs.a == 0, -1, 0, cf);
}

static void proc_cpl(cpu_context *ctx) {
    ctx->regs.a = ~ctx->regs.a;
    cpu_set_flags(ctx, -1, 1, 1, -1);
}

static void proc_scf(cpu_context *ctx) {
    cpu_set_flags(ctx, -1, 0, 0, 1);
}

static void proc_ccf(cpu_context *ctx) {
    cpu_set_flags(ctx, -1, 0, 0, CPU_FLAG_C ^ 1);
}

static void proc_halt(cpu_context *ctx) {
    ctx->halted = true;
}

static void proc_stop(cpu_context *ctx) {
    fprintf(stderr, "STOP!\n");
    NO_IMPL
}

static void proc_and(cpu_context *ctx) {
    ctx->regs.a &= ctx->fetched_data;
    cpu_set_flags(ctx, ctx->regs.a == 0, 0, 1, 0);
}

static void proc_xor(cpu_context *ctx) {
    ctx->regs.a ^= ctx->fetched_data & 0xFF;
    cpu_set_flags(ctx, ctx->regs.a == 0, 0, 0, 0);
}

static void proc_or(cpu_context *ctx) {
    ctx->regs.a |= ctx->fetched_data & 0xFF;
    cpu_set_flags(ctx, ctx->regs.a == 0, 0, 0, 0);
}

static void proc_cp(cpu_context *ctx) {
    int n = (int)ctx->regs.a - (int)ctx->fetched_data;

    cpu_set_flags(ctx, n == 0, 1,
        ((int)ctx->regs.a & 0x0F) - ((int)ctx->fetched_data & 0x0F) < 0, n < 0);
}

static void proc_di(cpu_context *ctx) {
    // disable interrupt
    ctx->int_master_enabled = false;
}

static void proc_ei(cpu_context *ctx) {
    // not immediately set int_master_enabled (requires extra cpu cycle)
    ctx->enabling_ime = true;
}

static bool is_16_bit(reg_type rt) {
    return rt >= RT_AF;
}

static void proc_ld(cpu_context *ctx) {
    // special case 1 for memory destination
    if (ctx->dest_is_mem) {
        //LD (BC), A for instance...
        if (is_16_bit(ctx->cur_inst->reg_2)) {
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
        //bus_write(0xFF00 | ctx->fetched_data, ctx->regs.a);
        bus_write(ctx->mem_dest, ctx->regs.a);
    }

    emu_cycles(1);
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

    u16 low = cpu_read_reg(ctx->cur_inst->reg_1) & 0xFF;
    emu_cycles(1);
    stack_push(low);

    emu_cycles(1);
}


static void proc_inc(cpu_context *ctx) {
    u16 val = cpu_read_reg(ctx->cur_inst->reg_1) + 1;

    if (is_16_bit(ctx->cur_inst->reg_1)) {
        emu_cycles(1); // add an extra cycle if 16 bit
    }

    if (ctx->cur_inst->reg_1 == RT_HL && ctx->cur_inst->mode == AM_MR) {
        val = bus_read(cpu_read_reg(RT_HL)) + 1;
        val &= 0xFF; // in case of overflow
        bus_write(cpu_read_reg(RT_HL), val);
    } else {
        cpu_set_reg(ctx->cur_inst->reg_1, val);
        val = cpu_read_reg(ctx->cur_inst->reg_1);
    }

    if ((ctx->cur_opcode & 0x03) == 0x03) {
        return; // for those INCs that do not set flags
    }

    cpu_set_flags(ctx, val == 0, 0, (val & 0x0F) == 0, -1);
}

static void proc_dec(cpu_context *ctx) {
    u16 val = cpu_read_reg(ctx->cur_inst->reg_1) - 1;

    if (is_16_bit(ctx->cur_inst->reg_1)) {
        emu_cycles(1); // add an extra cycle if 16 bit
    }

    if (ctx->cur_inst->reg_1 == RT_HL && ctx->cur_inst->mode == AM_MR) {
        val = bus_read(cpu_read_reg(RT_HL)) - 1;
        bus_write(cpu_read_reg(RT_HL), val);
    } else {
        cpu_set_reg(ctx->cur_inst->reg_1, val);
        val = cpu_read_reg(ctx->cur_inst->reg_1);
    }

    if ((ctx->cur_opcode & 0x0B) == 0x0B) {
        return; // for those DECs that do not set flags
    }

    cpu_set_flags(ctx, val == 0, 1, (val & 0x0F) == 0x0F, -1);
}

static void proc_sub(cpu_context *ctx) {
    u16 val = cpu_read_reg(ctx->cur_inst->reg_1) - ctx->fetched_data;

    int z = val == 0;
    int h = ((int)cpu_read_reg(ctx->cur_inst->reg_1) & 0xF) - ((int)ctx->fetched_data & 0xF) < 0;
    int c = ((int)cpu_read_reg(ctx->cur_inst->reg_1)) - ((int)ctx->fetched_data) < 0;

    cpu_set_reg(ctx->cur_inst->reg_1, val);
    cpu_set_flags(ctx, z, 1, h, c);
}

static void proc_sbc(cpu_context *ctx) {
    u8 val = ctx->fetched_data + CPU_FLAG_C;

    int z = cpu_read_reg(ctx->cur_inst->reg_1) - val == 0;

    int h = ((int)cpu_read_reg(ctx->cur_inst->reg_1) & 0xF) 
        - ((int)ctx->fetched_data & 0xF) - ((int)CPU_FLAG_C)< 0;
    int c = ((int)cpu_read_reg(ctx->cur_inst->reg_1)) 
        - ((int)ctx->fetched_data) - ((int)CPU_FLAG_C) < 0;

    cpu_set_reg(ctx->cur_inst->reg_1, cpu_read_reg(ctx->cur_inst->reg_1) - val);
    cpu_set_flags(ctx, z, 1, h, c);
}

// simpler than add because adc only works on accumulator(reg A)
static void proc_adc(cpu_context *ctx) {
    u16 u = ctx->fetched_data;
    u16 a = ctx->regs.a;
    u16 c = CPU_FLAG_C;

    ctx->regs.a = (a + u + c) & 0xFF;

    cpu_set_flags(ctx, ctx->regs.a == 0, 0,
        (a & 0xF) + (u & 0xF) + c > 0xF,
        a + u + c > 0xFF);
}

static void proc_add(cpu_context *ctx) {
    // u32 in case of overflow when adding two u16
    u32 val = cpu_read_reg(ctx->cur_inst->reg_1) + ctx->fetched_data;

    bool is_16bit = is_16_bit(ctx->cur_inst->reg_1);

    if (is_16bit) {
        emu_cycles(1);
    }

    if (ctx->cur_inst->reg_1 == RT_SP) {
        // char for both +- value
        val = cpu_read_reg(ctx->cur_inst->reg_1) + (char)ctx->fetched_data;
    }

    int z = (val & 0xFF) == 0;
    int h = (cpu_read_reg(ctx->cur_inst->reg_1) & 0xF) + (ctx->fetched_data & 0xF) >= 0x10;
    int c = (int)(cpu_read_reg(ctx->cur_inst->reg_1) & 0xFF) + (int)(ctx->fetched_data & 0xFF) >= 0x100;

    if (is_16bit) {
        z = -1;
        h = (cpu_read_reg(ctx->cur_inst->reg_1) & 0xFFF) + (ctx->fetched_data & 0xFFF) >= 0x1000;
        u32 n = ((u32)cpu_read_reg(ctx->cur_inst->reg_1)) + ((u32)ctx->fetched_data);
        c = n >= 0x10000;
    }

    if (ctx->cur_inst->reg_1 == RT_SP) {
        z = 0;
        h = (cpu_read_reg(ctx->cur_inst->reg_1) & 0xF) + (ctx->fetched_data & 0xF) >= 0x10;
        c = (int)(cpu_read_reg(ctx->cur_inst->reg_1) & 0xFF) + (int)(ctx->fetched_data & 0xFF) >= 0x100;
    }

    cpu_set_reg(ctx->cur_inst->reg_1, val & 0xFFFF);
    cpu_set_flags(ctx, z, 0, h, c);
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
    [IN_DEC] = proc_dec,
    [IN_INC] = proc_inc,
    [IN_RETI] = proc_reti,
    [IN_ADD] = proc_add,
    [IN_ADC] = proc_adc,
    [IN_SUB] = proc_sub,
    [IN_SBC] = proc_sbc,
    [IN_AND] = proc_and,
    [IN_OR] = proc_or,
    [IN_CP] = proc_cp,
    [IN_CB] = proc_cb,
    [IN_RLCA] = proc_rlca,
    [IN_RRCA] = proc_rrca,
    [IN_RLA] = proc_rla,
    [IN_RRA] = proc_rra,
    [IN_STOP] = proc_stop,
    [IN_HALT] = proc_halt,
    [IN_DAA] = proc_daa,
    [IN_CPL] = proc_cpl,
    [IN_SCF] = proc_scf,
    [IN_CCF] = proc_ccf,
    [IN_EI] = proc_ei,
    [IN_XOR] = proc_xor

};

IN_PROC inst_get_processor(in_type type) {
    return processors[type];
}

// DBG: 07-jr,jp,call,ret,rst
// DBG: 03-op sp,hl
// DBG: 08-misc instrs