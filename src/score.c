/* player.c  */
// рисуем очки игрока или сникерс

#include "score.h"

#include <dos.h>       
#include <stdlib.h>    
#include <string.h>  

#include "globals.h"
#include "ega_draw.h"    
#include "ega_text.h"    
#include "rle_draw.h"    
#include "res_gfx.h"
#include "resources.h"   
#include "sound.h"      
#include "cstr.h"
#include "random.h"


 
#define PLAYER_SCORE_GLYPH_H   14
#define PLAYER_SCORE_FONT      (gRes.fonts.f14)
#define PLAYER_TRANSP_RUBLES   1
#define PLAYER_TRANSP_SNICKERS 2

 
 
void player_draw_score(u16 slot)
{
    s16 v;
    u16 x, y;
    u16 vram_off;
    int x_byte;
    char num[16];
    u16 len;
    int base_x, base_y;

    if (slot > 3) return;

    x = g_score_x[slot];
    y = g_score_y[slot];

   
   
    x_byte = ((int)((s16)x - 4)) / 8;
    vram_off = (u16)(((u16)(y - 1) * 0x50) + (u16)x_byte);
    
     ega_latch_fill(vram_off,(u16)0x001D,(u16)0x000D,(u8)7);



    v = g_score_table[slot];

    // сыпем рублями
    if (v >= 1) {
        unsigned i;
        for (i = 1; i <= (u16)v; ++i) {
            u16 freq = (u16)(tp_random(10) + 50u);  
            if (g_soundONOF_soundMultiplier) {
                freq = (u16)(freq * g_soundONOF_soundMultiplier);
                if (freq) tp_sound(freq);
            } else {
                
            }

            

            draw_rle_packed_sprite(
                PLAYER_TRANSP_RUBLES,
                  (u16)((s16)y - 1 + (s16)tp_random(7)),
                (u16)((s16)x - 4 + (s16)tp_random(0x0C)),
              
                PTR_VRAM_A000,
                GET_GFX_PTR(RES_GFX_MONEY_RUBLES_SPRITE)
            );
            tp_nosound();
        }
    }

   
    if (v > 0) {   //  рисуем рубль + цифры с обводкой если есть деньги
       
        draw_rle_packed_sprite(
            PLAYER_TRANSP_RUBLES,
            (u16)(y),
             (u16)(x),
            PTR_VRAM_A000,
            GET_GFX_PTR(RES_GFX_MONEY_RUBLES_SPRITE)
        );

        i32_to_cstr(num, sizeof(num), (int)v);
        len = (u16)strlen(num);

         
       base_x = (int)x + 0x22 - 4 * (int)len;
       base_y = (int)y + 4;
        
        { // черная подложка и сверху белый текст
            int dx, dy;
            for (dx = -1; dx <= 1; ++dx) {
                for (dy = -1; dy <= 1; ++dy) {
                   
                    ega_draw_text_8xN(
                        PLAYER_SCORE_FONT,
                        (u16)PLAYER_SCORE_GLYPH_H,
                        (u16)(base_x + dx),
                        (u16)(base_y + dy),
                        EGA_COLOR_BLACK,
                        8,
                        num
                    );
                }
            }
        }

        
       
        ega_draw_text_8xN(
            PLAYER_SCORE_FONT,
            (u16)PLAYER_SCORE_GLYPH_H,
            (u16)base_x,
            (u16)base_y,
            EGA_COLOR_WHITE,
            8,
            num
        );
        return;
    }

    // или сникерс, если денег нет
 

    draw_rle_packed_sprite(
        PLAYER_TRANSP_SNICKERS,
         
         (u16)(y + 0x0A),
          (u16)(x + 4),
        PTR_VRAM_A000,
        GET_GFX_PTR(RES_GFX_MONEY_SNICKERS_SPRITE)
    );
}
