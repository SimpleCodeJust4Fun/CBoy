#include <cpu.h>
#include <bus.h>
#include <emu.h>

cpu_context ctx = {0};

void cpu_init() {
    // ctx.regs.pc = 0x100;
}

// fetch the instruction
static void fetch_instruction() {
    // read the cur opcode from the bus
    ctx.cur_opcode = bus_read(ctx.regs.pc++); // read from pc and increase it
    ctx.cur_inst = instruction_by_opcode(ctx.cur_opcode);
    if (ctx.cur_inst == NULL) {
        printf("Unkown Instruction %02X\n", ctx.cur_opcode);
        exit(-7); 
    }
}

// fetch data for the instruction
static void fetch_data() {
    ctx.mem_dest = 0;
    ctx.dest_is_mem = false;

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
            printf("Unkonwn Addressing Mode! %d\n", ctx.cur_inst->mode);
            exit(-7);
            return;
    }
}

// execute the instruction
static void execute() {
    printf("Execute Instruction: %02X   PC: %04X\n", ctx.cur_opcode, ctx.regs.pc);
    printf("Not executing yet\n");
}

bool cpu_step() {
    if(!ctx.halted) {
        fetch_instruction();
        fetch_data();
        execute();
    }
    return true;
}
