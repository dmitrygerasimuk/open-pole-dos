#include "ui_hof.h"

#include <string.h>
#include <stdio.h>
#include <dos.h>
#include <conio.h>

#include "ui_keymap.h"
#include "globals.h"
#include "resources.h"
#include "debuglog.h"
#include "ega_text.h"
#include "ega_draw.h"
#include "sys_delay.h"
#include "rle_draw.h"
#include "sound.h"
#include "input.h"
#include "boss_mode.h"
#include "end_screen.h"
#include "yakubovich.h"
#include "pole_dialog.h"
#include "res_gfx.h"



static inline u16 ui_name_row_y(u16 slot)
{
    return (u16)(UI_NAME_Y_BASE + (u16)((slot + 1u) * UI_NAME_Y_STEP));
}

static inline u16 ui_name_base_vram_off(u16 slot)
{
    u16 y = ui_name_row_y(slot);
    return (u16)((u16)(y * UI_VRAM_BYTES_PER_ROW) + UI_NAME_XBYTE_BASE);
}

static void ui_hof_store_name(u16 slot, const u8 *buf, u16 len)
{
    unsigned i;
    u16 base = (u16)(slot * UI_HOF_REC_SIZE);
    u8 *dst  = &g_hof_name_table[base];

    if (len > UI_HOF_STORE_MAXLEN) len = UI_HOF_STORE_MAXLEN;

    dst[0] = (u8)len;
    for (i = 0; i < len; ++i) dst[1u + i] = buf[i];
    for (; i < UI_HOF_STORE_MAXLEN; ++i) dst[1u + i] = 0;
    dst[1u + UI_HOF_STORE_MAXLEN] = 0;
}

static void ui_draw_single_char(u16 slot, u16 idx, u8 ch)
{
    u8 s[2];
    u16 y = ui_name_row_y(slot);

    s[0] = ch;
    s[1] = '\0';

    ega_draw_text_8xN(gRes.fonts.f14, UI_NAME_GLYPH_H,
                      (u16)(UI_NAME_X_PIX_SHAD_BASE + (u16)(idx * UI_NAME_X_ADV_PIX)),
                      y, UI_COLOR_SHADOW, UI_NAME_X_ADV_PIX, (const char*)s);

    ega_draw_text_8xN(gRes.fonts.f14, UI_NAME_GLYPH_H,
                      (u16)(UI_NAME_X_PIX_MAIN_BASE + (u16)(idx * UI_NAME_X_ADV_PIX)),
                      y, UI_COLOR_MAIN, UI_NAME_X_ADV_PIX, (const char*)s);
}

static inline void ui_clear_cell(u16 base_vram_off, u16 idx)
{
   
    ega_latch_fill((u16)(base_vram_off + idx),(u16)(UI_NAME_GLYPH_H - 1u),1u, UI_COLOR_CLEAR );
}

static inline void ui_draw_cursor(u16 base_vram_off, u16 idx, u8 on)
{
    u8 c = on ? UI_COLOR_CURSOR_ON : UI_COLOR_CLEAR;
 
    ega_latch_fill( (u16)(base_vram_off + idx + UI_CURSOR_VRAM_OFF_ADD),
                        (u16)(UI_CURSOR_H - 1u),UI_CURSOR_WBYTES,c);
}

 
void ui_input_winner_name(u16 slot)
{
    u16 base_off = ui_name_base_vram_off(slot);
    u16 len = 0, blink = 0;
    u8  name_buf[UI_NAME_MAX_CHARS];
    unsigned i;

    for (i = 0; i < UI_NAME_MAX_CHARS; ++i) name_buf[i] = 0;

    flushKeyboard();

    for (;;) {
        u8 ascii = 7u;
        u8 scan  = 0;

        
        if (input_tp_readkey_nb(&ascii, &scan)) {
            g_mPressedKey = ascii;
            input_process_game_sound_and_exit_control_keys();
        }

        if (ascii == KEY_BKSP) {
            if (len > 0) {
                ui_clear_cell(base_off, len);
                --len;
                ui_clear_cell(base_off, len);
                name_buf[len] = 0;
            }
        }

        if (ascii > 0x20u) {
            ascii = ui_map_input_letter(ascii);
            if (ascii > 0x20u) {
                if (len < UI_NAME_MAX_CHARS) {
                    ui_clear_cell(base_off, len);
                    ui_draw_single_char(slot, len, ascii);
                    name_buf[len++] = ascii;
                }
            }
        }

        pit_delay_ms(UI_BLINK_DELAY_TICKS);
        if (++blink > UI_BLINK_MAX) blink = 0;
        ui_draw_cursor(base_off, len, (u8)((blink / UI_BLINK_DIV) ? 1u : 0u));

        if (ascii == KEY_ENTER) break;
    }

    ui_hof_store_name(slot, name_buf, len);
}

