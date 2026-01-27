#ifndef RLE_DRAW_H
#define RLE_DRAW_H

#include "typedefs.h"
#include "debuglog.h"
#include "ega_draw.h"
#include "res_gfx.h"
 

static inline void draw_rle_packed_sprite_inline(
    u8 transparent,
    u16 y,
    u16 x,
    u8 far *vram,
    const u8 far *data
);


  void draw_rle_packed_sprite(
    u8 transparent,
    u16 y,
    u16 x,
    u8 far *vram,
    const u8 far *data
);




#endif
