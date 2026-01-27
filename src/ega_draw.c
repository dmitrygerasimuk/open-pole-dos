// ega_draw.c
// Рисование/копирование для EGA 640x350 
 

#include <dos.h>     
#include <i86.h>      
#include <memory.h>   

#include "typedefs.h"
#include "ega_draw.h"
#include "debuglog.h"

#include <conio.h>   

// Заполняет dst[0..count-1] значением color (WM2, BitMask=FF уже выставлен).
static inline void ega_wm2_memset_row(u8 __far *dst, u16 count, u8 color);
#pragma aux ega_wm2_memset_row = \
    "mov ah, al"      /* AX = color|color<<8 */ \
    "shr cx, 1"       /* CX = count/2, CF = count&1 */ \
    "rep stosw"       \
    "jnc short done"  \
    "stosb"           \
    "done:"           \
    parm [es di] [cx] [al] \
    modify [ax cx di];

u16 ega_bios_vram_kb(void)
{
    union REGS r;

    r.h.ah = 0x12;
    r.h.bl = 0x10;
    int86(0x10, &r, &r);

    // r.h.bl: 00=64K, 01=128K, 02=192K, 03=256K
    switch (r.h.bl) {
    case 0: return  64u;
    case 1: return 128u;
    case 2: return 192u;
    case 3: return 256u;
    default: return 0u; // неизвестно / BIOS не поддерживает
    }
}

u16 is_vga(void)
{
    union REGS r;

    r.x.ax = 0x1A00;
    int86(0x10, &r, &r);

    // AL == 0x1A означает, что функция присутствует (VGA BIOS)
    return (r.h.al == 0x1Au) ? 1u : 0u;
}

u16 ega_off_640(s16 x, s16 y)
{
    // 80 байт на строку, x в пикселях, адресуется по байтам
    return (u16)((u16)y * 80u + ((u16)x >> 3));
}

void ega_draw_grid_10px(void)
{
    s16 x, y;

    // Настройка EGA для MACRO_EGA_PUTPIXEL_FAST
    MACRO_EGA_PUTPIXEL_FAST_BEGIN();

    // Вертикальные линии каждые 10 пикселей
    for (x = 10; x < 640; x = (s16)(x + 10)) {
        for (y = 0; y < 350; ++y) {
            MACRO_EGA_PUTPIXEL_FAST(x, y, 0x02);
        }
    }

    // Горизонтальные линии каждые 10 пикселей, точками (шаг по X = 2)
    for (y = 0; y < 350; y = (s16)(y + 10)) {
        for (x = 0; x < 640; x = (s16)(x + 2)) {
            MACRO_EGA_PUTPIXEL_FAST(x, y, 0x02);
        }
    }
}

void ega_wm2_putpixel(u8 __far *vram, s16 x, s16 y, u8 color)
{
    u16 off;
    u8  mask;
    volatile u8 dummy;

    if ((u16)x >= 640u || (u16)y >= 350u) return;

    // 80 байт/строка, бит = x&7
    off  = (u16)((u16)y * 80u + ((u16)x >> 3));
    mask = (u8)(0x80u >> ((u16)x & 7u));

    // GC BitMask data port = 0x3CF (индекс 8 должен быть уже выбран снаружи)
    out_dx_al(0x3CF, mask);

    // latch-read обязателен для WM2
    dummy = vram[off];
    (void)dummy;

    vram[off] = color;
}