static void hof_draw_table_to_offscreen(u16 new_slot)
{
    unsigned i;

  
    ega_latch_fill( HOF_PANEL_OFFS,  HOF_PANEL_ROWS , HOF_PANEL_BYTES, EGA_COLOR_BLACK );

    ega_draw_text_8xN_vramoff(gRes.fonts.f14, 14, HOF_HDR_X_SH, HOF_HDR_Y1, 0x0Fu, 8u,
                              STR_8_LUCHSHIH_IGROKOV, EGA_OFFSCREEN_BASE);
    ega_draw_text_8xN_vramoff(gRes.fonts.f14, 14, HOF_HDR_X_MN, HOF_HDR_Y1, 0x07u, 8u,
                              STR_8_LUCHSHIH_IGROKOV, EGA_OFFSCREEN_BASE);

    ega_draw_text_8xN_vramoff(gRes.fonts.f14, 14, HOF_HDR_X_SH, HOF_HDR_Y2, 0x0Fu, 8u,
                              STR_VIIGR_FINAL, EGA_OFFSCREEN_BASE);
    ega_draw_text_8xN_vramoff(gRes.fonts.f14, 14, HOF_HDR_X_MN, HOF_HDR_Y2, 0x07u, 8u,
                              STR_VIIGR_FINAL, EGA_OFFSCREEN_BASE);

    for (i = 0; i < HOF_COUNT; ++i) {
        u16 y = (u16)(HOF_ROW_Y_BASE + (u16)((i + 1u) * HOF_ROW_Y_STEP));

        u8 main = (u8)((new_slot != 0xFFFFu && i == new_slot) ? 0x0Fu : 0x0Bu);
        u8 shad = (u8)(main - 8u);

        // Name
        {
            char name[HOF_NAME_MAX + 1];
            u8 nlen = g_hof_name_table[i * HOF_REC_SIZE + 0];
            if (nlen > (u8)HOF_NAME_MAX) nlen = (u8)HOF_NAME_MAX;

            {
                unsigned k;
                for (k = 0; k < (u16)nlen; ++k)
                    name[k] = (char)g_hof_name_table[i * HOF_REC_SIZE + 1u + k];
                name[nlen] = '\0';
            }

            //имя
            {
                char line[32];
                u16 p = 0;
                line[p++] = (char)('1' + (char)i);
                line[p++] = ' ';
                {
                    u16 k = 0;
                    while (name[k] && p < (u16)(sizeof(line) - 1u)) line[p++] = name[k++];
                }
                line[p] = '\0';

                ega_draw_text_8xN_vramoff(gRes.fonts.f14, 14, HOF_NAME_X_SH, y, shad, 8u, line, EGA_OFFSCREEN_BASE);
                ega_draw_text_8xN_vramoff(gRes.fonts.f14, 14, HOF_NAME_X_MN, y, main, 8u, line, EGA_OFFSCREEN_BASE);
            }
        }

        //  деньги 
        {
            u16 sc = (u16)g_hof_name_table[i * HOF_REC_SIZE + HOF_SCORE_OFF]
                   | (u16)((u16)g_hof_name_table[i * HOF_REC_SIZE + HOF_SCORE_OFF + 1u] << 8);

            char sscore[16];
            sprintf(sscore, "%u$", sc);

            ega_draw_text_8xN_vramoff(gRes.fonts.f14, 14, HOF_SCORE_X_SH, y, shad, 8u, sscore, EGA_OFFSCREEN_BASE);
            ega_draw_text_8xN_vramoff(gRes.fonts.f14, 14, HOF_SCORE_X_MN, y, main, 8u, sscore, EGA_OFFSCREEN_BASE);
        }
    }
}

static void hof_slide_in(void)
{
    s16 v = HOF_WIPE_VMAX;

    for (;;) {
        s16 tone;
        u16 dst;
        u16 maxRow;

        tone = (s16)(0x0640 - (s16)(v * 0x0014));
        tone = (s16)(tone * (s16)g_soundONOF_soundMultiplier);
        tp_sound((u16)tone);

        dst    = (u16)(HOF_WIPE_DST_BASE + (u16)((u16)v * HOF_WIPE_STRIDE));
        maxRow = (u16)(HOF_WIPE_MAXROW_BASE - (u16)((u16)v << 1));

        ega_vram_move_blocks(HOF_WIPE_SRC_BASE, dst, maxRow, HOF_WIPE_COUNTBYTES);

        tp_nosound();
        pit_delay_ms((u16)v);

        if (v == 0) break;
        --v;
    }
}

