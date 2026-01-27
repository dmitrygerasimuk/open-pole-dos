// stackpat.c    
#include <dos.h>
#include <stdio.h>

#include "asm.h"
#include "typedefs.h"
#include "debuglog.h"
#include "sys_delay.h"
#include <stdlib.h>

// у watcom в medium модели DS=SS, сверху лежат данные, снизу — стек
// можно его пометить, но чтобы не залесть на данные лучше внимательно смтотреть 
// какой option stack = в makefile 


#define STACK_TOTAL_BYTES   ((u16)1024u)   // заполним только 1024 байт
#define STACK_PATTERN_BYTE  (0xC0+(rand() % 0x2F))
#define STACK_GUARD_TOP     ((u16)512u)

u16 g_stackpat_ss = 0;
u16 g_stackpat_sp_at_fill = 0;
u16 g_stackpat_top = 0;
u16 g_stackpat_low = 0;
u16 g_stack_pattern = 0;
u16 g_stackpat_guard = STACK_GUARD_TOP;
u16 g_stackpat_total = STACK_TOTAL_BYTES;
u8  g_stackpat_panic = 0;

 
static void set_ds(u16 v);

 
#pragma aux set_ds = "mov ds, ax" parm [ax] modify [];

 void stack_fill_pattern_early(void)
{
    u16 ss = get_ss();
    u16 sp = get_sp();
    u16 ds0 = get_ds();

    // ВАЖНО: на раннем старте DS может быть не DGROUP
    // А глобалы лежат в DGROUP. SS уже на DGROUP (там STACK).
    // Поэтому временно делаем DS=SS, пишем глобалы, возвращаем DS назад.
    set_ds(ss);

    g_stackpat_ss = ss;
    g_stackpat_sp_at_fill = sp;
    g_stackpat_top = sp;

    // в медиум режиме DS=SS один скегмент делит и данные и стек так что 
     
    

    if ((u32)STACK_TOTAL_BYTES > (u32)sp) {
        g_stackpat_panic = 1u;
        g_stackpat_low = 0u;
        set_ds(ds0);
        return;
    }

    {
        u16 low = (u16)((u32)sp - (u32)STACK_TOTAL_BYTES);
        u16 safe_top = sp;
        

        if (sp > STACK_GUARD_TOP) safe_top = (u16)(sp - STACK_GUARD_TOP);

        g_stackpat_low = low;
                     {  u32 t = bios_ticks32();
    
                          srand((u16)((u16)t ^ (u16)(t >> 16)));
                           g_stack_pattern = STACK_PATTERN_BYTE;
                }
 
        if (safe_top > low) {
            u16 fill_bytes = (u16)(safe_top - low);
            u8 far *p = (u8 far*)MK_FP(ss, low);

            while (fill_bytes--) {
                *p++ = g_stack_pattern;
            }
        }
    }

    set_ds(ds0);
}

 
void stackpat_report(void)
{
    if (g_stackpat_panic) {
        printf("STACKPAT PANIC: total=%u too big for SP=%04X (SS=%04X)\n",
               (unsigned)g_stackpat_total,
               (unsigned)g_stackpat_sp_at_fill,
               (unsigned)g_stackpat_ss);
        return;
    }

    printf("STACKPAT: SS=%04X SP=%04X low=%04X top=%04X total=%u guard=%u pat=%02X\n",
           (unsigned)g_stackpat_ss,
           (unsigned)g_stackpat_sp_at_fill,
           (unsigned)g_stackpat_low,
           (unsigned)g_stackpat_top,
           (unsigned)g_stackpat_total,
           (unsigned)g_stackpat_guard,
           (unsigned)g_stack_pattern);
}
