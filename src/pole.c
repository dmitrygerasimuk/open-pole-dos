// pole.c - главный цикл игры (PROGRAM)
 

#include <stdio.h>
#include <dos.h>
#include <i86.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>

#include "typedefs.h"
#include "debuglog.h"
#include "rle_draw.h"
#include "resources.h"
#include "res_gfx.h"
#include "ega_text.h"
#include "sys_delay.h"
#include "ega_draw.h"
#include "input.h"
#include "baraban.h"
#include "bgi_mouse.h"
#include "bgi_palette.h"
#include "score.h"
#include "yakubovich.h"
#include "words.h"
#include "end_screen.h"
#include "startscreen.h"
#include "sale_info.h"
#include "globals.h"
#include "ui_answer.h"
#include "ui_hof.h"
#include "girl.h"
#include "moneybox.h"
#include "prize.h"
#include "letter.h"
#include "opponents.h"
#include "pole_dialog.h"
#include "ui_choice.h"
#include "ui_keymap.h"

// positions from disasm

#define POLE_MAX_ROUNDS 3

// baraban counter default
#define BARABAN_DEFAULT_COUNTER  0u

// заголовок раунда
#define STAGE_TITLE_X        204u
#define STAGE_TITLE_Y        78u

// полоска с буквами снизу
#define BLUE_TILE_Y          332u
#define LETTER_LABEL_Y       334u
#define LETTER_STEP_X        20u
#define LETTER_LABEL_X_OFF   4u

#define LETTERS_ROW_NUM_OF_LETTERS 32u

// плашки под слово 
#define WORD_BOX_Y1          36u
#define WORD_BOX_Y2          54u
#define WORD_BOX_X_BASE      121u
#define WORD_BOX_X2_OFF      13u
#define WORD_BOX_SLOT_OFF    5u


static void shuffle_player_shuffle_1based(void)
{
    // 0 не трогаем: массив перестановки игроков с 1 
    s16 i;

 for ( i = (s16)g_opponents_count(); i > 1; --i) {   // фишер-йейтс 
    s16 j = (s16)((rand() % i) + 1);  /* 1..i */
    u16 tmp = g_player_shuffle[(u16)i];
    g_player_shuffle[(u16)i] = g_player_shuffle[(u16)j];
    g_player_shuffle[(u16)j] = tmp;
 } 

}

static const char* stage_title_by_round_wins(u16 rw)
{
    if (rw == 0u) return STR_STAGE_TITLE_CHETVERTFINAL;
    if (rw == 1u) return STR_STAGE_TITLE_POLUFINAL;
    return STR_STAGE_TITLE_FINAL;
}

void ui_print_stage_title(void)
{
    u16 hminus1 = 13u;
    const char *s = stage_title_by_round_wins(g_round_wins);

    // 0x10 для  шага
    print_text(gRes.fonts.f14, VRAM_OFFSET_ONSCREEN, 0x10u, hminus1, 0u, STAGE_TITLE_Y, STAGE_TITLE_X, s);
}

static void ui_draw_background(u8 __far *vram)
{
    unsigned i;

  
    ega_gc_write(5u, 0u);

    // рамка  серая 
    ega_wm2_bar_u(vram, 120, 15, 520, 95, (u8)EGA_COLOR_LIGHT_GRAY);

    // горизонтальные линии: 
    for (i = 0u; i <= 4u; ++i) {
        u16 y = (u16)(15u + (u16)(i * 20u));
        ega_wm2_hline_fast(vram, 120, 520, (s16)y, (u8)EGA_COLOR_BLACK);
    }

    // вертикальные линии: 
    for (i = 0u; i < 26u; ++i) {
        u16 x = (u16)(120u + (u16)(i * 16u));
        ega_wm2_vline_fast(vram, (s16)x, 15, 95, (u8)EGA_COLOR_BLACK);
    }

    ega_gc_write(5u, 0u);

    // заголовок стадии (четвертьфинал/полуфинал/финал)
    ui_print_stage_title();
}

