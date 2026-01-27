#ifndef SYS_DELAY_H
#define SYS_DELAY_H

#include "typedefs.h"

u32 pit_now_cycles(void);     
void pit_delay_ms(u16 ms);
void pit_delay_us(u16 us);     
void long_pit_delay_ms(u16 ms);

static u16 pit_read_counter0(void);
 u32 bios_ticks32(void);
 u32 pit_cycles_from_ms(u16 ms);
 u32 pit_cycles_from_us(u16 us);

 

u32 pit_cycles_from_ms(u16 ms);
u32 pit_cycles_from_us(u16 us);
void pit_delay_cycles(u32 cycles);
 
void pit_wait_until_cycles(u32 deadline);


#endif

