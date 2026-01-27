// moneybox.c - порт Turbo Pascal процедур:
//   - shkatulkaStuff   (seg000:31CB..35D8)
//   - SELECT_BOX       (seg000:302A..31C8)

#include "moneybox.h"

#include <dos.h>
#include <conio.h>
#include <stdlib.h> // rand

#include "bgi_mouse.h"
#include "globals.h"
#include "resources.h"
#include "res_gfx.h"
#include "ega_draw.h"
#include "rle_draw.h"
#include "sys_delay.h"
#include "sound.h"
#include "score.h"
#include "yakubovich.h"
#include "boss_mode.h"
#include "input.h"
#include "ui_choice.h"


extern void ega_latch_fill(u16 offset, u16 row_count_minus_1, u16 count_bytes, u8 color);
extern void draw_rle_packed_sprite( u8 transparent, u16 y, u16 x, u8 far *vram,const u8 far *data);
extern void ega_vram_move_blocks(u16 srcOffset, u16 destOffset, u16 maxRowIndex, u16 countBytesPerLine);


 
 
#define UI_HAND_CHOSE_RIGHT_SIDE 1u

#define MONEYBOX_PRIZE_RUBLES     100u

#define MONEYBOX_Y_POS            ((u16)240u) // 0x0110

#define STRIP_SCREEN_OFFS         ((u16)(MONEYBOX_Y_POS * EGA_BPL))               // y=0xF0 => 240*80 = 0x4B00
#define STRIP_OFFSCREEN_OFFS      ((u16)(EGA_OFFSCREEN_BASE + STRIP_SCREEN_OFFS))            // 0xC800

 
//  количество строк для копирования 
#define STRIP_MAXROW              ((u16)0x50u)  
#define STRIP_BYTES               ((u16)0x14u) // ширина 20 байт (160 пикселей)

// параметры копирования панели
#define SEL_MAXROW                ((u16)26u)   // 0x1A
#define SEL_BYTES                 ((u16)20u)   // 0x14
#define SEL_VAR2_INIT             STRIP_SCREEN_OFFS // 0x4B00 : var_2 стартует отсюда (база VRAM/оффскрина)

// позиция руки-курсорa
#define SEL_HAND_Y                MONEYBOX_Y_POS   // 0x00F0
#define SEL_HAND_X_BASE           ((u16)18u)       // 0x0012
#define SEL_HAND_X_STEP           ((u16)60u)       // 0x003C
#define SEL_HAND_TRANSPARENT      ((u16)2u)        // 0x0002 :  прозрачность

// указатели на спрайты: закрытая шкатулка, открытая шкатулка, деньги, рука
#define PTR_RES_HAND_SPRITE       GET_GFX_PTR(RES_GFX_HAND_SPRITE)
#define PTR_RES_CLOSED_BOX        GET_GFX_PTR(RES_GFX_MONEYBOX_CLOSED)
#define PTR_RES_OPEN_BOX          GET_GFX_PTR(RES_GFX_MONEYBOX_OPENED)
#define PTR_RES_MONEY             GET_GFX_PTR(RES_GFX_MONEYBOX_MONEY)

#define BOX_LEFT_X                ((u16)6u)    // 0x0006
#define BOX_RIGHT_X               ((u16)64u)   // 0x0040
#define MONEY_RIGHT_X             ((u16)66u)   // 0x0042
#define MONEY_LEFT_X              ((u16)8u)    // 0x0008

// у каждого поля чудес левая шкатулка чуть выше правой
#define BOX_LEFT_Y_BASE           (MONEYBOX_Y_POS + 32u)  // ~272
#define BOX_RIGHT_Y_BASE          (MONEYBOX_Y_POS + 40u)  // ~280

#define SHUFFLED_RIGHT_BOX_Y_BASE (MONEYBOX_Y_POS + 22u)  // 262
#define SHUFFLED_RIGHT_BOX_X_BASE ((u16)49u)              // 0x0028

#define SHUFFLED_LEFT_BOX_Y_BASE  (MONEYBOX_Y_POS + 50u)  // 290
#define SHUFFLED_LEFT_BOX_X_BASE  ((u16)30u)              // 0x001E

