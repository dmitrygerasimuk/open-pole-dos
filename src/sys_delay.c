#include <dos.h>
#include <conio.h>
#include <i86.h>       
#include "sys_delay.h"
#include "asm.h"

#define PIT_CMD_PORT     0x43
#define PIT_CH0_PORT     0x40
#define BIOS_TICKS_SEG   0x0040
#define BIOS_TICKS_OFS   0x006C

#define PIT_BASE_HZ      1193182ul

static u16 pit_read_counter0(void)
{
    u8 lo, hi;

    out_dx_al(PIT_CMD_PORT, 0x00);   // latch count for counter 0
    lo = (u8)in_dx_al(PIT_CH0_PORT);
    hi = (u8)in_dx_al(PIT_CH0_PORT);

    return (u16)(((u16)hi << 8) | (u16)lo);
}

// BIOS ticks as dword at 0040:006C (18.2 Hz).
 
u32 bios_ticks32(void)
{
    volatile u16 far *p = (volatile u16 far*)MK_FP(BIOS_TICKS_SEG, BIOS_TICKS_OFS);
    u16 hi1, lo, hi2;

    do {
        hi1 = p[1];
        lo  = p[0];
        hi2 = p[1];
    } while (hi1 != hi2);

    return ((u32)hi1 << 16) | (u32)lo;
}

// PIT cycles since midnight ( ticks<<16 +  доля внутри тика из счётчика PIT).
 
u32 pit_now_cycles(void)
{
    u32 t1, t2;
    u16 c1, c2;

    t1 = bios_ticks32();
    c1 = pit_read_counter0();   // latch+read
    t2 = bios_ticks32();
    c2 = pit_read_counter0();

    if (t2 != t1) {
        // тик уже обновился — берём вторую пару
        t1 = t2;
        c1 = c2;
    } else {
        // если counter пошёл назад, значит мы пересекли границу тика
        // (на самом деле PIT считает вниз, так что рост значения = wrap на 0xFFFF)
        if (c2 > c1) {
            ++t1;
            c1 = c2;
        } else {
            c1 = c2;
        }
    }

    return (t1 << 16) + (u16)(0x10000u - c1);
}

u32 pit_cycles_from_ms(u16 ms)
{
     
    // округляем к ближайшему
    return (u32)(((unsigned long long)ms * (unsigned long long)PIT_BASE_HZ + 500ull) / 1000ull);
}

u32 pit_cycles_from_us(u16 us)
{
    // cycles  us * 1.193182
   
    return (u32)(((unsigned long long)us * (unsigned long long)PIT_BASE_HZ + 500000ull) / 1000000ull);
}

void pit_delay_ms(u16 ms)
{
    u32 start = pit_now_cycles();
    u32 wait  = pit_cycles_from_ms(ms);

    while ((u32)(pit_now_cycles() - start) < wait) {
       // вроде не обрезатеся компилятором, но надо гнлянуть дизасм
    }
}

// pit_delay_ms тупит на длинных задержках — делим на куски по 32 мс
// почему? todo
void long_pit_delay_ms(u16 ms)
{
    unsigned i;
    u16 n = (u16)(ms >> 5);  // /32

    for (i = 0; i < n; ++i) {
        pit_delay_ms(32u);
    }

    // остаток (0..31)
    pit_delay_ms((u16)(ms & 31u));
}

void pit_delay_us(u16 us)
{
    u32 start = pit_now_cycles();
    u32 wait  = pit_cycles_from_us(us);

    while ((u32)(pit_now_cycles() - start) < wait) {
        // spin
    }
}

void pit_wait_until_cycles(u32 deadline)
{
    // wrap-safe сравнение через signed
    while ((s32)(deadline - pit_now_cycles()) > 0) {
         //  wait cycle
    }
}

void pit_delay_cycles(u32 cycles)
{
    u32 start = pit_now_cycles();

    while ((u32)(pit_now_cycles() - start) < cycles) {
        // wait cycle
    }
}