static void ui_draw_random_tiles_and_props(u8 __far *vram)
{
    u16 t;

    // 36 случайных плиток фона, сетка 12x3
    for (t = 0u; t <= 35u; ++t) {
        u16 rid = (u16)(RES_GFX_RANDOM_BACK_TILE_0 + (u16)(rand() % 3));
        u16 x   = (u16)(((t % 12u) * 52u) + 5u);
        u16 y   = (u16)(((t / 12u) * 31u) + 15u);

        draw_rle_packed_sprite(
            (u8)EGA_COLOR_BLUE, // маска прозрачности
            y, x,
            vram,
            (const u8 __far*)GET_GFX_PTR(rid)
        );
    }

    // синие  стены
    draw_rle_packed_sprite(
        (u8)EGA_COLOR_LIGHT_GRAY,
        25u, 0u,
        vram,
        (const u8 __far*)GET_GFX_PTR(RES_GFX_BACKGROUND_BLUE_WALL_LEFT)
    );

    draw_rle_packed_sprite(
        (u8)EGA_COLOR_LIGHT_GRAY,
        25u, 600u,
        vram,
        (const u8 __far*)GET_GFX_PTR(RES_GFX_BACKGROUND_BLUE_WALL_RIGHT)
    );

    // две зелёные лампы
    draw_rle_packed_sprite(
        (u8)EGA_COLOR_BLUE,
        3u, 69u,
        vram,
        (const u8 __far*)GET_GFX_PTR(RES_GFX_BACKGROUND_GREEN_LAMP)
    );

    draw_rle_packed_sprite(
        (u8)EGA_COLOR_BLUE,
        3u, 559u,
        vram,
        (const u8 __far*)GET_GFX_PTR(RES_GFX_BACKGROUND_GREEN_LAMP)
    );

    ega_gc_write(5u, 0u);
}

static void ui_draw_letter_tiles_and_labels(u8 __far *vram)
{
    unsigned i;
    char one[2];

    one[1] = '\0';

    // синие плитки под алфавитом
    for (i = 0u; i < (u16)LETTERS_ROW_NUM_OF_LETTERS; ++i) {
        u16 x = (u16)(i * (u16)LETTER_STEP_X);
        draw_rle_packed_sprite(8u, (u16)BLUE_TILE_Y, x, vram,
                               (const u8 __far*)GET_GFX_PTR(RES_GFX_REVEAL_0));
    }

    // подписи букв АБВГДФВФЫ
    for (i = 0u; i < (u16)LETTERS_ROW_NUM_OF_LETTERS; ++i) {
        u16 x = (u16)(i * (u16)LETTER_STEP_X + (u16)LETTER_LABEL_X_OFF);
        one[0] = (char)kAlphabet32Pas[1u + i];

        print_text(gRes.fonts.f14, 0u, 8u, 0x0Du, 0u, (u16)LETTER_LABEL_Y, x, one);
    }
}

static void ui_clear_word_state_opened(u16 wordLen)
{
    unsigned i;

    for (i = 0u; i < wordLen; ++i) {
        g_word_state.opened[i] = 0u;
    }
    g_word_state.match_count = 0u;
}

static void ui_draw_word_boxes(u8 __far *vram, u16 wordLen)
{
    unsigned i;

    for (i = 1u; i <= wordLen; ++i) {
        s16 slot = (s16)(((s16)i + (s16)WORD_BOX_SLOT_OFF) << 4);
        s16 x1 = (s16)WORD_BOX_X_BASE + slot;
        s16 x2 = (s16)WORD_BOX_X_BASE + slot + (s16)WORD_BOX_X2_OFF;
        ega_wm2_bar_u(vram, x1, (s16)WORD_BOX_Y1, x2, (s16)WORD_BOX_Y2, (u8)EGA_COLOR_DARK_GRAY);
    }
}

static u16 recount_hidden_letters(u16 wordLen)
{
    unsigned i;
    u16 hidden = 0u;

    for (i = 0u; i < wordLen; ++i) {
        if (g_word_state.opened[i] == 0u) ++hidden;
    }
    return hidden;
}

#define XMAX  (EGA_SCR_W)
#define YMAX  (EGA_SCR_H)

