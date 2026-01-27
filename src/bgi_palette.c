// bgi_palette.c 

// в оригинале палитрой занимается EGAVGA.BGI но статически
// его анализировать это боль, он кладет указатели на свои функции
// в рантайме, так что я просто сделал свой аналог, не трогал порты, 
// чтение палитры на реальных EGA  не очень хорошо работает (тестил)
// todo: можно глянуть как egavga.bgi это делает 
#include "bgi_palette.h"

#include <dos.h>
#include <i86.h>

#include "typedefs.h"
#include "ega_draw.h"   
#include "debuglog.h"
#include "globals.h"     

// Pack 2-bit per channel RGB (0..3) into EGA 6-bit value 
// Bit layout matches EGA/VGA BIOS palette regs usage (BH=0..63).
u8 ega_pack_rgb2(u8 r0_3, u8 g0_3, u8 b0_3)
{
    u8 r = (u8)(r0_3 & 3u);
    u8 g = (u8)(g0_3 & 3u);
    u8 b = (u8)(b0_3 & 3u);

    u8 v = 0u;

    // c0 bits
    v |= (u8)((b & 1u) << 0); // b0
    v |= (u8)((g & 1u) << 1); // g0
    v |= (u8)((r & 1u) << 2); // r0

    // c1 bits
    v |= (u8)(((b >> 1) & 1u) << 3); // b1
    v |= (u8)(((g >> 1) & 1u) << 4); // g1
    v |= (u8)(((r >> 1) & 1u) << 5); // r1

    return v; // 0..63
}

//   int 10h, AX=1000h: set palette register
//   BL = logical color  0..15 
//   BH = 6-bit value  0..63 
static void bios_set_pal_reg(u8 logical0_15, u8 val0_63)
{
    union REGS r;
    r.x.ax = 0x1000;
    r.h.bl = (u8)(logical0_15 & 0x0Fu);
    r.h.bh = (u8)(val0_63 & 0x3Fu);
    int86(0x10, &r, &r);
}

// int 10h, AX=1001h: set overscan/border color
//   BH = 6-bit value 0..63 
static void bios_set_overscan(u8 val0_63)
{
    union REGS r;
    r.x.ax = 0x1001;
    r.h.bh = (u8)(val0_63 & 0x3Fu);
    int86(0x10, &r, &r);
}

 
// These are 6-bit values (BH), not 0..15  
static const u8 g_pal_default_16[16] = {
    (u8)EGA_COLOR_BLACK_CODE,         // 0
    (u8)EGA_COLOR_BLUE_CODE,          // 1
    (u8)EGA_COLOR_GREEN_CODE,         // 2
    (u8)EGA_COLOR_CYAN_CODE,          // 3
    (u8)EGA_COLOR_RED_CODE,           // 4
    (u8)EGA_COLOR_MAGENTA_CODE,       // 5
    (u8)EGA_COLOR_BROWN_CODE,         // 6
    (u8)EGA_COLOR_LIGHT_GRAY_CODE,    // 7
    (u8)EGA_COLOR_DARK_GRAY_CODE,     // 8
    (u8)EGA_COLOR_LIGHT_BLUE_CODE,    // 9
    (u8)EGA_COLOR_LIGHT_GREEN_CODE,   // 10
    (u8)EGA_COLOR_LIGHT_CYAN_CODE,    // 11
    (u8)EGA_COLOR_LIGHT_RED_CODE,     // 12
    (u8)EGA_COLOR_LIGHT_MAGENTA_CODE, // 13
    (u8)EGA_COLOR_YELLOW_CODE,        // 14
    (u8)EGA_COLOR_WHITE_CODE          // 15
};

#define DEFAULT_OVERSCAN_0_63      (0u)

 // ремапы палитры от автора 
#define NEW_COLOR_FOR_BROWN_6      (0x2Eu)
#define NEW_COLOR_FOR_RED_4        (0x14u)
#define NEW_COLOR_FOR_MAGENTA_5    (0x27u)

void bgi_palette_apply_default(void)
{
    unsigned i;

    for (i = 0u; i < 16u; ++i) {
        bios_set_pal_reg((u8)i, g_pal_default_16[i]);
    }

    bios_set_overscan((u8)DEFAULT_OVERSCAN_0_63);

    DBG("PAL: apply_default\n");
}
 
void bgi_palette_apply_startup(void)
{
    if (g_addon_mono_palette) {
        bgi_palette_set_pal_for_mono_screen();
        DBG("PAL: apply_startup (mono)\n");
        return;
    }

    bgi_palette_apply_default();

    //  ремапим
    bios_set_pal_reg((u8)EGA_COLOR_BROWN,   (u8)NEW_COLOR_FOR_BROWN_6);
    bios_set_pal_reg((u8)EGA_COLOR_RED,     (u8)NEW_COLOR_FOR_RED_4);
    bios_set_pal_reg((u8)EGA_COLOR_MAGENTA, (u8)NEW_COLOR_FOR_MAGENTA_5);

    DBG("PAL: apply_startup (default + remaps)\n");
}

 
void bgi_palette_set_slot(u16 logical_color, u16 val0_63)
{
    if (logical_color > 15u) return;

    bios_set_pal_reg((u8)logical_color, (u8)(val0_63 & 0x3Fu));
    DBG("PAL: slot %u <- %02X\n",
        (u16)logical_color,
        (u16)(val0_63 & 0x3Fu));
}

// overscan/border: 0..63
void bgi_palette_set_overscan(u16 val0_63)
{
    bios_set_overscan((u8)(val0_63 & 0x3Fu));
    DBG("PAL: overscan <- %02X\n", (u16)(val0_63 & 0x3Fu));
}

void bgi_palette_set_pal_for_mono_screen(void)
{
 // для монохромных дисплеев 
    static const u8 mono_pal[16] = {
        0x00u, 0x07u, 0x3Fu, 0x38u,
        0x00u, 0x3Fu, 0x3Fu, 0x38u,
        0x00u, 0x38u, 0x00u, 0x3Fu,
        0x07u, 0x3Fu, 0x3Fu, 0x3Fu
    };

    u8 i;
    for (i = 0u; i < 16u; ++i) {
        bios_set_pal_reg(i, mono_pal[i]);
    }
}