#define MBOX_MOUSE_LEFT_TRESH     ((u16)0x012Cu)
#define MBOX_MOUSE_RIGHT_TRESH    ((u16)0x0154u)
#define MBOX_MOUSE_SET_X          ((u16)0x0140u)
#define MBOX_MOUSE_SET_Y          ((u16)0x0064u)

// ---------------- SELECT_BOX (seg000:302A..31C8) ----------------

static void moneybox_select_box(u16 *players_choice) // 0 / 1 лево/право
{
    // локальные переменные ida
    u16 var_2 = SEL_VAR2_INIT; // = 0x4B00
    u16 accepted = 0u;

      
 
    *players_choice = UI_HAND_CHOSE_RIGHT_SIDE; // default

    flushKeyboard();

    for (;;) {
        // Чистим область панели выбора внутри OFFSCREEN-полосы:
        // dst = EGA_OFFSCREEN_BASE + var_2 => 0xC800
        ega_latch_fill(
            (u16)(EGA_OFFSCREEN_BASE + var_2),
            SEL_MAXROW,
            SEL_BYTES,
            EGA_COLOR_LIGHT_GRAY
        );

        // Рисуем руку  в OFFSCREEN при y=0xF0  
        {
            u16 sel = *players_choice;
            u16 hand_x = (u16)((u16)(sel * SEL_HAND_X_STEP) + SEL_HAND_X_BASE);

            draw_rle_packed_sprite(
                (u16)SEL_HAND_TRANSPARENT,
                (u16)SEL_HAND_Y,
                (u16)hand_x,
              PTR_VRAM_7D00_OFFSCREEN,
                PTR_RES_HAND_SPRITE
            );
        }

        // Копируем маленькую панель из OFFSCREEN-полосы на видимый экран (0x4B00).
        ega_vram_move_blocks(
            (unsigned)(EGA_OFFSCREEN_BASE + var_2),
            (unsigned)var_2,
            (unsigned)SEL_MAXROW,
            (unsigned)SEL_BYTES
        );

         
        g_mPressedKey = KEY_DEFAULT; // 7 = “нет клавиши”

        if (bios_kbhit()) {
            unsigned ax = bios_getkey_ax();
            u8 al = (u8)(ax & 0xFFu);        // ASCII
            u8 ah = (u8)((ax >> 8) & 0xFFu); // SCAN

            if (al == 0u || al == 0xE0u) {
                // расширенная клавиша 
                if (ah == KEY_LEFT || ah == KEY_RIGHT) {
                    *players_choice = (u16)(1u - *players_choice);
                }
                g_mPressedKey = 0u; // как в TP: ReadKey вернул 0
            } else {
                g_mPressedKey = al;
            }

            input_process_game_sound_and_exit_control_keys();
        }

        // Обработка мыши: если есть, проверяем X в диапазоне  12C..154
        if ((s16)g_mouse_present > 0) {
            static u16 prev_buttons = 0u;

            u16 buttons = 0u;
            u16 mx = 0u, my = 0u;

            mouse_query(&buttons, &mx, &my);

            // мышка или пробел
            g_space_pressed = ((buttons & 1u) && !(prev_buttons & 1u)) ? 1u : 0u;
            prev_buttons = buttons;

            
            if (mx < MBOX_MOUSE_LEFT_TRESH || mx > MBOX_MOUSE_RIGHT_TRESH) {
                *players_choice = (u16)(1u - *players_choice);
                bgi_mouse_set_pos(MBOX_MOUSE_SET_X, MBOX_MOUSE_SET_Y);
            }
        }

        //  выбор: по SPACE или по клику мышью (spacePressed==1).
        if (g_mPressedKey == KEY_SPACE) {
            accepted = 1u;
        } else if ((s16)g_mouse_present > 0) {
            if (g_space_pressed == 1u) accepted = 1u;
        }

        // DELAY(0x1E)
        pit_delay_ms(30u);

        if (accepted != 0u) break;
    }

    // Перед выходом чистим область панели на видимом экране (в ida dst=var_2).
    ega_latch_fill(
        var_2,
        SEL_MAXROW,
        SEL_BYTES,
        EGA_COLOR_LIGHT_GRAY
    );
}

