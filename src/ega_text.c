#include <dos.h>
#include <conio.h>
#include <string.h>

#include "debuglog.h"
#include "ega_text.h"
#include "ega_draw.h"

 
void ega_mode10_set(void)
{
    union REGS r;
    r.h.ah = 0x00;
    r.h.al = 0x10; // EGA 640x350 mode 10h 
    int86(0x10, &r, &r);
}

void ega_clear_0(void)   // в draw бы его
{
    
    ega_seq_write(0x02, 0x0F);  
    ega_gc_write(0x05, 0x00);   
    _fmemset(PTR_VRAM_A000, 0, 0x8000); // 32 KB  
}

 
 void ega_text_init_write_mode2(void)
{
   
    ega_gc_write(0x01, 0x00);  
    ega_gc_write(0x03, 0x00);  
    ega_gc_write(0x05, 0x02);  
   // GC Bit Mask (idx 8) is set per row in ega_put_glyph_row() 
}

static void ega_put_glyph_row(u16 vram_off, u8 mask, u8 color)
{
    volatile u8 dummy;

    ega_gc_write(0x08, mask);     
    dummy = PTR_VRAM_A000[vram_off];  // latch load 
    PTR_VRAM_A000[vram_off] = color;  // WM2: low 4 bits act as plane data 

    (void)dummy;
}
 



void ega_draw_text_8xN(const u8 far *font, u16 glyph_h,
                       u16 x, u16 y, u8 color,
                       u16 xadvance, const char *s)
{
    ega_draw_text_8xN_vramoff(font, glyph_h, x, y, color, xadvance, s, 0x0000u);
}

void ega_draw_text_8xN_vramoff(const u8 far *font, u16 glyph_h,
                               u16 x, u16 y, u8 color,
                               u16 xadvance, const char *s,
                               u16 vram_off)
{
    u16 startx, starty;
    u16 dbg_strlen;

    if (!font || !s) return;
    ega_text_init_write_mode2();
    startx = x;
    starty = y;
    dbg_strlen = (u16)strlen(s);

    DBG("[TEXT] x:%u y:%u s:\"%s\" strlen:%u color:%u vram_off:0x%04X font:%FP\n",
        (u16)startx, (u16)starty, s, (u16)dbg_strlen,
        (u16)color, (u16)vram_off, font);

    while (*s) {
        unsigned char ch = (unsigned char)(*s++);
        u16 glyphBase = (u16)(glyph_h * (u16)ch);

        u16 xByte  = (u16)(x >> 3);
        u16 xBit   = (u16)(x & 7u);
        u16 shift2 = (u16)(8u - xBit);

         
        u16 rowBase = (u16)(vram_off + (u16)(y * EGA_BPL) + xByte);

        {
            unsigned i;
            for (i = 0u; i < glyph_h; ++i) {
                u8 rowbits = font[glyphBase + i];

                /* First byte (aligned/shifted right) */
                u8 mask1 = (u8)((u16)rowbits >> xBit);
                ega_put_glyph_row((u16)(rowBase + (u16)(i * 80u)), mask1, color);

                /* Spill into next byte if not byte-aligned */
                if (shift2 != 8u) {
                    u8 mask2 = (u8)((u16)rowbits << shift2);
                    ega_put_glyph_row((u16)(rowBase + (u16)(i * 80u) + 1u), mask2, color);
                }
            }
        }

        x = (u16)(x + xadvance);
    }

   // дебаг
    {
        unsigned long w = (unsigned long)xadvance * (unsigned long)dbg_strlen;
        unsigned h = (u16)glyph_h;

        if (vram_off > 0x6D50u) {
            DBGL("ega_draw_text_8xN_vramoff: TEXT_OFF_%p rect: x:%u y:%u w:%lu h:%u (calcoff:%04X)\n",
                 (const void *)(s - dbg_strlen),
                 (u16)startx, (u16)(starty + 400u),
                 w, h,
                 (u16)vram_off);
            SCRN_OFF(EGA_OFFSCREEN_BASE);
        } else {
            DBGL("ega_draw_text_8xN_vramoff: TEXT_%p rect: x:%u y:%u w:%lu h:%u (calcoff:%04X)\n",
                 (const void *)(s - dbg_strlen),
                 (u16)startx, (u16)starty,
                 w, h,
                 (u16)vram_off);
            SCRN();
        }
    }
}

 

void print_text(
    const u8 far *font,
    u16 vram_off,           
    u16 advance_pixels,    // x advance in pixels (usually 8 or 16)  
    u16 glyph_h_minus_1,   // 14px font => pass 13   минус один
    u8  color,
    u16 y,
    u16 x,
    const char *s
)
{
    ega_draw_text_8xN_vramoff(font,
                              (u16)(glyph_h_minus_1 + 1u),
                              x, y,
                              color,
                              advance_pixels,
                              s,
                              vram_off);
}
