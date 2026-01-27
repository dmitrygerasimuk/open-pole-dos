#ifndef EGA_DRAW_H
#define EGA_DRAW_H

#include <dos.h>      
#include <conio.h>    
#include "typedefs.h"
#include "asm.h"

 
// EGA constants
 #define CRTC_IDX 0x3D4
#define CRTC_DAT 0x3D5

#define EGA_SCR_W 640
#define EGA_SCR_H 350

#define EGA_COLOR_BLACK          0u
#define EGA_COLOR_BLUE           1u
#define EGA_COLOR_GREEN          2u
#define EGA_COLOR_CYAN           3u
#define EGA_COLOR_RED            4u
#define EGA_COLOR_MAGENTA        5u
#define EGA_COLOR_BROWN          6u
#define EGA_COLOR_LIGHT_GRAY     7u

#define EGA_COLOR_DARK_GRAY      8u
#define EGA_COLOR_LIGHT_BLUE     9u
#define EGA_COLOR_LIGHT_GREEN    10u
#define EGA_COLOR_LIGHT_CYAN     11u
#define EGA_COLOR_LIGHT_RED      12u
#define EGA_COLOR_LIGHT_MAGENTA  13u
#define EGA_COLOR_YELLOW         14u
#define EGA_COLOR_WHITE          15u

// canonical palette codes (0..63)
#define EGA_COLOR_BLACK_CODE          0u
#define EGA_COLOR_BLUE_CODE           1u
#define EGA_COLOR_GREEN_CODE          2u
#define EGA_COLOR_CYAN_CODE           3u
#define EGA_COLOR_RED_CODE            4u
#define EGA_COLOR_MAGENTA_CODE        5u
#define EGA_COLOR_BROWN_CODE          20u
#define EGA_COLOR_LIGHT_GRAY_CODE     7u

#define EGA_COLOR_DARK_GRAY_CODE      56u
#define EGA_COLOR_LIGHT_BLUE_CODE     57u
#define EGA_COLOR_LIGHT_GREEN_CODE    58u
#define EGA_COLOR_LIGHT_CYAN_CODE     59u
#define EGA_COLOR_LIGHT_RED_CODE      60u
#define EGA_COLOR_LIGHT_MAGENTA_CODE  61u
#define EGA_COLOR_YELLOW_CODE         62u
#define EGA_COLOR_WHITE_CODE          63u

#define EGA_COLOR_NO_TRANSPARENT 0x10


#define EGA_BPL       80u
#define EGA_W         640u
#define EGA_H         350u
#define EGA_VIDEO_SEGMENT 0xA000u
#define EGA_OFFSCREEN_BASE    0x7D00u // A000:7D00

#define VRAM_OFFSET_ONSCREEN 0u
#define EGA_SCREEN_BYTES      (EGA_BPL * EGA_H) // 28000 = 0x6D60
 

// Координаты -> оффсет в A000:0000, x в пикселях (кратно 1), y в строках
#define X_Y_TO_OFFSET(x, y)   ((u16)((u16)(y) * (u16)EGA_BPL + (u16)((u16)(x) / 8u)))
#define X_TO_XBYTE(x_px)      ((u16)((u16)(x_px) / 8u))

#define PTR_VRAM_A000             ((u8 __far *)MK_FP(EGA_VIDEO_SEGMENT, 0))
#define PTR_VRAM_7D00_OFFSCREEN   ((u8 __far *)MK_FP(EGA_VIDEO_SEGMENT, EGA_OFFSCREEN_BASE))

static inline u8 __far *vram_ptr(u16 off)
{
    return (u8 __far*)MK_FP(EGA_VIDEO_SEGMENT, off);
}

// -------------------------------------------------------------------------
 



void ega_gc_write_asm(u8 index, u8 data);
#pragma aux ega_gc_write_asm = \
    "mov dx, 03CEh" \
    "out dx, al" \
    "inc dx" \
    "mov al, ah" \
    "out dx, al" \
    parm [al] [ah] \
    modify [dx al];

static inline void ega_gc_write(u8 index, u8 data)
{
    ega_gc_write_asm(index, data);
}
 

void ega_gc_data(u8 data);
#pragma aux ega_gc_data = \
    "mov dx, 03CFh" \
    "out dx, al" \
    parm [al] \
    modify [dx];

