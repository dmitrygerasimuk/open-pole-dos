 
#ifndef RANDOM_H_
#define RANDOM_H_

#include <stdint.h>
#include <stdlib.h>
#include "typedefs.h"

// Seed RNG from BIOS tick counter (BDA 0040:006C) 
void randomize(u16 seed);
u16 random_seed();
// как в турбопаскале 
static inline u16 tp_random(u16 range)
{
    if (range == 0) return 0;                // защита от %0  
    return (u16)((u16)rand() % range);      // rand() даёт int 
}

 
u16 tp_random_range(u16 minv, u16 maxv);

 
#endif /* RANDOM_H_ */
