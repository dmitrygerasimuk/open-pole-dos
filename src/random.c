// random.c - TP-like random/randomize  
#include "random.h"

#include <stdlib.h>    
#include "debuglog.h"  
#include "sys_delay.h" 

 

u16 random_seed(void)
{
    u32 t = bios_ticks32();
     
    return (u16)((u16)t ^ (u16)(t >> 16));
}

void randomize(u16 seed)
{
    srand((unsigned)seed);
    DBG("randomize: seed for this game is: %u\n", (unsigned)seed);
}

u16 tp_random_range(u16 minv, u16 maxv)
{
    u16 lo = minv;
    u16 hi = maxv;

    if (lo > hi) {
        u16 tmp = lo;
        lo = hi;
        hi = tmp;
    }

 
   
    if (lo == 0u && hi == 0xFFFFu) {
        
        return (u16)(((u16)rand() << 1) ^ (u16)rand());
    }

    return (u16)(lo + tp_random((u16)(hi - lo + 1u)));
}