static inline void EGA_SET_MASK(u8 m) { ega_gc_data(m); }


static __inline u8 ega_gc_read(u8 idx)
{
    out_dx_al(0x3CE, idx);
    return (u8)in_dx_al(0x3CF);
}

static __inline void ega_seq_write(u8 idx, u8 val)
{
    out_dx_al(0x3C4, idx);
    out_dx_al(0x3C5, val);
}

static __inline u8 ega_seq_read(u8 idx)
{
    out_dx_al(0x3C4, idx);
    return (u8)in_dx_al(0x3C5);
}
 
// Fast putpixel macro (WM2)
 
#define MACRO_EGA_PUTPIXEL_FAST_BEGIN()                    \
do {                                                       \
    /* GC Mode: write mode 2 */                            \
    ega_gc_write(0x05u, 0x02u);                             \
    /* select BitMask register */                          \
    out_dx_al(0x03CEu, 0x08u); /* index=8 */               \
} while (0)

#define MACRO_EGA_PUTPIXEL_FAST(x_, y_, color_)                                  \
do {                                                                             \
    unsigned int  __xx   = (unsigned int)(x_);                                   \
    unsigned int  __yy   = (unsigned int)(y_);                                   \
    unsigned int  __off  = (unsigned int)(__yy * 80u + (__xx >> 3));             \
    unsigned char __mask = (unsigned char)(0x80u >> (__xx & 7u));                \
    volatile unsigned char __far* __v =                                          \
        (volatile unsigned char __far*)MK_FP(EGA_VIDEO_SEGMENT, __off);          \
    volatile unsigned char __dummy;                                              \
    out_dx_al(0x3CF, __mask);                                                         \
    __dummy = *__v; (void)__dummy;                                               \
    *__v = (unsigned char)((color_) & 0x0F);                                     \
} while (0)

 
// API
 

// Возвращает объём VRAM по BIOS (обычно 64/128/192/256), 0 если непонятно
u16 ega_bios_vram_kb(void);

// Проверка наличия VGA BIOS (INT10h AX=1A00h)
u16 is_vga(void);

// Оффсет внутри строки 640 (80 bytes/row) по координатам
u16 ega_off_640(s16 x, s16 y);

void ega_draw_grid_10px(void);

void ega_wm2_putpixel(u8 __far *vram, s16 x, s16 y, u8 color);
void ega_wm2_hline_fast(u8 __far *vram, s16 x1, s16 x2, s16 y, u8 color);
void ega_wm2_vline_fast(u8 __far *vram, s16 x, s16 y1, s16 y2, u8 color);

void ega_wm2_bar(u8 __far *vram, s16 x1, s16 y1, s16 x2, s16 y2, u8 color);
void ega_wm2_bar_u(u8 __far *vram, u16 x1, u16 y1, u16 x2, u16 y2, u8 color);

void ega_wm2_rectangle(u8 __far *vram, s16 x1, s16 y1, s16 x2, s16 y2, u8 color);

void ega_wm2_line(s16 x0, s16 y0, s16 x1, s16 y1, u8 color);
void ega_wm2_line_thick3(s16 x0, s16 y0, s16 x1, s16 y1, u8 color);

void ega_wm2_line_thick3_fastest(s16 x0, s16 y0, s16 x1, s16 y1, u8 color);


void text_mode_restore(void);
void ega_clear_screen_fast(u8 color);

// VRAM->VRAM копирование по строкам в WM1 (с memmove-логикой внутри строки)
void ega_vram_move_blocks(u16 srcOffset,
                          u16 destOffset,
                          u16 maxRowIndex,
                          u16 countBytesPerLine);

 
static void ega_decode_off_640(u16 off, u16 *x_px, u16 *y, u16 *x_byte);

s16 ega_copy_screen_to_offscreen_fast(u16 dstOff);
s16 ega_restore_screen_from_offscreen_fast(u16 srcOff);

void ega64_to_rgb(u16 v, u8 *r, u8 *g, u8 *b);

 
 
void ega_latch_fill(u16 offset, u16 row_count_minus_1, u16 count_bytes, u8 color);

u16 ega_get_offset_from_xy(u16 x, u16 y);
u8 ega_toggle_startaddr_hi_00_7d(void);

#endif // EGA_DRAW_H
