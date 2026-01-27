// sale_info.c
#include "sale_info.h"

#include <dos.h>
#include <string.h>

#include "debuglog.h"
#include "globals.h"
#include "resources.h"
#include "ega_text.h"
#include "ega_draw.h"
#include "rle_draw.h"
#include "res_gfx.h"
#include "input.h"
#include "sound.h"
#include "sys_delay.h"
#include "pole_dialog.h"
#include "yakubovich.h"

 
 
 

#define CURTAIN_V_START        ((s16)80)     // 0x50

// sound
#define CURTAIN_TONE_BASE      ((s16)1700)   // 0x06A4
#define CURTAIN_TONE_STEP      ((s16)20)     // 0x14

 
#define CURTAIN_COPY_WBYTES    ((u16)21u)    // 0x15

// VRAM offsets 
#define CURTAIN_DST_BASE_OFF   ((u16)0x364Bu)
#define CURTAIN_SRC_OFF        ((u16)0xB34Bu)

 
#define CURTAIN_DST_STRIDE     ((u16)160u)   // 0x00A0

 
#define CURTAIN_MAX_ROWS(v_)   ((u16)(160u - (u16)((u16)(v_) << 1)))

 
#define CURTAIN_DST_OFF(v_)    ((u16)(CURTAIN_DST_BASE_OFF + (u16)((u16)(v_) * CURTAIN_DST_STRIDE)))

 
#define CURTAIN_DELAY_MS(v_)   ((u16)((v_) / 2u))

// x = 557 - (len * 4)  (0x22D - (len<<2))
static u16 sale_center_x(const char *s)
{
    const u16 area   = 557u;
    const u16 char_w = 4u;

    u16 len = s ? (u16)strlen(s) : 0u;
    u16 w   = (u16)(len * char_w);

    return (w > area) ? 0u : (u16)(area - w);
}

 
static void sale_print_line(u16 y, const char *s)
{
    u16 x = sale_center_x(s);

    // draw text in offscreen; color is yellow
    ega_draw_text_8xN_vramoff(
        gRes.fonts.f8,
        8,
        x,
        y,
        EGA_COLOR_YELLOW,
        8,
        s ? s : "",
        EGA_OFFSCREEN_BASE
    );
}

// SALE_INFO_PRINT draws (in offscreen) and then reveals by "curtain":
//   S_COMP_GAME   = "Компьютерная игра"
//   S_SOLD_AT     = "продается по адресу"
//   S_ADDR1       = "101000-Ц, МОСКВА,"
//   S_ADDR2       = "проезд Серова, 11."
//   S_FIRST25     = "25 самых первых"
//   S_BUYERS      = "покупателей будут"
//   S_INVITED     = "приглашены со"
//   S_FAMILIES    = "своими семьями"
//   S_SHOOTING    = "на съемки телеигры"
//   S_LAST_LINE   = "ПОЛЕ ЧУДЕС!"

void sale_info_print(void)
{
    u16 y;

    DBG("sale_info_print: begin\n");

    //  
    // бэкапим задник
    ega_vram_move_blocks(0x35FBu, 0xB2FBu, (u16)(0xAAu - 1u), CURTAIN_COPY_WBYTES);

    // панель синяя 
    draw_rle_packed_sprite(
        EGA_COLOR_NO_TRANSPARENT,
        173u,
        481u,
        PTR_VRAM_7D00_OFFSCREEN,
        GET_GFX_PTR(RES_GFX_BLUE_PANEL_REKLAMA)
    );

    //   (from disasm): base y=164,   +10,   +74,   +8    , +10  
    y = 164u;

    y = (u16)(y + 10u);
    sale_print_line(y, S_COMP_GAME);

    y = (u16)(y + 74u);
    sale_print_line(y, S_SOLD_AT);

    y = (u16)(y + 8u);  sale_print_line(y, S_ADDR1);
    y = (u16)(y + 8u);  sale_print_line(y, S_ADDR2);
    y = (u16)(y + 8u);  sale_print_line(y, S_FIRST25);
    y = (u16)(y + 8u);  sale_print_line(y, S_BUYERS);
    y = (u16)(y + 8u);  sale_print_line(y, S_INVITED);
    y = (u16)(y + 8u);  sale_print_line(y, S_FAMILIES);
    y = (u16)(y + 8u);  sale_print_line(y, S_SHOOTING);

    y = (u16)(y + 10u);
    sale_print_line(y, S_LAST_LINE);

    // выезжаюзая шторка
   
    {
        s16 v = CURTAIN_V_START;

        for (;;) {   // цикл с копирование блоков - анимация
            s16 tone;

           
            tone = (s16)(CURTAIN_TONE_BASE - (s16)(v * CURTAIN_TONE_STEP));
            tone = (s16)(tone * (s16)g_soundONOF_soundMultiplier);
            tp_sound((u16)tone);

          
            ega_vram_move_blocks(
                CURTAIN_SRC_OFF,
                CURTAIN_DST_OFF(v),
                CURTAIN_MAX_ROWS(v),
                CURTAIN_COPY_WBYTES
            );

            tp_nosound();
            pit_delay_ms(CURTAIN_DELAY_MS(v));

            if (v == 0) break;
            --v;
        }
    }

    loop_wait_and_get_keys(0u);

    // вернем якубовича на место
    draw_rle_packed_sprite(
        RES_GFX_TRANSPARENT_GRAY,
        YAK_FULL_Y,
        YAK_FULL_X,
        PTR_VRAM_A000,
        GET_GFX_PTR(RES_GFX_YAKUBOVICH_152x152)
    );

    draw_rle_packed_sprite(
        EGA_COLOR_NO_TRANSPARENT,
        YAK_FACE_Y,
        YAK_FACE_X,
        PTR_VRAM_A000,
        GET_GFX_PTR(RES_YAKUBOVICH_FACE_MOUTH_CLOSE)
    );

    draw_rle_packed_sprite(
        EGA_COLOR_NO_TRANSPARENT,
        YAK_EYES_Y_OPEN,
        YAK_EYES_X,
        PTR_VRAM_A000,
        GET_GFX_PTR(RES_YAKUBOVICH_ONLY_EYES_OPEN)
    );

    DBG("sale_info_print: end\n");
}
