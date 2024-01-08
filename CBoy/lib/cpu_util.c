#include <cpu.h>
#include <bus.h>

extern cpu_context ctx;

// reverse a 16-bit's low and high
// 将传入的16位无符号整数 n 的高低字节进行交换，并返回交换后的结果。
u16 reverse(u16 n) {
    return ((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8);
}

u16 cpu_read_reg(reg_type rt) {
    switch(rt) {
        case RT_A: return ctx.regs.a;
        case RT_F: return ctx.regs.f;
        case RT_B: return ctx.regs.b;
        case RT_C: return ctx.regs.c;
        case RT_D: return ctx.regs.d;
        case RT_E: return ctx.regs.e;
        case RT_H: return ctx.regs.h;
        case RT_L: return ctx.regs.l;

        // refer to https://gbdev.io/pandocs/CPU_Registers_and_Flags.html
        // AF in memory(C struct) is A then F, but the register AF is low F high A, therefore reverse is required
        case RT_AF: return reverse(*((u16 *)&ctx.regs.a)); // 使用指针强制类型转换 *((u16 *)&ctx.regs.a) 来访问寄存器 ctx.regs.a 的内存表示
        case RT_BC: return reverse(*((u16 *)&ctx.regs.b));
        case RT_DE: return reverse(*((u16 *)&ctx.regs.d));
        case RT_HL: return reverse(*((u16 *)&ctx.regs.h));

        case RT_PC: return ctx.regs.pc;
        case RT_SP: return ctx.regs.sp;
        default: return 0;
    }
}

void cpu_set_reg(reg_type rt, u16 val) {
    switch(rt) {
        case RT_A: ctx.regs.a = val & 0xFF; break;
        case RT_F: ctx.regs.f = val & 0xFF; break;
        case RT_B: ctx.regs.b = val & 0xFF; break;
        case RT_C: {
             ctx.regs.c = val & 0xFF;
        } break;
        case RT_D: ctx.regs.d = val & 0xFF; break;
        case RT_E: ctx.regs.e = val & 0xFF; break;
        case RT_H: ctx.regs.h = val & 0xFF; break;
        case RT_L: ctx.regs.l = val & 0xFF; break;

        case RT_AF: *((u16 *)&ctx.regs.a) = reverse(val); break;
        case RT_BC: *((u16 *)&ctx.regs.b) = reverse(val); break;
        case RT_DE: *((u16 *)&ctx.regs.d) = reverse(val); break;
        case RT_HL: {
         *((u16 *)&ctx.regs.h) = reverse(val); 
         break;
        }

        case RT_PC: ctx.regs.pc = val; break;
        case RT_SP: ctx.regs.sp = val; break;
        case RT_NONE: break;
    }
}

// specific for bitwise INSTRUCTIONS
u8 cpu_read_reg8(reg_type rt) {
    switch(rt) {
        case RT_A: return ctx.regs.a;
        case RT_F: return ctx.regs.f;
        case RT_B: return ctx.regs.b;
        case RT_C: return ctx.regs.c;
        case RT_D: return ctx.regs.d;
        case RT_E: return ctx.regs.e;
        case RT_H: return ctx.regs.h;
        case RT_L: return ctx.regs.l;
        case RT_HL: {
            return bus_read(cpu_read_reg(RT_HL));
        }
        default:
            printf("**ERR INVALID REG8: %d\n", rt);
            NO_IMPL
    }
}

// specific for bitwise INSTRUCTIONS
void cpu_set_reg8(reg_type rt, u8 val) {
    switch(rt) {
        case RT_A: ctx.regs.a = val & 0xFF; break;
        case RT_F: ctx.regs.f = val & 0xFF; break;
        case RT_B: ctx.regs.b = val & 0xFF; break;
        case RT_C: ctx.regs.c = val & 0xFF; break;
        case RT_D: ctx.regs.d = val & 0xFF; break;
        case RT_E: ctx.regs.e = val & 0xFF; break;
        case RT_H: ctx.regs.h = val & 0xFF; break;
        case RT_L: ctx.regs.l = val & 0xFF; break;
        case RT_HL: bus_write(cpu_read_reg(RT_HL), val); break;
        default:
            printf("**ERR INVALID REG8: %d\n", rt);
            NO_IMPL
    }
}

cpu_registers *cpu_get_regs() {
    return &ctx.regs;
}