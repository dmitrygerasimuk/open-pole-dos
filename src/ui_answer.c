// ui_answer.c
// Рисуем инпут для ввода ответа и проверяем

#include "ui_answer.h"

#include <dos.h>
#include <stdio.h>
#include <string.h>

#include "ui_keymap.h"
#include "globals.h"
#include "resources.h"
#include "ega_text.h"
#include "ega_draw.h"
#include "sys_delay.h"
#include "cstr.h"
#include "input.h"
#include "boss_mode.h"
#include "sound.h"

// C string правильного ответа
extern char g_current_game_word[64];
extern char g_answer_entered[21];

// X/Y в define — десятичные, оффсеты VRAM — hex

#define UI_ANS_BOX_Y1            300u
#define UI_ANS_BOX_Y2            320u
#define UI_ANS_TEXT_Y            303u

#define UI_ANS_CELL_STEP_PX      16u
#define UI_ANS_BOX_X1_ADD        4u
#define UI_ANS_BOX_X2_ADD        20u
#define UI_ANS_TEXT_X_ADD        8u

#define UI_ANS_VRAM_BASE_OFF     ((u16)0x5EB1u)
#define UI_ANS_CURSOR_ADD        800u

#define UI_ANS_ADV_PX            8u
#define UI_ANS_GLYPH_HM1         13u

#define UI_ANS_MAXLEN            20u

#define UI_ANS_COLOR_TEXT        0u
#define UI_ANS_COLOR_BG          7u

#define UI_ANS_BLINK_MAX         39u
#define UI_ANS_BLINK_DIV         20u
#define UI_ANS_DELAY_TICKS       25u

 

static void ui_ans_clear_cell(u16 idx)
{
    u16 off = (u16)(UI_ANS_VRAM_BASE_OFF + (u16)(idx << 1));
  
    ega_latch_fill(off, UI_ANS_GLYPH_HM1, 1,  UI_ANS_COLOR_BG);
    
}

static void ui_ans_draw_cursor(u16 idx, u8 on)
{
    u16 off = (u16)(UI_ANS_VRAM_BASE_OFF + (u16)(idx << 1) + UI_ANS_CURSOR_ADD);
   
    ega_latch_fill(off, 1u, 1u, on ? UI_ANS_COLOR_BG : 0u );
    
}

static void ui_ans_draw_char(u16 idx, u8 ch)
{
    char tmp[2];
    u16 x = (u16)((u16)(idx * UI_ANS_CELL_STEP_PX) + UI_ANS_TEXT_X_ADD);

    print_text(gRes.fonts.f14, 0u, UI_ANS_ADV_PX, UI_ANS_GLYPH_HM1,
               UI_ANS_COLOR_TEXT, UI_ANS_TEXT_Y, x,
               cstr_char_to_str_buf(tmp, (char)ch));
}

static void ui_ans_draw_boxes(u16 answer_len)
{
    unsigned i;
   

    if (answer_len == 0u) return;

    for (i = 0; i < answer_len; ++i) {
        int x1 = (int)((u16)(i * UI_ANS_CELL_STEP_PX) + UI_ANS_BOX_X1_ADD);
        int x2 = (int)((u16)(i * UI_ANS_CELL_STEP_PX) + UI_ANS_BOX_X2_ADD);

        ega_wm2_bar(PTR_VRAM_A000, x1, (int)UI_ANS_BOX_Y1, x2, (int)UI_ANS_BOX_Y2, UI_ANS_COLOR_BG);
        ega_wm2_rectangle(PTR_VRAM_A000, x1, UI_ANS_BOX_Y1, x2, UI_ANS_BOX_Y2, 0u);
    }
}

static u16 ui_ans_is_equal(const char *a, const char *b)
{
    if (!a || !b) return 0u;
    return (strncmp(a, b, (size_t)UI_ANS_MAXLEN + 1u) == 0) ? 1u : 0u;
}

u16 ui_enter_answer_word(void)
{
    u16 answer_len = G_WORDLEN;
    u16 typed_len  = 0u;
    u16 blink      = 0u;

    char typed[UI_ANS_MAXLEN + 1u];
    typed[0] = '\0';
    g_answer_entered[0] = '\0';

    // Рамки
    ui_ans_draw_boxes(answer_len);

    flushKeyboard();

    for (;;) {
        u8 ascii = KEY_DEFAULT;
        u8 scan  = 0u;

        if (input_tp_readkey_nb(&ascii, &scan)) {
            g_mPressedKey = ascii;
            input_process_game_sound_and_exit_control_keys();
        }

        if (ascii == KEY_BKSP) {
            if (typed_len > 0u) {
                ui_ans_clear_cell(typed_len);
                --typed_len;
                ui_ans_clear_cell(typed_len);
                typed[typed_len] = '\0';
            }
        }

        if (ascii > 0x20u) {
            ascii = ui_map_input_letter(ascii);
            if (ascii > 0x20u) {
                if (typed_len < answer_len && typed_len < UI_ANS_MAXLEN) {
                    ui_ans_clear_cell(typed_len);
                    ui_ans_draw_char(typed_len, ascii);
                    typed[typed_len++] = (char)ascii;
                    typed[typed_len] = '\0';
                }
            }
        }

        pit_delay_ms(UI_ANS_DELAY_TICKS);
        if (++blink > UI_ANS_BLINK_MAX) blink = 0u;
        ui_ans_draw_cursor(typed_len, (u8)((blink / UI_ANS_BLINK_DIV) ? 1u : 0u));

        if (ascii == KEY_ENTER) break;
    }

    // Сохранить введённое
    {
        unsigned i;
        for (i = 0; i < typed_len && i < UI_ANS_MAXLEN; ++i) g_answer_entered[i] = typed[i];
        g_answer_entered[i] = '\0';
    }

    return ui_ans_is_equal(g_answer_entered, g_current_game_word) ? 0u : 1u;
}