static void pole_draw_underlay(u8 __far *vram)
{

    // void ega_wm2_bar_u(u8 __far *vram, s16 x1, s16 y1, s16 x2, s16 y2, u8 color);
    ega_gc_write(0x05u, 0x00u);

    // seg000:57FD 9A AA 13 4B 17                    call    @BAR$q7INTEGERt1t1t1 ; BAR(INTEGER,INTEGER,INTEGER,INTEGER)
    ega_wm2_bar_u(vram, 0, 0, XMAX, 0x0A, (u8)EGA_COLOR_CYAN);

    // отделяем потолок от стены
    ega_wm2_bar_u(vram, 0, 0x0A, XMAX, 0x0C, (u8)EGA_COLOR_DARK_GRAY);

    // основной фон под плитки 
    ega_wm2_bar_u(vram, 0, 0x0D, XMAX, 0x6B, (u8)EGA_COLOR_BLUE);

    // отделяем стену от пола  
    ega_wm2_bar_u(vram, 0, 0x6C, XMAX, 0x6E, (u8)EGA_COLOR_DARK_GRAY);

    // большая светлая область (пол)
    ega_wm2_bar_u(vram, 0, 0x6F, XMAX, YMAX, (u8)EGA_COLOR_LIGHT_GRAY);

    // линия над блоком букв  
    ega_wm2_bar_u(vram, 0, 0x14A, XMAX, 0x14B, (u8)EGA_COLOR_DARK_GRAY);

    ega_gc_write(0x05u, 0x00u);
}

// ---------- PROGRAM seg000:56B2                                   public PROGRAM
// seg000:56B2                   PROGRAM         proc near ----------