void ega_wm2_hline_fast(u8 __far *vram, s16 x1, s16 x2, s16 y, u8 color)
{
    u16 off, b1, b2, bit1, bit2;
    u8  m;
    volatile u8 dummy;

    if (x1 > x2) { s16 t = x1; x1 = x2; x2 = t; }
    if ((u16)y >= 350u) return;
    if (x2 < 0 || x1 >= 640) return;
    if (x1 < 0) x1 = 0;
    if (x2 > 639) x2 = 639;

    // Выставляем WM2 и выбираем BitMask
    out_dx_al(0x3CE, 0x05); out_dx_al(0x3CF, 0x02);
    out_dx_al(0x3CE, 0x08);

    b1   = (u16)((u16)x1 >> 3);
    b2   = (u16)((u16)x2 >> 3);
    bit1 = (u16)((u16)x1 & 7u);
    bit2 = (u16)((u16)x2 & 7u);

    off = (u16)((u16)y * 80u + b1);

    if (b1 == b2) {
        m = (u8)((0xFFu >> bit1) & (0xFFu << (7u - bit2)));
        out_dx_al(0x3CF, m);
        dummy = vram[off]; (void)dummy;
        vram[off] = color;
        return;
    }

    // Первый байт (частичный)
    m = (u8)(0xFFu >> bit1);
    out_dx_al(0x3CF, m);
    dummy = vram[off]; (void)dummy;
    vram[off++] = color;

    // Полные байты
    if (b2 > (u16)(b1 + 1u)) {
        u16 n = (u16)(b2 - b1 - 1u);
        out_dx_al(0x3CF, 0xFF);
        while (n--) {
            dummy = vram[off]; (void)dummy;
            vram[off++] = color;
        }
    }

    // Последний байт (частичный)
    m = (u8)(0xFFu << (7u - bit2));
    out_dx_al(0x3CF, m);
    dummy = vram[off]; (void)dummy;
    vram[off] = color;
}

