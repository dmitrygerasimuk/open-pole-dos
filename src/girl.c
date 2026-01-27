// girl.c - порт girl_and_score_run (максимально близко к логике из Ghidры)

#include "girl.h"

#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>

#include "globals.h"
#include "resources.h"
#include "res_gfx.h"
#include "ega_text.h"
#include "ega_draw.h"
#include "score.h"
#include "rle_draw.h"
#include "sys_delay.h"
#include "sound.h"
#include "debuglog.h"
#include "cstr.h"
#include "input.h"
#include "boss_mode.h"

 
// g_score_table[0] хранит выпавшее значение 
// 202 = x2, 204 = x4  
#define BARABAN_MULT_BASE   200
#define BARABAN_MULT_X2     202
#define BARABAN_MULT_X4     204

#define BACKGROUND_BACKUP_HEIGHT 120u  

// --- offscreen

#define FIXED_VRAM_OFFSCREEN_BUFFER_X 0
#define FIXED_VRAM_OFFSCREEN_BUFFER_Y 200
#define FIXED_VRAM_OFF_BYTES \
    (EGA_OFFSCREEN_BASE + X_Y_TO_OFFSET(FIXED_VRAM_OFFSCREEN_BUFFER_X, FIXED_VRAM_OFFSCREEN_BUFFER_Y))

// 0x81C9 место в offscreen начинаются буквы
#define SLOTS_START_X 200u
#define SLOTS_START_Y 15u
#define SLOTS_START_OFFSET_OFFSCR \
    (EGA_OFFSCREEN_BASE + X_Y_TO_OFFSET(SLOTS_START_X, SLOTS_START_Y))

#define SLOT_WIDTH 16u
#define SLOT_XBYTES (SLOT_WIDTH / 8u)

// --- геометрия/фазы

#define GIRL_INIT_X_PX        ((u16)40)
#define GIRL_INIT_Y_PX        ((u16)25)

#define LOOP_X_LIMIT_PX       ((u16)583)   // цикл пока girl_X < 583

#define LETTER_X_BASE_PX      ((u16)110)   // 0x6E
#define LETTER_PRINT_X_BASE_PX ((u16)125)  // 0x7D

#define PHASE_PRINT           ((u16)10)
#define PHASE_MAX             ((u16)20)

#define STRIP_WBYTES          ((u16)6)     // ширина полосы в байтах (48 пикселей)
#define STRIP_ROWS            ((u16)90)    // высота полосы в строках

#define FINAL_WBYTES          ((u16)8)
#define FINAL_ROWS            ((u16)91)

// print_text(OFFSCR, adv=8, gh_m1=13, color=0, y=38, x=..., "A")
#define PRINT_ADV_PIX         ((u16)8)
#define PRINT_GH_M1           ((u16)13)
#define PRINT_COLOR           ((u8)0)
#define PRINT_Y_PX            ((u16)38)

static inline u16 found_pos_1based(u16 idx1)
{
    // idx1: 1..match_count, а массив found[] 0-based
    return (u16)g_word_state.found[idx1 - 1u];  
}

static inline u16 calc_letter_x_px(u16 found_pos)
{
    // (found + 5) * 16 + 110
    return (u16)((found_pos + 5u) * 16u + LETTER_X_BASE_PX);
}

static inline u16 calc_letter_print_x_px(u16 found_pos)
{
    // (found + 5) * 16 + 125
    return (u16)((found_pos + 5u) * 16u + LETTER_PRINT_X_BASE_PX);
}

static void girl_poll_keys(void)
{
    // Гидра: mPressedKey=7; if keypressed then readkey; дальше ctrl+s/tab/esc
    g_mPressedKey = KEY_DEFAULT;

    if (bios_kbhit()) {
        unsigned ax = bios_getkey_ax(); 
        g_mPressedKey = (u8)(ax & 0xFFu);  
    }

    input_process_game_sound_and_exit_control_keys();
}

// --- main ---

