#ifndef EGA_TEXT_H
#define EGA_TEXT_H

#include "typedefs.h"
 
void ega_mode10_set(void);
void ega_text_init_write_mode2(void);
void ega_clear_0(void);

 
void ega_draw_text_8xN(const u8 far *font, u16 glyph_h,
                       u16 x, u16 y, u8 color,
                       u16 xadvance, const char *s);

void ega_draw_text_8xN_vramoff(const u8 far *font, u16 glyph_h,
                       u16 x, u16 y, u8 color,
                       u16 xadvance, const char *s,u16 vram_off);
 

void print_text(
                     const u8 far *font,
  /*dstBase*/       u16 vram_off,
  /*xAdvancePixels*/  u16 advance_pixels ,
  /*glyph_h_minus_1*/ u16 glyph_h_minus_1,          
  /*egaColorIndex*/   u8 color,
  /*Y*/               u16 y,
  /*X*/               u16 x,
  /*string*/          const char *s
);


 

#endif