static void hof_save_pic_to_disk(void)
{
    FILE *f = fopen("pole.pic", "wb");
    if (f) {
        fwrite(g_hof_name_table, HOF_REC_SIZE, HOF_COUNT, f);
        fclose(f);
    }
}

void ui_hall_of_fame_routine(void)
{
    unsigned i;

     
  
    ega_clear_screen_fast(EGA_COLOR_LIGHT_GRAY);

    draw_rle_packed_sprite(RES_GFX_TRANSPARENT_GRAY, 0x000Au, 0x000Au, PTR_VRAM_A000,
                           (const u8 far*)GET_GFX_PTR(RES_GFX_TITLE_POLE));
    draw_rle_packed_sprite(RES_GFX_TRANSPARENT_GRAY, 0x000Au, 0x00C8u, PTR_VRAM_A000,
                           (const u8 far*)GET_GFX_PTR(RES_GFX_TITLE_CHUDES));
    draw_rle_packed_sprite(RES_GFX_TRANSPARENT_GRAY, 0x00ACu, 0x01E0u, PTR_VRAM_A000,
                           (const u8 far*)GET_GFX_PTR(RES_GFX_YAKUBOVICH_152x152));
    draw_rle_packed_sprite(RES_GFX_NO_TRANSPARENCY,  0x00ADu, 0x01FFu, PTR_VRAM_A000,
                           (const u8 far*)GET_GFX_PTR(RES_YAKUBOVICH_FACE_MOUTH_CLOSE));
    draw_rle_packed_sprite(RES_GFX_NO_TRANSPARENCY,  0x00D1u, 0x0214u, PTR_VRAM_A000,
                           (const u8 far*)GET_GFX_PTR(RES_YAKUBOVICH_ONLY_EYES_OPEN));

    end_screen_print_footer();
    yakubovich_talks(YT_FINAL_WIN);
    loop_wait_and_get_keys(0);
    yak_anim_close_mouth();

    {
        u16 new_slot = 0xFFFFu;
        u16 my = (u16)g_score_table[3];

        u16 lo = (u16)g_hof_name_table[(u16)(7u * HOF_REC_SIZE + HOF_SCORE_OFF)]
               | (u16)((u16)g_hof_name_table[(u16)(7u * HOF_REC_SIZE + HOF_SCORE_OFF + 1u)] << 8);

        if (my > lo) {
            u16 pos = 7u;

            while (pos > 0) {
                u16 prev = (u16)g_hof_name_table[(u16)((pos - 1u) * HOF_REC_SIZE + HOF_SCORE_OFF)]
                         | (u16)((u16)g_hof_name_table[(u16)((pos - 1u) * HOF_REC_SIZE + HOF_SCORE_OFF + 1u)] << 8);
                if (my <= prev) break;
                --pos;
            }

            new_slot = pos;

            for (i = 7u; i > new_slot; --i) {
                memcpy(&g_hof_name_table[i * HOF_REC_SIZE],
                       &g_hof_name_table[(i - 1u) * HOF_REC_SIZE],
                       HOF_REC_SIZE);
            }

            g_hof_name_table[new_slot * HOF_REC_SIZE + 0] = 0;
            for (i = 0; i < HOF_NAME_MAX; ++i)
                g_hof_name_table[new_slot * HOF_REC_SIZE + 1u + i] = 0;

            g_hof_name_table[new_slot * HOF_REC_SIZE + HOF_SCORE_OFF]      = (u8)(my & 0xFFu);
            g_hof_name_table[new_slot * HOF_REC_SIZE + HOF_SCORE_OFF + 1u] = (u8)((my >> 8) & 0xFFu);
        }

        //  Рисуем всю таблицу в OFFSCREEN
        hof_draw_table_to_offscreen(new_slot);

        //   Выезжающая шторка
        hof_slide_in();

        //   теперь — ввод имени, если попал в топ
        if (new_slot != 0xFFFFu) {
            ui_input_winner_name(new_slot);

            // safe score  
            g_hof_name_table[new_slot * HOF_REC_SIZE + HOF_SCORE_OFF]      = (u8)(my & 0xFFu);
            g_hof_name_table[new_slot * HOF_REC_SIZE + HOF_SCORE_OFF + 1u] = (u8)((my >> 8) & 0xFFu);

            hof_save_pic_to_disk();

            // Обновим отображение без второй анимации: один moveBlocks всем прямоугольником (v=0)
            hof_draw_table_to_offscreen(new_slot);
            ega_vram_move_blocks(HOF_WIPE_SRC_BASE, HOF_WIPE_DST_BASE, HOF_WIPE_MAXROW_BASE, HOF_WIPE_COUNTBYTES);
        }

        loop_wait_and_get_keys(0);
    }
}
