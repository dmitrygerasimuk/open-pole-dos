#include <dos.h>    // inp/outp
#include <stdlib.h>  

#include "sys_delay.h"  
#include "asm.h"
#include "sound.h"
#include "globals.h"

 
#define PIT_BASE_HZ 1193181ul

void tp_sound(u16 freq_hz)
{
    u16 div;
    u8  p61;

 
    if (freq_hz <= 18u) return;

     
    div = (u16)(PIT_BASE_HZ / (u32)freq_hz);
    if (div == 0u) div = 1u;

    // включаем спикер (бит0=gate2, бит1=data2)
    p61 = (u8)in_dx_al(0x61);
    if ((p61 & 0x03u) != 0x03u) {
        out_dx_al(0x61, (u8)(p61 | 0x03u));
    }

    // PIT ch2: lobyte/hibyte, mode 3 (square wave), binary
    // control пишем всегда в  0x43
    out_dx_al(0x43, 0xB6);
    out_dx_al(0x42, (u8)(div & 0xFFu));
    out_dx_al(0x42, (u8)(div >> 8));
}

void tp_nosound(void)
{
    u8 p61 = (u8)in_dx_al(0x61);
    out_dx_al(0x61, (u8)(p61 & (u8)~0x03u));
}

void sound_play6_notes(void)
{
    unsigned i;

 
    for (i = 1u; i <= 6u; ++i) {
        u16 r;
        u32 f32;

        
        r = (u16)(rand() % 100u);

        
        f32 = (u32)r * (u32)g_soundONOF_soundMultiplier;
        tp_sound((u16)f32);

        
        pit_delay_ms((u16)(6u - i));

        tp_nosound();

 
        pit_delay_ms(1u);
    }
}
