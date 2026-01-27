// prize.c - Prize UI

#include "prize.h"

#include <stdio.h>
#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <i86.h>

#include "globals.h"
#include "resources.h"
#include "res_gfx.h"
#include "ega_draw.h"
#include "ega_text.h"
#include "rle_draw.h"
#include "sys_delay.h"
#include "sound.h"
#include "score.h"
#include "yakubovich.h"
#include "boss_mode.h"
#include "input.h"
#include "pole_dialog.h"
#include "end_screen.h"
#include "debuglog.h"
#include "ui_choice.h"
#include "cstr.h"

static const char * const kPrizeItems[10] = {
    kPrizeItem0, kPrizeItem1, kPrizeItem2, kPrizeItem3, kPrizeItem4,
    kPrizeItem5, kPrizeItem6, kPrizeItem7, kPrizeItem8, kPrizeItem9
};
/*  const char kPrizeItem0[] = "ЗУБНУЮ ЩЕТКУ";
 const char kPrizeItem1[] = "Порошок ARIEL";
 const char kPrizeItem2[] = "Расчестку для усов";
 const char kPrizeItem3[] = "Зубочистку";
 const char kPrizeItem4[] = "Круг для унитаза";
 const char kPrizeItem5[] = "Рулон мягкой бумаги";
 const char kPrizeItem6[] = "Шнурки для калош";
 const char kPrizeItem7[] = "Подтяжки для носков";
 const char kPrizeItem8[] = "Беруши для ушей";
 const char kPrizeItem9[] = "Пивную открывашку";
 строки в Cp866 лежат в  pole-dialog.c */

 
#define UI_FILL_COLOR    EGA_COLOR_LIGHT_GRAY
 
#define UI_W_BYTES_50    0x0050u
 

// ------------------------------------------------------------------

void beruPRIZ(void)
{ 
    u16 prize_try = 0;   // приз или деньги
    u16 prize_or_money_choice = 1; // прииииз
    u32 money_ticks_left = 0;

   
    DBGL("latch from here\n");
    ega_latch_fill(0, 0x015E, UI_W_BYTES_50, UI_FILL_COLOR);

    // background
    ui_draw_rle(0x000A, 0x000A, 7, 0, RES_GFX_TITLE_POLE);
    ui_draw_rle(0x000A, 0x00C8, 7, 0, RES_GFX_TITLE_CHUDES);
    ui_draw_rle(0x00AC, 0x01E0, 0x10, 0, RES_GFX_YAKUBOVICH_152x152);

    // mouth/eyes: прозрачность по маске 0x10
    ui_draw_rle(0x00AD, 0x01FF, 0x10, 0, RES_YAKUBOVICH_FACE_MOUTH_CLOSE);
    ui_draw_rle(0x00D1, 0x0214, 0x10, 0, RES_YAKUBOVICH_ONLY_EYES_OPEN);

    // приз или деньги
    while (prize_try <= 3u && prize_or_money_choice != 0) {
        if (prize_try == 0) {
            write_to_g_buffer_str(kMoneySto);    
            money_ticks_left = 10ul;
        } else if (prize_try == 1u) {
            write_to_g_buffer_str(kMoneyTyshcha);
            money_ticks_left = 100ul;
        } else if (prize_try == 2u) {
            write_to_g_buffer_str(kMoneyStoTyshch);
            money_ticks_left = 10000ul;
        } else { // 3
            write_to_g_buffer_str(kMoneyMillion);
            money_ticks_left = 100000ul;
        }

        yakubovich_talks(YT_PRIZE_OR_RUBLES);
        prize_or_money_choice = ui_choice(3u);    // третий режим ui_choice это Беру ДЕНЬГИ или Беру ПРИЗ
        yak_anim_close_mouth();

        ++prize_try;
    }

    //   prize_or_money_choice==0 БЕРУ ДЕНЬГИ
    if (prize_or_money_choice == 0) {
        yakubovich_talks(YT_TAKE_MONEY);    // Забирайте ваши деньги

        while (money_ticks_left > 0ul) {
            u16 freq = (u16)(rand() % 50);  

            --money_ticks_left;

            tp_sound(freq);

            ui_draw_rle(
                (u16)(rand() % 0x0127),   // x
                (u16)(rand() % 0x0190),   // y
                RLE_TRANSPARENT_SPRITE_57_RUBLES,
                0,
                RES_GFX_RUBLE_STANDALONE
            );

            tp_nosound();
        }

        end_screen_print_footer();
        loop_wait_and_get_keys(0);
        yak_anim_close_mouth();
        return;
    }

    // Беру ПРИЗ 
    {
        u16 textY = 0x00BE;
        u16 idx;

        yakubovich_talks(YT_TAKE_PRIZE);

        textY = (u16)(textY + 0x0012);
        ega_print_centered_f14(textY, 0, kPrizeLine1);

        textY = (u16)(textY + 0x0012);
        ega_print_centered_f14(textY, 0, kPrizeLine2);

        idx = (u16)(rand() % 10);    // что-то рандомное типа подтяжек для шнурков
        concat_to_g_buffer_str(kPrizeItems[idx], kPrizeSuffix);

        textY = (u16)(textY + 0x0012);
        ega_print_centered_f14(textY, 0x0F, (const char*)g_buffer_string);

        textY = (u16)(textY + 0x0012);
        ega_print_centered_f14(textY, 0, kPrizeAddrHdr);

        textY = (u16)(textY + 0x0012);
        ega_print_centered_f14(textY, 0x0F, kPrizeAddr);

        textY = (u16)(textY + 0x0012);
        ega_print_centered_f14(textY, 0, kPrizeNote);

        end_screen_print_footer();
        loop_wait_and_get_keys(0);
        yak_anim_close_mouth();
    }
}