void start_game(void)
{
    u8 __far *vram_onscreen = (u8 __far*)MK_FP(EGA_VIDEO_SEGMENT, VRAM_OFFSET_ONSCREEN);

    unsigned i;
    u16 letters_left;
    u16 wordLen;
    s16 current_sector;

    // init video
    bgi_mouse_init();
    ega_mode10_set();
    ega_clear_0();
    bgi_palette_apply_startup();
    bgi_mouse_init_640x350();

    g_soundONOF_soundMultiplier = 1u; // default - sound ON

    shuffle_player_shuffle_1based();
    
    startscreen(&gRes);
 
    g_round_wins = 0u;

    for (i = 1u; i <= 3u; ++i) g_score_table[i] = 0;

    g_used_word_idx[1] = 0u;
    g_used_word_idx[2] = 0u;

    //  ИГРОВОЙ ЦИКЛ
    // ---------------- 
    do {
        //рандомно подкинем денег игрокам
        for (i = 1u; i <= 2u; ++i) {
            s32 base = (s32)((rand() % 4) * 15 + 5);
            s32 val  = base * (s32)g_round_wins;
            g_score_table[i] = (s16)val;
        }

        // рисуем игрровой экран
        ega_clear_screen_fast((u8)EGA_COLOR_LIGHT_GRAY);
        pole_draw_underlay(vram_onscreen);
        ui_draw_random_tiles_and_props(vram_onscreen);
        ui_draw_background(vram_onscreen);
        yak_draw_yakubovich(vram_onscreen);
        ui_draw_letter_tiles_and_labels(vram_onscreen);

        g_wheel_anim_counter = 0u;
        baraban_draw((u16)BARABAN_DEFAULT_COUNTER);

        draw_opponents();
        g_score_table[0] = 0;

     
        clear_used_letters();

       
        for (i = 1u; i <= 3u; ++i) player_draw_score(i);

        words_load_word_and_randomize();

        // плашки слова
        wordLen = (u16)G_WORDLEN;
        ui_clear_word_state_opened(wordLen);
        ui_draw_word_boxes(vram_onscreen, wordLen);

        // yakubovich_talks / wait / animate 
        yakubovich_talks(YT_WORD_CHOSEN_START);
        loop_wait_and_get_keys(0x32u);
        yak_anim_close_mouth();

        yakubovich_talks(YT_THEME);
        loop_wait_and_get_keys(0u);
        yak_anim_close_mouth();

        g_active_player = (u16)PLAYER_HUMAN;
        g_guessed_letters_right = 0u;

        // status (в гидре gameStatus_ZeroIfWon)
        g_game_status_zeroifwon = 0;

        // ВНУТРЕННИЙ ЦИКЛ В ОБЩЕМ ИГРОВОМ
        // ХОДЫ ИГРОКОВ
        
        do {
            if (g_guessed_letters_right == 3u) {
                yakubovich_talks(YT_3LETTERS_BONUS);
                moneybox_run();
                g_guessed_letters_right = 0u;
            }

            g_word_state.match_count = 0u;

            if (g_active_player > (u16)PLAYER_HUMAN) {
                g_active_player = (u16)PLAYER_1;
            }

            yakubovich_talks(YT_TURN_SPIN);

            g_game_status_zeroifwon = (u16)UI_ACT_CONTINUE;

            if (g_active_player == (u16)PLAYER_HUMAN) {
                g_game_status_zeroifwon = (u16)ui_choice(UI_ACT_MODE1_SKAZHU_KRUCHU);
            }

            if (g_game_status_zeroifwon == (u16)UI_ACT_CONTINUE) {
                baraban_spin();

                // если сектор не ПЛЮС, увеличиваем счётчик правильных букв подряд
                if (g_score_table[0] != 100) {
                    ++g_guessed_letters_right;
                }

                yakubovich_talks(YT_AFTER_SPIN_SECTOR);

                // призовой сектор: только для человека
                if ((g_score_table[0] == 300) && (g_active_player == (u16)PLAYER_HUMAN)) {
                    g_game_status_zeroifwon = (u16)ui_choice(UI_ACT_MODE2_PRIZ_ILI_IGRAT);
                    if (g_game_status_zeroifwon == (u16)UI_ACT_CONTINUE) {
                        yak_anim_close_mouth();
                        yakubovich_talks(YT_IF_SO_NAME_LETTER);
                    }
                }

                if (g_game_status_zeroifwon == 0u) {
                    loop_wait_and_get_keys(0x28u);
                    yak_anim_close_mouth();

                    current_sector = (s16)g_score_table[0];

                    // отрицательный сектор: смерть, сброс денег
                    if (current_sector < 0) {
                        g_score_table[g_active_player] = 0;
                        player_draw_score(g_active_player);
                        current_sector = (s16)g_score_table[0];
                    }

                    // сектор 0: фига, передаём ход
                    if (current_sector < 1) {
                        ++g_active_player;
                        g_guessed_letters_right = 0u;
                    } else {
                        // сектор 100: ПЛЮС
                        if ((u16)current_sector == 100u) {
                            sector_PLUS_open_letter();
                        } else {
                            pick_letter_player();
                        }

                        // если буква не совпала ни разу, ход дальше
                        if (g_word_state.match_count < 1u) {
                            ++g_active_player;
                            g_guessed_letters_right = 0u;
                        } else {
                            // совпало: анимация девочки + начисление
                            girl_and_score_run();
                        }
                    }
                }

                letters_left = recount_hidden_letters((u16)G_WORDLEN);
            } else {
                // если status выставлен ui_choice — просто пересчёт условия цикла
                letters_left = recount_hidden_letters((u16)G_WORDLEN);
            }

        } while ((letters_left != 0u) && (g_game_status_zeroifwon < 1u));

        // КОНЕЦ РАУНДА
        // -------------- 
        if (letters_left == 0u) {
            yakubovich_talks(YT_ROUND_WIN);
            loop_wait_and_get_keys(0x32u);
            yak_anim_close_mouth();

            // победа зависит от того, кто завершил раунд
            g_game_status_zeroifwon =
                (g_active_player == (u16)PLAYER_HUMAN) ? (u16)GAME_STATUS_HUMAN_WON : (u16)GAME_STATUS_HUMAN_LOST;
        } else {
            yak_anim_close_mouth();

            if (g_game_status_zeroifwon == (u16)UI_ACT_MODE2_PRIZ_ILI_IGRAT) {
                beruPRIZ();
            } else if (g_game_status_zeroifwon > 0u) {
                g_game_status_zeroifwon = (u16)ui_enter_answer_word();
                if (g_game_status_zeroifwon == 0u) {
                    yakubovich_talks(YT_YOU_ARE_CORRECT);
                } else {
                    yakubovich_talks(YT_WRONG_ELIM);
                }
            }

            loop_wait_and_get_keys(50u);
            yak_anim_close_mouth();
        }

        // при победе: увеличиваем roundWins и показываем рекламную паузу
        if (g_game_status_zeroifwon == (u16)GAME_STATUS_HUMAN_WON) {
            ++g_round_wins;
            yakubovich_talks(YT_AD_BREAK);
            loop_wait_and_get_keys(0u);
            yak_anim_close_mouth();
            sale_info_print();
        }

    } while ((g_game_status_zeroifwon == (u16)GAME_STATUS_HUMAN_WON) && (g_round_wins < (u16)POLE_MAX_ROUNDS));

    if (g_round_wins < (u16)POLE_MAX_ROUNDS) {
        yakubovich_talks(YT_FINAL_LOSE);
        loop_wait_and_get_keys(0u);
        yak_anim_close_mouth();
    } else {
        // ура, выиграли финал
        ui_hall_of_fame_routine();
    }

    exit_game();
}