void ega_wm2_bar(u8 __far *vram, s16 x1, s16 y1, s16 x2, s16 y2, u8 color)
{
    s16 y;

    if (x1 > x2) { s16 t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { s16 t = y1; y1 = y2; y2 = t; }
    if (y2 < 0 || y1 >= 350) return;
    if (y1 < 0) y1 = 0;
    if (y2 > 349) y2 = 349;

    for (y = y1; y <= y2; ++y) {
        ega_wm2_hline_fast(vram, x1, x2, y, color);
    }

    DBG("ega_wm2_bar: rect x=%d y=%d w=%d h=%d base_off=%04X\n",
        (int)x1, (int)y1, (int)(x2 - x1), (int)(y2 - y1),
        ((u16)FP_OFF(vram) >= 0x7D00u) ? 0x7D00u : 0x0000u);

    SCRN();
}


void ega_wm2_bar_u(u8 __far *vram, u16 x1, u16 y1, u16 x2, u16 y2, u8 color)
{
    u16 y;

    if (x1 > x2) { u16 t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { u16 t = y1; y1 = y2; y2 = t; }

    for (y = y1; y <= y2; ++y) {
        ega_wm2_hline_fast(vram, x1, x2, y, color);
    }

    DBG("ega_wm2_bar_u: rect x=%u y=%u w=%u h=%u base_off=%04X\n",
        (u16)x1, (u16)y1,
        (u16)(x2 - x1), (u16)(y2 - y1),
        ((u16)FP_OFF(vram) >= 0x7D00u) ? 0x7D00u : 0x0000u);

    SCRN();
}


// Bresenham (диагональные линии)
void ega_wm2_line(s16 x0, s16 y0, s16 x1, s16 y1, u8 color)
{
    s16 dx = (x1 > x0) ? (s16)(x1 - x0) : (s16)(x0 - x1);
    s16 sx = (x0 < x1) ? 1 : -1;
    s16 dy = (y1 > y0) ? (s16)(y1 - y0) : (s16)(y0 - y1);
    s16 sy = (y0 < y1) ? 1 : -1;
    s16 err = (s16)(((dx > dy) ? dx : (s16)-dy) / 2);
    s16 e2;

    MACRO_EGA_PUTPIXEL_FAST_BEGIN();

    for (;;) {
        MACRO_EGA_PUTPIXEL_FAST(x0, y0, color);

        if (x0 == x1 && y0 == y1) break;

        e2 = err;
        if (e2 > (s16)-dx) { err = (s16)(err - dy); x0 = (s16)(x0 + sx); }
        if (e2 <  dy)      { err = (s16)(err + dx); y0 = (s16)(y0 + sy); }
    }
}

void text_mode_restore(void)
{
    union REGS r;

    r.h.ah = 0x00;
    r.h.al = 0x03;
    int86(0x10, &r, &r);
}

void ega_wm2_line_thick3(s16 x0, s16 y0, s16 x1, s16 y1, u8 color)
{
    s16 dx = (x1 > x0) ? (s16)(x1 - x0) : (s16)(x0 - x1);
    s16 sx = (x0 < x1) ? 1 : -1;
    s16 dy = (y1 > y0) ? (s16)(y1 - y0) : (s16)(y0 - y1);
    s16 sy = (y0 < y1) ? 1 : -1;
    s16 err = (s16)(((dx > dy) ? dx : (s16)-dy) / 2);
    s16 e2;

    // Если линия более горизонтальная, утолщаем по Y, иначе по X
    u16 thick_along_y = (dx >= dy) ? 1u : 0u;

    MACRO_EGA_PUTPIXEL_FAST_BEGIN();

    for (;;) {
        if (thick_along_y) {
            MACRO_EGA_PUTPIXEL_FAST(x0, (s16)(y0 - 1), color);
            MACRO_EGA_PUTPIXEL_FAST(x0, y0,             color);
            MACRO_EGA_PUTPIXEL_FAST(x0, (s16)(y0 + 1), color);
        } else {
            MACRO_EGA_PUTPIXEL_FAST((s16)(x0 - 1), y0, color);
            MACRO_EGA_PUTPIXEL_FAST(x0,             y0, color);
            MACRO_EGA_PUTPIXEL_FAST((s16)(x0 + 1), y0, color);
        }

        if (x0 == x1 && y0 == y1) break;

        e2 = err;
        if (e2 > (s16)-dx) { err = (s16)(err - dy); x0 = (s16)(x0 + sx); }
        if (e2 <  dy)      { err = (s16)(err + dx); y0 = (s16)(y0 + sy); }
    }
}
static inline u16 ega_off80(u16 y, u16 x)
{
    /* y*80 = y*64 + y*16 */
    return (u16)((y << 6) + (y << 4) + (x >> 3));
}

static const u8 ega_mask_tbl[8] = { 0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01 };

static inline u8 ega_mask(u16 x) { return ega_mask_tbl[x & 7u]; }

/* vram_base = MK_FP(A000,0) один раз */
#define EGA_WM2_POKE_OFF(vram_base_, off_, color_)                  \
do {                                                               \
    volatile unsigned char __far* __p = (vram_base_) + (off_);      \
    volatile unsigned char __dummy = *__p; (void)__dummy;          \
    *__p = (unsigned char)((color_) & 0x0Fu);                      \
} while (0)

void ega_wm2_line_thick3_fastest(s16 x0, s16 y0, s16 x1, s16 y1, u8 color)
{
    s16 dx = (x1 > x0) ? (s16)(x1 - x0) : (s16)(x0 - x1);
    s16 sx = (x0 < x1) ? 1 : -1;
    s16 dy = (y1 > y0) ? (s16)(y1 - y0) : (s16)(y0 - y1);
    s16 sy = (y0 < y1) ? 1 : -1;

    s16 err = (dx > dy) ? (s16)(dx >> 1) : (s16)(-(dy >> 1));
    s16 e2;

    u8 thick_along_y = (dx >= dy) ? 1u : 0u;

    volatile unsigned char __far* vram =
        (volatile unsigned char __far*)MK_FP(EGA_VIDEO_SEGMENT, 0);

    MACRO_EGA_PUTPIXEL_FAST_BEGIN();

    if (thick_along_y) {
        for (;;) {
            u16 xx = (u16)x0;
            u16 yy = (u16)y0;
            u16 off = (u16)(((yy << 6) + (yy << 4)) + (xx >> 3));
            u8  m   = ega_mask_tbl[xx & 7u];

            // один outp на три пикселя  
            out_dx_al(0x3CF, m);

            /* (y-1), y, (y+1) */
            if (yy > 0u)              EGA_WM2_POKE_OFF(vram, (u16)(off - 80u), color);
                                     EGA_WM2_POKE_OFF(vram, off,              color);
            if (yy < 349u)            EGA_WM2_POKE_OFF(vram, (u16)(off + 80u), color);

            if (x0 == x1 && y0 == y1) break;

            e2 = err;
            if (e2 > (s16)-dx) { err = (s16)(err - dy); x0 = (s16)(x0 + sx); }
            if (e2 <  dy)      { err = (s16)(err + dx); y0 = (s16)(y0 + sy); }
        }
    } else {
        for (;;) {
            u16 xx = (u16)x0;
            u16 yy = (u16)y0;
            u16 off = (u16)(((yy << 6) + (yy << 4)) + (xx >> 3));
            u8  b   = (u8)(xx & 7u);
            u8  m   = ega_mask_tbl[b];

            if (b >= 1u && b <= 6u) {
              // 3 пикселя в одном байте  
                u8 mm = (u8)((m << 1) | m | (m >> 1));
                out_dx_al(0x3CF, mm);
                EGA_WM2_POKE_OFF(vram, off, color);
            } else if (b == 0u) {
              // x-1 в prev byte, x и x+1 в current 
                out_dx_al(0x3CF, 0x01u);
                if (off > 0u) EGA_WM2_POKE_OFF(vram, (u16)(off - 1u), color);

                out_dx_al(0x3CF, 0xC0u);
                EGA_WM2_POKE_OFF(vram, off, color);
            } else { // b == 7  
               // x-1 и x в current, x+1 в next  
                out_dx_al(0x3CF, 0x03u);
                EGA_WM2_POKE_OFF(vram, off, color);

                out_dx_al(0x3CF, 0x80u);
                EGA_WM2_POKE_OFF(vram, (u16)(off + 1u), color);
            }

            if (x0 == x1 && y0 == y1) break;

            e2 = err;
            if (e2 > (s16)-dx) { err = (s16)(err - dy); x0 = (s16)(x0 + sx); }
            if (e2 <  dy)      { err = (s16)(err + dx); y0 = (s16)(y0 + sy); }
        }
    }
}

void ega_clear_screen_fast(u8 color)
{
    // WM2 + BitMask=FF, после этого можно _fmemset по VRAM
    out_dx_al(0x3CE, 0x05); out_dx_al(0x3CF, 0x02);
    out_dx_al(0x3CE, 0x08); out_dx_al(0x3CF, 0xFF);

    _fmemset(PTR_VRAM_A000, color, (u16)EGA_SCREEN_BYTES);

    out_dx_al(0x3CE, 0x08);
}

// Декодирует A000:off в координаты  
static void ega_decode_off_640(u16 off, u16 *x_px, u16 *y, u16 *x_byte)
{
    u16 xb, yy;

    xb = (u16)(off % (u16)EGA_BPL);
    yy = (u16)(off / (u16)EGA_BPL);

    if (x_byte) *x_byte = xb;
    if (y)      *y      = yy;
    if (x_px)   *x_px   = (u16)(xb * 8u);
}

// Копирование в WM1: чтение байта загружает летчи, запись игнорирует байт и пишет летч 
static void wm1_movsb_fwd(u16 srcOff, u16 dstOff, u16 count);
#pragma aux  wm1_movsb_fwd = \
    "push ds"                \
    "push es"                \
    "mov  ax, 0A000h"        \
    "mov  ds, ax"            \
    "mov  es, ax"            \
    "cld"                    \
    "jcxz short fwd_done"    \
"fwd_loop:"                 \
    "lodsb"                  \
    "mov  byte ptr es:[di], 0" \
    "inc  di"                \
    "loop fwd_loop"          \
"fwd_done:"                 \
    "pop  es"                \
    "pop  ds"                \
    parm [si] [di] [cx]      \
    modify [ax si di cx];

static void wm1_movsb_bwd(u16 srcOffLast, u16 dstOffLast, u16 count);
#pragma aux wm1_movsb_bwd = \
    "push ds"                \
    "push es"                \
    "mov  ax, 0A000h"        \
    "mov  ds, ax"            \
    "mov  es, ax"            \
    "std"                    \
    "jcxz short bwd_done"    \
"bwd_loop:"                 \
    "lodsb"                  \
    "mov  byte ptr es:[di], 0" \
    "dec  di"                \
    "loop bwd_loop"          \
"bwd_done:"                 \
    "cld"                    \
    "pop  es"                \
    "pop  ds"                \
    parm [si] [di] [cx]      \
    modify [ax si di cx];

// Аналог Pascal Move для VRAM (A000) по строкам, с memmove-логикой внутри строки.
void ega_vram_move_blocks(u16 srcOffset,
                          u16 destOffset,
                          u16 maxRowIndex,
                          u16 countBytesPerLine)
{
    u16 row;
    u16 n = countBytesPerLine;

    if (n == 0u) return;

#ifdef DBGON
    {
        u16 sx, sy, sb, dx, dy, db;
        u16 wpx = (u16)(countBytesPerLine * 8u);
        u16 h   = (u16)(maxRowIndex + 1u);

        ega_decode_off_640(srcOffset,  &sx, &sy, &sb);
        ega_decode_off_640(destOffset, &dx, &dy, &db);

        DBGL("VRAMMOVE: src x=%u y=%u w=%u h=%u off=%04X xb=%u  dst x=%u y=%u off=%04X xb=%u\n",
             sx, sy, wpx, h, srcOffset, sb, dx, dy, destOffset, db);
    }
#endif

    // Настраиваем EGA на WM1 копирование
    out_dx_al(0x3CE, 0x05); out_dx_al(0x3CF, 0x01);
    out_dx_al(0x3CE, 0x08); out_dx_al(0x3CF, 0xFF);
    out_dx_al(0x3C4, 0x02); out_dx_al(0x3C5, 0x0F);

    for (row = 0u; row <= maxRowIndex; ++row) {
        u16 s = (u16)(srcOffset  + (u16)(row * (u16)EGA_BPL));
        u16 d = (u16)(destOffset + (u16)(row * (u16)EGA_BPL));

        if (d == s) continue;

        // Проверка перекрытия  
        {
            u32 su = (u32)s;
            u32 du = (u32)d;
            u32 nu = (u32)n;

            if (du < su || du >= (su + nu)) {
                wm1_movsb_fwd(s, d, n);
            } else {
                wm1_movsb_bwd((u16)(s + n - 1u), (u16)(d + n - 1u), n);
            }
        }
    }

    // Возвращаем WM2 для обычных рисовалок
    out_dx_al(0x3CE, 0x05); out_dx_al(0x3CF, 0x02);
    out_dx_al(0x3CE, 0x08);
}

// Быстрая VRAM->VRAM копия rep movsb  
void vram_rep_movsb_fwd(u16 srcOff, u16 dstOff, u16 bytes);
#pragma aux vram_rep_movsb_fwd = \
    "push ds"        \
    "push es"        \
    "mov ax,0A000h"  \
    "mov ds,ax"      \
    "mov es,ax"      \
    "cld"            \
    "rep movsb"      \
    "pop es"         \
    "pop ds"         \
    parm [si] [di] [cx] \
    modify [ax];

static void ega_prepare_copy_wm1(u8 *s_seq2,
                                 u8 *s_gc0,
                                 u8 *s_gc1,
                                 u8 *s_gc3,
                                 u8 *s_gc4,
                                 u8 *s_gc5,
                                 u8 *s_gc8)
{
    if (s_seq2) *s_seq2 = ega_seq_read(2);
    if (s_gc0)  *s_gc0  = ega_gc_read(0);
    if (s_gc1)  *s_gc1  = ega_gc_read(1);
    if (s_gc3)  *s_gc3  = ega_gc_read(3);
    if (s_gc4)  *s_gc4  = ega_gc_read(4);
    if (s_gc5)  *s_gc5  = ega_gc_read(5);
    if (s_gc8)  *s_gc8  = ega_gc_read(8);

    // Все 4 плоскости
    ega_seq_write(2, 0x0F);

    // Очищаем Set/Reset и вращения данных
    ega_gc_write(0, 0x00);
    ega_gc_write(1, 0x00);
    ega_gc_write(3, 0x00);
    ega_gc_write(8, 0xFF);

    // Read Mode 0 + Write Mode 1
    ega_gc_write(5, 0x01);

    // Read Map Select = 0
    ega_gc_write(4, 0x00);
}

static void ega_restore_copy(u8 s_seq2,
                             u8 s_gc0,
                             u8 s_gc1,
                             u8 s_gc3,
                             u8 s_gc4,
                             u8 s_gc5,
                             u8 s_gc8)
{
    ega_seq_write(2, s_seq2);
    ega_gc_write(0, s_gc0);
    ega_gc_write(1, s_gc1);
    ega_gc_write(3, s_gc3);
    ega_gc_write(4, s_gc4);
    ega_gc_write(5, s_gc5);
    ega_gc_write(8, s_gc8);
}

// screen(A000:0000) -> offscreen(A000:dstOff)
s16 ega_copy_screen_to_offscreen_fast(u16 dstOff)
{
    u8 s_seq2, s_gc0, s_gc1, s_gc3, s_gc4, s_gc5, s_gc8;

    if (dstOff > (u16)(0x10000u - (u16)EGA_SCREEN_BYTES)) return 0;

    ega_prepare_copy_wm1(&s_seq2, &s_gc0, &s_gc1, &s_gc3, &s_gc4, &s_gc5, &s_gc8);
    vram_rep_movsb_fwd(0x0000u, dstOff, (u16)EGA_SCREEN_BYTES);
    ega_restore_copy(s_seq2, s_gc0, s_gc1, s_gc3, s_gc4, s_gc5, s_gc8);
    return 1;
}

// offscreen(A000:srcOff) -> screen(A000:0000)
s16 ega_restore_screen_from_offscreen_fast(u16 srcOff)
{
    u8 s_seq2, s_gc0, s_gc1, s_gc3, s_gc4, s_gc5, s_gc8;

    if (srcOff > (u16)(0x10000u - (u16)EGA_SCREEN_BYTES)) return 0;

    ega_prepare_copy_wm1(&s_seq2, &s_gc0, &s_gc1, &s_gc3, &s_gc4, &s_gc5, &s_gc8);
    vram_rep_movsb_fwd(srcOff, 0x0000u, (u16)EGA_SCREEN_BYTES);
    ega_restore_copy(s_seq2, s_gc0, s_gc1, s_gc3, s_gc4, s_gc5, s_gc8);
    return 1;
}

void ega_latch_fill(u16 offset, u16 row_count_minus_1, u16 count_bytes, u8 color)
{
    
    

    volatile u8 __far *vram  = (volatile u8 __far *)MK_FP(0xA000, 0);
    volatile u8 __far *p     = vram + offset;
    volatile u8 dummy;
    u16 row;

    // Расчёт прямоугольника полезен только для диагностики переполнений по строке
    {
        u16 y0     = (u16)(offset / EGA_BPL);
        u16 x_byte = (u16)(offset % EGA_BPL);
        u16 x0     = (u16)(x_byte * 8u);

        u16 h   = (u16)(row_count_minus_1 + 1u);
        u16 wpx = (u16)(count_bytes * 8u);

        u16 left = (u16)(EGA_BPL - x_byte);
        u16 clipped_bytes = (count_bytes <= left) ? count_bytes : left;
        u16 wpx_clip = (u16)(clipped_bytes * 8u);

        u16 x1 = (wpx_clip > 0u) ? (u16)(x0 + wpx_clip - 1u) : x0;
        u16 y1 = (h > 0u) ? (u16)(y0 + h - 1u) : y0;

        DBG("ega_latch_fill: off=%04X color=%u bytes=%u rows=%u rect x0=%u y0=%u x1=%u y1=%u\n",
            offset, (u16)color, count_bytes, (u16)(row_count_minus_1 + 1u),
            x0, y0, x1, y1);

        if (count_bytes > left) {
            DBG("ega_latch_fill: WARN bytes cross scanline: bytes=%u x_byte=%u left=%u\n",
                count_bytes, x_byte, left);
        }
    }

    // WM2 + BitMask=FF, пишем один байт чтобы запечь паттерн в latch
    out_dx_al(0x3CE, 0x05); out_dx_al(0x3CF, 0x02);
    out_dx_al(0x3CE, 0x08); out_dx_al(0x3CF, 0xFF);

    dummy = *p;
    *p = (u8)color;

    // WM1: чтение загрузит latch, запись пишет latch по BitMask
    out_dx_al(0x3CE, 0x05); out_dx_al(0x3CF, 0x01);

    dummy = *p;
    *p = 0;

    for (row = 0u; row <= row_count_minus_1; ++row) {
        volatile u8 __far *rowp = vram + (u16)(offset + (u16)(row * EGA_BPL));
        unsigned i;
        for (i = 0u; i < count_bytes; ++i) {
            rowp[i] = 0;
        }
    }

    (void)dummy;

    // Возврат к режиму, ожидаемому остальными рисовалками
    ega_seq_write(2, 0x0F);
    ega_gc_write(1, 0x00);
    ega_gc_write(8, 0xFF);
    ega_gc_write(5, 0x02);

   
}

 

void ega_wm2_vline_fast(u8 __far *vram, s16 x, s16 y1, s16 y2, u8 color)
{
    u16 off;
    u8  mask;
    volatile u8 dummy;

    if (y1 > y2) { s16 t = y1; y1 = y2; y2 = t; }
    if ((u16)x >= 640u) return;
    if (y2 < 0 || y1 >= 350) return;
    if (y1 < 0) y1 = 0;
    if (y2 > 349) y2 = 349;

    mask = (u8)(0x80u >> ((u16)x & 7u));
    off  = (u16)((u16)y1 * 80u + ((u16)x >> 3));

    // WM2 + BitMask
    out_dx_al(0x3CE, 0x05); out_dx_al(0x3CF, 0x02);
    out_dx_al(0x3CE, 0x08); out_dx_al(0x3CF, mask);

    for (; y1 <= y2; ++y1) {
        dummy = vram[off]; (void)dummy;
        vram[off] = color;
        off = (u16)(off + 80u);
    }

    out_dx_al(0x3CE, 0x08);
}

void ega_wm2_rectangle(u8 __far *vram, s16 x1, s16 y1, s16 x2, s16 y2, u8 color)
{
    if (x1 > x2) { s16 t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { s16 t = y1; y1 = y2; y2 = t; }

    if (x2 < 0 || x1 >= 640) return;
    if (y2 < 0 || y1 >= 350) return;
    if (x1 < 0) x1 = 0;
    if (x2 > 639) x2 = 639;
    if (y1 < 0) y1 = 0;
    if (y2 > 349) y2 = 349;

    ega_wm2_hline_fast(vram, x1, x2, y1, color);
    if (y2 != y1) ega_wm2_hline_fast(vram, x1, x2, y2, color);

    ega_wm2_vline_fast(vram, x1, y1, y2, color);
    if (x2 != x1) ega_wm2_vline_fast(vram, x2, y1, y2, color);
}

u16 ega_get_offset_from_xy(u16 x, u16 y)
{
    // ofs = y*80 + (x>>3)
    u32 t = (u32)y * (u32)EGA_BPL;
    t += (u32)(x >> 3);
    return (u16)t;
}



static u8 crtc_read(u8 reg)
{
    out_dx_al(CRTC_IDX, reg);
    return (u8)in_dx_al(CRTC_DAT);
}

static void crtc_write(u8 reg, u8 v)
{
    out_dx_al(CRTC_IDX, reg);
    out_dx_al(CRTC_DAT, v);
}

// Переключает старший байт start address: 0x00 <-> 0x7D.
// Возвращает предыдущее значение регистра 0x0C.
u8 ega_toggle_startaddr_hi_00_7d(void)
{
    u8 saved_idx;
    u8 hi;
    u8 next;

    _disable();
    saved_idx = (u8)in_dx_al(CRTC_IDX);

    hi = crtc_read(0x0Cu);
    next = (hi == 0x7Du) ? 0x00u : 0x7Du;
    crtc_write(0x0Cu, next);

    out_dx_al(CRTC_IDX, saved_idx);
    _enable();

    return hi;
}

 
