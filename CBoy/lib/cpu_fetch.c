#include <cpu.h>
#include <bus.h>
#include <emu.h>

extern cpu_context ctx;

// fetch data for the instruction
void fetch_data() {
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
        
        case AM_R_R: // addressing mode register to register
            ctx.fetched_data = cpu_read_reg(ctx.cur_inst->reg_2);
            return;

        case AM_R_D8: // addressing mode D8 to register(read an 8-bit value and transfer to register)
            ctx.fetched_data = bus_read(ctx.regs.pc);
            emu_cycles(1); // emulate the CPU cycle that took for this read
            ctx.regs.pc++;
            return;
        
        case AM_R_D16:
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

        // register to memory region
        case AM_MR_R: 
            ctx.fetched_data = cpu_read_reg(ctx.cur_inst->reg_2);
            ctx.mem_dest = cpu_read_reg(ctx.cur_inst->reg_1);
            ctx.dest_is_mem = true;

            // refer to gbctr, special case for LD (C) A
            if (ctx.cur_inst->reg_1 == RT_C) {
                ctx.mem_dest |= 0xFF00; 
            }

            return;
        

        case AM_R_MR: {
            u16 addr = cpu_read_reg(ctx.cur_inst->reg_2);

            if (ctx.cur_inst->reg_2 == RT_C) {
                addr |= 0xFF00;
            }

            ctx.fetched_data = bus_read(addr);
            emu_cycles(1);
        } return;
        

        case AM_R_HLI: 
            ctx.fetched_data = bus_read(cpu_read_reg(ctx.cur_inst->reg_2));
            emu_cycles(1);
            cpu_set_reg(RT_HL, cpu_read_reg(RT_HL) + 1);
            return;
        
        case AM_R_HLD: 
            ctx.fetched_data = bus_read(cpu_read_reg(ctx.cur_inst->reg_2));
            emu_cycles(1);
            cpu_set_reg(RT_HL, cpu_read_reg(RT_HL) - 1);
            return;
        
        case AM_HLI_R:
            ctx.fetched_data = cpu_read_reg(ctx.cur_inst->reg_2);
            ctx.mem_dest = cpu_read_reg(ctx.cur_inst->reg_1);
            ctx.dest_is_mem = true;
            cpu_set_reg(RT_HL, cpu_read_reg(RT_HL) + 1);
            return;

        case AM_HLD_R:
            ctx.fetched_data = cpu_read_reg(ctx.cur_inst->reg_2);
            ctx.mem_dest = cpu_read_reg(ctx.cur_inst->reg_1);
            ctx.dest_is_mem = true;
            cpu_set_reg(RT_HL, cpu_read_reg(RT_HL) - 1);
            return;
        
        // for F0
        case AM_R_A8:
            ctx.fetched_data = bus_read(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++;
            return;

        case AM_A8_R:
            ctx.mem_dest = bus_read(ctx.regs.pc) | 0xFF00;
            ctx.dest_is_mem = true;
            emu_cycles(1);
            ctx.regs.pc++;
            return;
        
        //
        case AM_HL_SPR:
            ctx.fetched_data = bus_read(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++;
            return;
        
        case AM_D8:
            ctx.fetched_data = bus_read(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++;
            return;

        case AM_A16_R:
        case AM_D16_R: {
            u16 low = bus_read(ctx.regs.pc);
            emu_cycles(1);

            u16 high = bus_read(ctx.regs.pc + 1);
            emu_cycles(1);

            ctx.mem_dest = low | (high << 8);
            ctx.dest_is_mem = true;

            ctx.regs.pc += 2;
            ctx.fetched_data = cpu_read_reg(ctx.cur_inst->reg_2);
        } return;
        
        case AM_MR_D8:
            ctx.fetched_data = bus_read(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++;
            ctx.mem_dest = cpu_read_reg(ctx.cur_inst->reg_1);
            ctx.dest_is_mem = true;
            return;

        case AM_MR:
            ctx.mem_dest = cpu_read_reg(ctx.cur_inst->reg_1);
            ctx.dest_is_mem = true;
            ctx.fetched_data = bus_read(cpu_read_reg(ctx.cur_inst->reg_1));
            emu_cycles(1);
            return;

        case AM_R_A16: {
            u16 low = bus_read(ctx.regs.pc);
            emu_cycles(1);

            u16 high = bus_read(ctx.regs.pc + 1);
            emu_cycles(1);

            // combined the low 8bit and high 8bit to formulate a D16
            u16 addr = low | (high << 8);

            ctx.regs.pc += 2;
            ctx.fetched_data = bus_read(addr);
            emu_cycles(1);

        } return;

        default:
            printf("Unkonwn Addressing Mode! %d (%02X)\n", ctx.cur_inst->mode, ctx.cur_opcode);
            exit(-7);
            return;
    }
}