void girl_and_score_run(void)
{
    unsigned current_idx1;
    u16 letter_x_px;

    u16 girl_sprite_res;
    unsigned phase;
    unsigned frame_counter;

    unsigned step;
    u16 girl_x_px;
    u16 girl_y_px;

    u16 print_x_px;
    u16 prev_strip_off;
    u16 cur_strip_off;

    u16 fixed_off_bytes;
    u16 score_slot_off_bytes;

    char letter_s[2];
   
    tp_nosound();

    // current_idx1 = 1; letter_X = (found[0]+5)*16 + 110
    current_idx1 = 1;
    letter_x_px = calc_letter_x_px(found_pos_1based(current_idx1));

    // Гидра: ega_vram_move_blocks(0x50,0x78,32000,0)
    // Смысл: скопировать 120 строк по 80 байт с экрана (0) в OFFSCR (32000)
    ega_vram_move_blocks(
        (u16)0,
        (u16)EGA_OFFSCREEN_BASE,
        (u16)BACKGROUND_BACKUP_HEIGHT,
        (u16)EGA_BPL
    );

    fixed_off_bytes = (u16)FIXED_VRAM_OFF_BYTES;

    girl_x_px = GIRL_INIT_X_PX;
    girl_y_px = GIRL_INIT_Y_PX;

    phase = 0;
    frame_counter = 0;

    prev_strip_off = X_Y_TO_OFFSET(girl_x_px, girl_y_px);
    cur_strip_off  = prev_strip_off;

    step = 20u; 

    // разгон звука: step 20 - 100
    for (;;) {
        u16 f = (g_soundONOF_soundMultiplier * step);   // тут хрупкое место почему-то, при определенных сборках кто-то ломает стек, вероятно кто-то кто читает таймер todo погонять в дебагере todo
                                                                                 
        tp_sound(f);
        pit_delay_ms(1u); 
          
        tp_nosound();
        pit_delay_ms((u16)((100u - step) / 5u));  
        if (step == 100u) break;
        step = (u16)(step + 1u);
    }

    do {
        cur_strip_off = X_Y_TO_OFFSET(girl_x_px, girl_y_px);
        frame_counter = (u16)(frame_counter + 1u);

        // phase == 10: копируем кусочек с фона повыше (имитация открытия плашки буквы) и печатаем букву в OFFSCR
        if (phase == PHASE_PRINT) {
            // Гидра: ega_vram_move_blocks(2,0x13, maybescoreSlotOffset+0x640, maybescoreSlotOffset)
            // Смысл: 19 строк по 2 байта: src = slot, dst = slot+1600
            // 1600 = 0x640
            ega_vram_move_blocks(
                (u16)score_slot_off_bytes,
                (u16)(score_slot_off_bytes + X_Y_TO_OFFSET(0, 20)),
                (u16)19u,
                (u16)2u
            );

            (void)cstr_char_to_str_buf(letter_s, (char)g_current_letter);

            print_text(
                gRes.fonts.f14,
                EGA_OFFSCREEN_BASE,
                PRINT_ADV_PIX,
                PRINT_GH_M1,
                PRINT_COLOR,
                PRINT_Y_PX,
                print_x_px,
                letter_s
            );
        }

        // Гидра: ega_vram_move_blocks(6,0x5a, fixedVRAMOffset, currentScoreStripOffs+32000)
        // Смысл: сохранить фон-полоску из OFFSCR в фиксированный буфер (BB80)
        ega_vram_move_blocks(
            (u16)(EGA_OFFSCREEN_BASE + cur_strip_off),
            (u16)fixed_off_bytes,
            (u16)STRIP_ROWS,
            (u16)STRIP_WBYTES
        );
 
        // выбор спрайта и шага, когда phase==0 
        // seg000:17C9                   loc_117C9:                              ; CODE XREF: girl_and_score_run+128↑j
 
        if (phase < 1u) {
            unsigned m = (u16)(frame_counter & 3u);  // магическая золотая последовательность кадров девушки
            if (m == 0u)      { girl_sprite_res = RES_GFX_GIRL_WALK_SPRITE_1; step = 10u; }
            else if (m == 1u) { girl_sprite_res = RES_GFX_GIRL_WALK_SPRITE_3; step = 0u;  }
            else if (m == 2u) { girl_sprite_res = RES_GFX_GIRL_WALK_SPRITE_2; step = 12u; }
            else              { girl_sprite_res = RES_GFX_GIRL_WALK_SPRITE_3; step = 3u;  }
        } else {
            // Гидра ставит 0x17
            girl_sprite_res = RES_GFX_GIRL_WALK_SPRITE_0;
        }

        // phase: если >0, то ++; если >20, то 0
        if (phase > 0u) phase = (u16)(phase + 1u);
        if (phase > PHASE_MAX) phase = 0;

        // рисуем девочку в OFFSCR
        draw_rle_packed_sprite(
            2u,
            girl_y_px,
            girl_x_px,
            PTR_VRAM_7D00_OFFSCREEN,
            (const u8 far*)GET_GFX_PTR(girl_sprite_res)
        );

        // восстановление полосы на экране: текущая и предыдущая позиции
        ega_vram_move_blocks(
            (u16)(EGA_OFFSCREEN_BASE + cur_strip_off),
            (u16)cur_strip_off,
            (u16)STRIP_ROWS,
            (u16)STRIP_WBYTES
        );

        ega_vram_move_blocks(
            (u16)(EGA_OFFSCREEN_BASE + prev_strip_off),
            (u16)prev_strip_off,
            (u16)STRIP_ROWS,
            (u16)STRIP_WBYTES
        );

        prev_strip_off = cur_strip_off;

        // случайный писк на phase 0 
        if (phase == 0u) {
            u16 f = (u16)((rand() % 100) + 1000);  
            f = (u16)(f * g_soundONOF_soundMultiplier);
            tp_sound(f);
            
        }

        // вернуть сохранённый фон обратно в OFFSCR  
        ega_vram_move_blocks(
            (u16)fixed_off_bytes,
            (u16)(EGA_OFFSCREEN_BASE + prev_strip_off),
            (u16)STRIP_ROWS,
            (u16)STRIP_WBYTES
        );

        tp_nosound();

        // движение только когда phase 0
        if (phase == 0u) {
            girl_x_px = (u16)(girl_x_px + step);
        }

        // переход к следующей букве, когда девочка прошла целевой X
        if (letter_x_px < girl_x_px) {
            u16 found_pos = found_pos_1based(current_idx1);

            // Гидра: maybescoreSlotOffset = found[idx]*2 + 0x81C9
            // 0x81C9 = вычисленное значение оффсета по координатам (15,200) в offscreen
            score_slot_off_bytes = (u16)((found_pos * SLOT_XBYTES) + SLOTS_START_OFFSET_OFFSCR);

            print_x_px = calc_letter_print_x_px(found_pos);

            current_idx1 = (u16)(current_idx1 + 1u);

            if (g_word_state.match_count < current_idx1) {
                letter_x_px = (u16)1000; // идем за экран
            } else {
                letter_x_px = calc_letter_x_px(found_pos_1based(current_idx1));
            }

            phase = 1u;
        }

        pit_delay_ms(50u);
        girl_poll_keys();

    } while (girl_x_px < LOOP_X_LIMIT_PX);

    // финальная очистка полосы на экране (чуть больше)
    ega_vram_move_blocks(
        (u16)(EGA_OFFSCREEN_BASE + cur_strip_off),
        (u16)cur_strip_off,
        (u16)FINAL_ROWS,
        (u16)FINAL_WBYTES
    );

    // затухание звука: 100 - 20
    step = 100u;
   
    for (;;) {
      
        tp_sound((u16)(g_soundONOF_soundMultiplier * step));

        pit_delay_ms((u16)((100u - step) / 10u));
        tp_nosound();
        pit_delay_ms(1u);
        if (step == 20u) break;
        step = (u16)(step - 1u);
    }

    // начисление очков: g_score_table[0] 
    if (g_score_table[0] < 100) { // меньше ста лежат очки
        g_score_table[g_active_player] =
            (s16)(g_score_table[g_active_player] +
                  (s16)(g_score_table[0] * (u16)g_word_state.match_count));
    } else if (g_score_table[0] == BARABAN_MULT_X2 || g_score_table[0] == BARABAN_MULT_X4) {
        int mult = (int)g_score_table[0] - BARABAN_MULT_BASE; // 2 или 4
        g_score_table[g_active_player] = (s16)(mult * (int)g_score_table[g_active_player]);
    }
    // мы тут можем оказаться ксттаи если сектор приз и мы выбрали показывать букву - тогда в score_table 300
    // и ничего нам за это не дадут
    
    player_draw_score(g_active_player);
 
}