void moneybox_run(void)
{
    // локальные переменные по раскладке стека в дизасме
    u16 copyvar4_highBoundariesAnimation;
    u16 whereisMoney_initWith_1;
    u16 players_choice;
    u16 y_offset;
    u16 x_offset;
    u16 animIterator;

    tp_nosound();

    x_offset = 5u;
    y_offset = 0x18u;

    // Чистим OFFSCREEN-полосу (A000:C800) под сцену.
    ega_latch_fill(
        STRIP_OFFSCREEN_OFFS,
        STRIP_MAXROW,
        STRIP_BYTES,
        EGA_COLOR_LIGHT_GRAY
    );

    // рисуем шкатулки в offscreen

    // левая открытая шкатулка
    draw_rle_packed_sprite(
       RES_GFX_TRANSPARENT_GRAY,
        (u16)(BOX_LEFT_Y_BASE - x_offset),
        BOX_LEFT_X,
      PTR_VRAM_7D00_OFFSCREEN,
        PTR_RES_OPEN_BOX
    );

    // правая открытая шкатулка
    draw_rle_packed_sprite(
        RES_GFX_TRANSPARENT_GRAY,
        (u16)(BOX_RIGHT_Y_BASE - x_offset),
        BOX_RIGHT_X,
      PTR_VRAM_7D00_OFFSCREEN,
        PTR_RES_OPEN_BOX
    );

    // деньги в правой шкатулке
    draw_rle_packed_sprite(
       RES_GFX_TRANSPARENT_GRAY,
        (u16)(BOX_RIGHT_Y_BASE - y_offset),
        MONEY_RIGHT_X,
      PTR_VRAM_7D00_OFFSCREEN,
        PTR_RES_MONEY
    );

    
    animIterator = 0x28u;

    for (;;) {
        // SOUND( (0x3E8 - anim*0x14) * multiplier )
        {
            u16 dx = (u16)(animIterator * 0x14u);
            u16 f  = (u16)(1000u - dx);
            f = (u16)(f * g_soundONOF_soundMultiplier);
            tp_sound(f);
        }

        // moveBlocks:
        //   src = 0xC800
        //   dst = 0x4B00 + (anim*2)*0x50  (каждый шаг — на 2 строки ниже)
        //   maxRowIndex = 0x50 - (anim*2)
        //   bytes = 0x14
        {
            u16 a2   = (u16)(animIterator * 2); 
            u16 dst  = (u16)(STRIP_SCREEN_OFFS + (u16)(a2 * EGA_BPL));
            u16 maxr = (u16)(0x50u - a2);

            ega_vram_move_blocks(
                (unsigned)STRIP_OFFSCREEN_OFFS,
                (unsigned)dst,
                (unsigned)maxr,
                (unsigned)STRIP_BYTES
            );
        }

        tp_nosound();

        // DELAY(animIterator)
        pit_delay_ms(animIterator);

        if (animIterator == 0u) break;
        --animIterator;
    }

    // loop_wait_and_get_keys(0)
    loop_wait_and_get_keys(0u);

    // Чистим полосу  maxRow=0x33  bytes=0x14
    ega_latch_fill(
        STRIP_OFFSCREEN_OFFS,
        (u16)0x33u,
        STRIP_BYTES,
       EGA_COLOR_LIGHT_GRAY
    );

    // SOUND(mult*1000)
    tp_sound((u16)(g_soundONOF_soundMultiplier * 1000u));

    // Рисуем закрытую левую шкатулку в OFFSCREEN
    draw_rle_packed_sprite(
        RES_GFX_TRANSPARENT_GRAY,
        BOX_LEFT_Y_BASE,
        BOX_LEFT_X,
      PTR_VRAM_7D00_OFFSCREEN,
        PTR_RES_CLOSED_BOX
    );

    tp_nosound();

    // SOUND(mult*100)
    tp_sound((u16)(g_soundONOF_soundMultiplier * 100u));

    // Рисуем закрытую правую шкатулку в OFFSCREEN
    draw_rle_packed_sprite(
        RES_GFX_TRANSPARENT_GRAY,
        BOX_RIGHT_Y_BASE,
        BOX_RIGHT_X,
      PTR_VRAM_7D00_OFFSCREEN,
        PTR_RES_CLOSED_BOX
    );

    tp_nosound();

    // SOUND(mult*500)
    tp_sound((u16)(g_soundONOF_soundMultiplier * 500u));

    // Копируем  на экран (src=C800 -> dst=4B00, maxRow=0x50, bytes=0x14)
    ega_vram_move_blocks(
        (unsigned)STRIP_OFFSCREEN_OFFS,
        (unsigned)STRIP_SCREEN_OFFS,
        (unsigned)STRIP_MAXROW,
        (unsigned)STRIP_BYTES
    );

    tp_nosound();

    
    loop_wait_and_get_keys(0x28u);

    // x_offset = (Random(0x14)+0x0A)*2  => чётное 20..58
    {
        u16 r = (u16)(rand() % 20u);  
        r = (u16)(r + 10u);           
        x_offset = (u16)(r << 1);    
    }

    // whereisMoney_initWith_1 = 1; copyvar4 = x_offset
    whereisMoney_initWith_1 = 1u;
    copyvar4_highBoundariesAnimation = x_offset;

    // animIterator = 1; цикл до animIterator == copyvar4
    animIterator = 1u;

    for (;;) {
        // SOUND( (Random(0x64)+0x32) * multiplier )
        {
            u16 rr = (u16)(rand() % 100u); // 0..99
            u16 f  = (u16)(rr + 0x32u);    // +50
            f = (u16)(f * g_soundONOF_soundMultiplier);
            tp_sound(f);
        }

        // DELAY(0x0A); NOSOUND; DELAY(0x32)
        pit_delay_ms(10u);
        tp_nosound();
        pit_delay_ms(50u);

        // если animIterator чётный -> переключаем whereisMoney_initWith_1
        if ((animIterator & 1u) == 0u) {
            whereisMoney_initWith_1 = (u16)(1u - whereisMoney_initWith_1);
        }

        // чистим полосу
        ega_latch_fill(
            STRIP_OFFSCREEN_OFFS,
            STRIP_MAXROW,
            STRIP_BYTES,
            EGA_COLOR_LIGHT_GRAY
        );

        // если animIterator чётный -> шкатулки на (6,110) и (40,118)
        // иначе -> шкатулки на (28,106) и (1E,122)
        if ((animIterator & 1u) == 0u) {
            draw_rle_packed_sprite(
                RES_GFX_TRANSPARENT_GRAY,
                BOX_LEFT_Y_BASE,
                BOX_LEFT_X,
              PTR_VRAM_7D00_OFFSCREEN,
                PTR_RES_CLOSED_BOX
            );
            draw_rle_packed_sprite(
               RES_GFX_TRANSPARENT_GRAY,
                BOX_RIGHT_Y_BASE,
                BOX_RIGHT_X,
              PTR_VRAM_7D00_OFFSCREEN,
                PTR_RES_CLOSED_BOX
            );
        } else {
            draw_rle_packed_sprite(
                RES_GFX_TRANSPARENT_GRAY,
                SHUFFLED_RIGHT_BOX_Y_BASE,
                SHUFFLED_RIGHT_BOX_X_BASE,
              PTR_VRAM_7D00_OFFSCREEN,
                PTR_RES_CLOSED_BOX
            );
            draw_rle_packed_sprite(
                RES_GFX_TRANSPARENT_GRAY,
                SHUFFLED_LEFT_BOX_Y_BASE,
                SHUFFLED_LEFT_BOX_X_BASE,
              PTR_VRAM_7D00_OFFSCREEN,
                PTR_RES_CLOSED_BOX
            );
        }

        // копируем полосу на экран
        ega_vram_move_blocks(
            (unsigned)STRIP_OFFSCREEN_OFFS,
            (unsigned)STRIP_SCREEN_OFFS,
            (unsigned)STRIP_MAXROW,
            (unsigned)STRIP_BYTES
        );

        if (animIterator == copyvar4_highBoundariesAnimation) break;
        ++animIterator;
    }

    //  whereisMoney  

    yak_anim_close_mouth();
    yakubovich_talks(YT_WHERE_MONEY); // “где деньги?”

    // SELECT_BOX пишет players_choice (0/1)
    moneybox_select_box(&players_choice);

    yak_anim_close_mouth();

   
    x_offset = 5u;

    // чистим полосу
    ega_latch_fill(
        STRIP_OFFSCREEN_OFFS,
        STRIP_MAXROW,
        STRIP_BYTES,
        EGA_COLOR_LIGHT_GRAY
    );

   
    {
       // какую выбрали такой и будет спрайт
        
         u16 res_left = RES_GFX_NULL_SPRITE;
         u16 res_right = RES_GFX_NULL_SPRITE;
         
        if (players_choice == UI_HAND_CHOSE_RIGHT_SIDE) { 
            res_left = (u16)RES_GFX_MONEYBOX_CLOSED;
            res_right = (u16)RES_GFX_MONEYBOX_OPENED;

        } else if (players_choice == UI_HAND_CHOSE_LEFT_SIDE) { 
            res_left = (u16)RES_GFX_MONEYBOX_OPENED;
            res_right = (u16)RES_GFX_MONEYBOX_CLOSED;
        }
        
        
        DBG("players_choice=%u res_left=%u res_right=%u\n", players_choice, res_left, res_right);

        // лево
        {
            u16 dy = (u16)((u16)(1u - players_choice) * x_offset);
            u16 yy = (u16)(BOX_LEFT_Y_BASE - dy);

            draw_rle_packed_sprite(
               RES_GFX_TRANSPARENT_GRAY,
                yy,
                BOX_LEFT_X,
              PTR_VRAM_7D00_OFFSCREEN,
                GET_GFX_PTR(res_left)
            );
        }

        // право
        {
            u16 dy = (u16)(players_choice * x_offset);
            u16 yy = (u16)(BOX_RIGHT_Y_BASE - dy);

            draw_rle_packed_sprite(
                RES_GFX_TRANSPARENT_GRAY,
                yy,
                BOX_RIGHT_X,
              PTR_VRAM_7D00_OFFSCREEN,
                GET_GFX_PTR(res_right)
            );
        }
    }

    // копируем полосу на экран
    ega_vram_move_blocks(
        (unsigned)STRIP_OFFSCREEN_OFFS,
        (unsigned)STRIP_SCREEN_OFFS,
        (unsigned)STRIP_MAXROW,
        (unsigned)STRIP_BYTES
    );

    // если угадал неправильно -> sorryNOMONEY
    if (whereisMoney_initWith_1 != players_choice) {
        yakubovich_talks(YT_BOX_EMPTY); // “шкатулка пуста”
    } else {
        // добавляем 100 рублей активному игроку
        g_score_table[g_active_player] += MONEYBOX_PRIZE_RUBLES;

        // рисуем деньги прямо на видимый экран (dest = A000:0000)
        if (whereisMoney_initWith_1 == 1u) {
            // справа
            draw_rle_packed_sprite(
                RES_GFX_TRANSPARENT_GRAY,
                (u16)(BOX_RIGHT_Y_BASE - y_offset),
                MONEY_RIGHT_X,
                vram_ptr(0u),
                PTR_RES_MONEY
            );
        } else {
            // слева
            draw_rle_packed_sprite(
                RES_GFX_TRANSPARENT_GRAY,
                (u16)(BOX_LEFT_Y_BASE - y_offset),
                MONEY_LEFT_X,
                vram_ptr(0u),
                PTR_RES_MONEY
            );
        }

        yakubovich_talks(YT_BOX_GUESSED_RIGHT); //  вы угадали 
        player_draw_score(g_active_player);
    }

    loop_wait_and_get_keys(0u);

    yak_anim_close_mouth();

    // чистим видимую полосу (dst = 0x4B00)
    ega_latch_fill(
        STRIP_SCREEN_OFFS,
        STRIP_MAXROW,
        STRIP_BYTES,
        EGA_COLOR_LIGHT_GRAY
    );

    tp_nosound();
}