void test_run_all(void) { 
unsigned i;
u16 a;
 
char tmp[64];
#define XTEXT 160u
#define YTEXT 40u

loadResourcesFromFiles(&gRes, "POLE.LIB", "POLE.FNT", "POLE.PIC");

 bgi_mouse_init();
    ega_mode10_set();
  
    bgi_palette_apply_startup();
    bgi_mouse_init_640x350();

    g_soundONOF_soundMultiplier = 1u; // default - sound ON
// BARABAN
    ega_clear_screen_fast(EGA_COLOR_LIGHT_GRAY);
  
       a =  baraban_spin();
       sprintf(tmp,"g_wheel_sector: %d",a);
       ega_draw_text_8xN(gRes.fonts.f8, 8u, XTEXT, YTEXT, EGA_COLOR_BLACK, 8u, tmp);
       sprintf(tmp,"g_wheel_anim_counter: %d",g_wheel_anim_counter);
       ega_draw_text_8xN(gRes.fonts.f8, 8u, XTEXT, YTEXT+10u, EGA_COLOR_BLACK, 8u, tmp);
       
 loop_wait_and_get_keys(0u);
          ega_clear_screen_fast(EGA_COLOR_BLUE);
  ega_vram_move_blocks(
            WHEEL_XBASE_OFF,
             (u16)(WHEEL_XBASE_OFF + EGA_OFFSCREEN_BASE),
           
            WHEEL_COPY_ROWS,
            WHEEL_COPY_COUNBYTES
        );
for (i=g_wheel_anim_counter; i<=g_wheel_anim_counter+BARABAN_STEPS; i++) { 
   
  
        baraban_draw(i);

   
sprintf(tmp,"i: %d",i);

 ega_draw_text_8xN(gRes.fonts.f8, 8, XTEXT, 116u, EGA_COLOR_LIGHT_GRAY, 8u, tmp);
 
loop_wait_and_get_keys(0u);
ega_vram_move_blocks(
            (u16)(WHEEL_XBASE_OFF + EGA_OFFSCREEN_BASE),
            WHEEL_XBASE_OFF,
            WHEEL_COPY_ROWS,
            WHEEL_COPY_COUNBYTES
        );
}


    
    pole_draw_underlay(PTR_VRAM_A000);
    ega_draw_text_8xN(gRes.fonts.f14, 14u, 10, 20, 2, 8u, "MONEYBOX");
    moneybox_run();
    ega_clear_0();
    ega_draw_text_8xN(gRes.fonts.f14, 14u, 10, 20, 2, 8u, " SALE_INFO_PRINT");
    sale_info_print();
    ega_clear_screen_fast(0);
    ega_draw_text_8xN(gRes.fonts.f14, 14u, 10, 20, 2, 8u, " DRAW_OPPONENTS");
     
    for (i=0u; i<6u; i++) g_player_shuffle[i]=3;

        
   
    draw_opponents();
    ega_clear_screen_fast(4);
    g_current_game_word[0] = ui_map_input_letter('G');  
    g_current_game_word[1] = ui_map_input_letter('H');  
    g_current_game_word[2] = ui_map_input_letter('B');  
    g_current_game_word[3] = ui_map_input_letter('D');  
    g_current_game_word[4] = ui_map_input_letter('T');  
    g_current_game_word[5] = ui_map_input_letter('N');  
    g_current_game_word[6] = ui_map_input_letter('N');  
    g_current_game_word[7] = ui_map_input_letter('D'); 
     
    g_current_game_word[8] = '\0'; 
     ui_draw_letter_tiles_and_labels(PTR_VRAM_A000);
       ui_draw_word_boxes(PTR_VRAM_A000, G_WORDLEN);
     sector_PLUS_open_letter();
     if (g_word_state.match_count>0) girl_and_score_run();
  
     ui_choice(UI_ACT_MODE1_SKAZHU_KRUCHU);

    exit_game();

}
