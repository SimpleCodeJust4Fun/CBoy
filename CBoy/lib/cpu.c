#include <cpu.h>
#include <bus.h>
#include <emu.h>

cpu_context ctx = {0};

void cpu_init() {
    ctx.regs.pc = 0x100;
    ctx.regs.a = 0x01;
}

// fetch the instruction
static void fetch_instruction() {
    // read the cur opcode from the bus
    ctx.cur_opcode = bus_read(ctx.regs.pc++); // read from pc and increase it
    ctx.cur_inst = instruction_by_opcode(ctx.cur_opcode);
}

// fetch data for the instruction
static void fetch_data() {
    ctx.mem_dest = 0;
    ctx.dest_is_mem = false;

    if (ctx.cur_inst == NULL) {
        return;
    }
    
    switch(ctx.cur_inst->mode) {
        case AM_IMP: return; // addressing mode imply(nothing needs to be read, just return)
        case AM_R: // addressing mode register(CPU fetch data from register)
            ctx.fetched_data = cpu_read_reg(ctx.cur_inst->reg_1);
            return;
        case AM_R_D8: // addressing mode D8 to register(read an 8-bit value and transfer to register)
            ctx.fetched_data = bus_read(ctx.regs.pc);
            emu_cycles(1); // emulate the CPU cycle that took for this read
            ctx.regs.pc++;
            return;
        case AM_D16: {
            // gameboy only read 8 bit at a time
            // therefore two reads is required for a D16(low 8bit + high 8bit = 16 bit)
            u16 low = bus_read(ctx.regs.pc);
            emu_cycles(1);

            u16 high = bus_read(ctx.regs.pc + 1);
            emu_cycles(1);

            // combined the low 8bit and high 8bit to formulate a D16
            ctx.fetched_data = low | (high << 8);

            ctx.regs.pc += 2;

            return;
        }

        default:
            printf("Unkonwn Addressing Mode! %d (%02X)\n", ctx.cur_inst->mode, ctx.cur_opcode);
            exit(-7);
            return;
    }
}

// execute the instruction
static void execute() {
    IN_PROC proc = insr_get_processor(ctx.cur_inst->type);

    if (!proc) {
        NO_IMPL
    }

    proc(&ctx);
}

bool cpu_step() {
    if(!ctx.halted) {
        u16 pc = ctx.regs.pc;

        fetch_instruction();
        fetch_data();

        printf("%04X: %-7s  (%02X %02X %02X) A: %02X B: %02X C: %02X\n",
            pc, inst_name(ctx.cur_inst->type), ctx.cur_opcode,
            bus_read(pc + 1), bus_read(pc + 2), ctx.regs.a, ctx.regs.b, ctx.regs.c);
        

        // printf("Execute Instruction: %02X   PC: %04X\n", ctx.cur_opcode, pc);
        
        if (ctx.cur_inst == NULL) {
            printf("Unkown Instruction %02X\n", ctx.cur_opcode);
            exit(-7); 
        }

        execute();
    }
    return true;
}
