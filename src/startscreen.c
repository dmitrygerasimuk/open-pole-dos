#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>

#include "resources.h"
#include "res_gfx.h"
#include "debuglog.h"
#include "ega_text.h"
#include "sys_delay.h"
#include "rle_draw.h"
#include "pole_dialog.h"
#include "ega_draw.h"
#include "input.h"
#include "sound.h"
#include "startscreen.h"
 
// startScreen port
 
static inline void draw_one_byte_char(Resources *R, u8 ch, u16 x, u16 y, u8 color)
{
    char s[2];
    s[0] = (char)ch;
    s[1] = '\0';
    ega_draw_text_8xN(R->fonts.f14, 14, x, y, color, 8, s);
}

static inline void startScreen_letter_beep(u16 i)
{
    u16 freq = (u16)(g_soundONOF_soundMultiplier * i * 100u);
    if (freq != 0u) tp_sound(freq);
}

void startscreen(Resources *R)
{
    int counter;
    int v;

    // Ghidra: SetFillStyle(7,1); SetColor(0); SetLineStyle(3,0,0);

    
    for (counter = 0; counter <= 160; ++counter) {
        // y = 345-counter; x: (20+counter) .. (620-counter)
        ega_wm2_bar(PTR_VRAM_A000,
            20 + counter,
            345 - counter,
            620 - counter,
            345 - counter,
            EGA_COLOR_LIGHT_GRAY
        );
        pit_delay_ms(10u);
    }

    loop_wait_and_get_keys(40u);

    //  рамка/лучи
    for (counter = 0; counter <= 160; ++counter) {
        // LINE(20,345) -> (180-counter, 185-counter)
        ega_wm2_line_thick3_fastest(
            20, 345,
            180 - counter, 185 - counter,
            EGA_COLOR_LIGHT_GRAY
        );

        // BAR at y=(185-counter) from x (180-counter) to  (460+counter)
        ega_wm2_bar(PTR_VRAM_A000,
            180 - counter,
            185 - counter,
            460 + counter,
            185 - counter,
            EGA_COLOR_LIGHT_GRAY
        );

        // LINE(460+counter,185-counter)  
        ega_wm2_line_thick3_fastest(
            460 + counter, 185 - counter,
            620, 345,
            EGA_COLOR_LIGHT_GRAY
        );

        pit_delay_ms(5u);
    }

    //  очищаем основной прямоугольник
    ega_wm2_bar(PTR_VRAM_A000, 20, 25, 620, 345, EGA_COLOR_LIGHT_GRAY);

    // сетка фаза 1 (вертикальные плашки)
    for (v = 1; v <= 60; ++v) {
        int x1 = 20 + (v - 1) * 10;
        int x2 = 20 +  v      * 10; // важно: без -1

        for (counter = 0; counter <= 8; ++counter) {
            int y1 = 25 + counter * 40;
            int y2 = 27 + counter * 40;
            ega_wm2_bar(PTR_VRAM_A000, x1, y1, x2, y2, EGA_COLOR_RED);
        }
        pit_delay_ms(10u);
    }

    // сетка фаза 2 (горизонтальные  плашки)
    for (v = 1; v <= 40; ++v) {
        int y1 = 25 + (v - 1) * 8;
        int y2 = 25 +  v      * 8;

        for (counter = 0; counter <= 12; ++counter) {
            int x1 = 19 + counter * 50;
            int x2 = 21 + counter * 50;
            ega_wm2_bar(PTR_VRAM_A000, x1, y1, x2, y2, EGA_COLOR_RED);
        }
        pit_delay_ms(10u);
    }

    loop_wait_and_get_keys(40u);

    //  sprites POLE/CHUDES
    draw_rle_packed_sprite(7u, 60u, 90u,  PTR_VRAM_A000, GET_GFX_PTR(RES_GFX_TITLE_POLE));
    draw_rle_packed_sprite(7u, 60u, 280u, PTR_VRAM_A000, GET_GFX_PTR(RES_GFX_TITLE_CHUDES));

    loop_wait_and_get_keys(40u);

    //  "КАПИТАЛ ШОУ" по буквам с обводкой + писк
    {
        unsigned i;
        u16 len = (u16)strlen(cKAPITALSHOW);

        for (i = 1u; i <= len; ++i) {
            int x_offset = (i < 9u) ? -15 : 0;
            int dx, dy;
            u8 ch = (u8)cKAPITALSHOW[i - 1u];

            int xbase = (int)i * 50 + x_offset;
            int ybase = 238;

            // обводка 
            for (dx = -1; dx <= 1; ++dx) {
                for (dy = -1; dy <= 2; ++dy) {
                    draw_one_byte_char(R, ch, (u16)(xbase + dx), (u16)(ybase + dy), EGA_COLOR_BLACK);
                }
            }

            startScreen_letter_beep(i);

            // основной белый 
            for (dy = 0; dy <= 1; ++dy) {
                draw_one_byte_char(R, ch, (u16)xbase, (u16)(ybase + dy), EGA_COLOR_WHITE);
            }

            tp_nosound();
            long_pit_delay_ms(250u);
        }

        tp_nosound();
    }

    // кредитсы
    {
        ega_draw_text_8xN(R->fonts.f8,  8,  68u,  2u,  7u, 8u, sdelalDimaBashurov);
        ega_draw_text_8xN(R->fonts.f8,  8, 176u, 12u,  7u, 8u, telephonString);

        // с обводкой
        ega_draw_text_8xN(R->fonts.f14, 14, 80u, 27u, 0u, 25u, posvDruzyam);
        ega_draw_text_8xN(R->fonts.f14, 14, 80u, 28u, 0u, 25u, posvDruzyam);

        // чёрный блок ( 
        
        ega_latch_fill(0x6040u, 0x46u, 0x50u, 0u );


        ega_draw_text_8xN(R->fonts.f8, 8,  27u, 310u, 7u, 8u, spravka1);
        ega_draw_text_8xN(R->fonts.f8, 8,  99u, 320u, 7u, 8u, spravka2);
        ega_draw_text_8xN(R->fonts.f8, 8,  99u, 330u, 7u, 8u, spravka3);
        ega_draw_text_8xN(R->fonts.f8, 8,  99u, 340u, 7u, 8u, spravka4);
    }

    loop_wait_and_get_keys(0u);
}
