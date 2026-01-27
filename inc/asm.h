#ifndef ASM_H
#define ASM_H

#include <dos.h>      
#include <conio.h>    
#include "typedefs.h"


//out dx, al  
static inline void out_dx_al(u16 port, u8 val);
#pragma aux out_dx_al = \
    "out dx, al" \
    parm [dx] [al] \
    modify [];

static inline u8 in_dx_al(u16 port);
#pragma aux in_dx_al = \
    "in al, dx" \
    parm [dx] \
    value [al] \
    modify [];

// дебаг вывод сегментов
static inline u16 get_cs(void);
static inline u16 get_ds(void);
static inline u16 get_ss(void);
static inline u16 get_es(void);
static inline u16 get_sp(void);
static inline u16 get_bp(void);

#pragma aux get_cs = "mov ax, cs" value [ax] modify [ax];
#pragma aux get_ds = "mov ax, ds" value [ax] modify [ax];
#pragma aux get_ss = "mov ax, ss" value [ax] modify [ax];
#pragma aux get_es = "mov ax, es" value [ax] modify [ax];
#pragma aux get_sp = "mov ax, sp" value [ax] modify [ax];
#pragma aux get_bp = "mov ax, bp" value [ax] modify [ax];


#endif // ASM_H
