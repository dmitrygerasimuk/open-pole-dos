
// opponents.c (C89) - draw_opponents(),   Ghidra + IDA disasm
// рисуем компьютерных оппонентов
#include "opponents.h"

#include <string.h> // strncpy

#include "globals.h"
#include "resources.h"
#include "res_gfx.h"
#include "ega_draw.h"
#include "ega_text.h"
#include "rle_draw.h"
#include "sys_delay.h"
#include "sound.h"
#include "yakubovich.h"
#include "input.h"
#include "boss_mode.h"
#include "pole_dialog.h"

// -----------------------------
 

#define GUY_FILL_COLOR          7u
#define GUY_FILL_ROWS           10u        // 0x0A
#define GUY_FILL_WBYTES         0x5Au      // bytes  

#define UI_ADVANCE_PX           8u
#define UI_GLYPH_H_M1_F14       13u        // 14px font => 13 
#define UI_TEXT_COLOR           0u

// базовые кооридинаты игроков 
#define P1_BASE_X_PX            384u
#define P1_BASE_Y               149u
#define P2_BASE_X_PX            80u
#define P2_BASE_Y               149u
#define P3_BASE_X_PX            240u
#define P3_BASE_Y               220u

#define P1_BASE_XBYTE           X_TO_XBYTE(P1_BASE_X_PX)
#define P2_BASE_XBYTE           X_TO_XBYTE(P2_BASE_X_PX)
#define P3_BASE_XBYTE           X_TO_XBYTE(P3_BASE_X_PX)

// смещения для текста 
#define HUMAN_LINE1_DY          72u        // 0x48
#define HUMAN_LINE2_DY          86u        // 0x56
#define HUMAN_LINE2_DX          24u        // 0x18  
#define HUMAN_TEXT_XBYTE_ADD    9u

#define OPP_LINE1_DY_NEG        29u        // 0x1D
#define OPP_LINE2_DY_NEG        16u        // 0x10
#define OPP_LINE2_DX            4u         //  

// -----------------------------
 

static u16 pick_profile_id_for_guy(u16 guy_index)
{
    // guy_index: 0..2, 2 - человек
    if (guy_index < 2u) {
        //   1-based  g_player_shuffle.
        // idx = roundWins*2 + 1 + guy_index  => (1..2), (3..4), (5..6)
        u16 idx = (u16)((g_round_wins << 1) + 1u + guy_index);
        return g_player_shuffle[idx];
    }
    return 0u; // human
}

static void pick_base_pos_for_guy(u16 guy_index, u16 *xByte, u16 *y)
{
    if (guy_index == 0u) { *xByte = P1_BASE_XBYTE; *y = P1_BASE_Y; return; }
    if (guy_index == 1u) { *xByte = P2_BASE_XBYTE; *y = P2_BASE_Y; return; }
    *xByte = P3_BASE_XBYTE; *y = P3_BASE_Y; // human
}

static u16 opponent_sprite_res_from_profile(u16 profile_id)
{
    if (profile_id >= 1u && profile_id <= g_opponents_count()) {
        return g_opponents[profile_id - 1u].sprite_id_word;
    }
    return 0u;
}

static const char *opponent_name_from_profile(u16 profile_id)
{
    if (profile_id >= 1u && profile_id <= g_opponents_count()) {
        return g_opponents[profile_id - 1u].name;
    }
    return "";
}

static void draw_guy_frame_into_offscreen(u16 anim_phase, u16 profile_id, u16 base_x_byte, u16 base_y)
{
    u16 sprId;

    // 0 1 2 пузыри 3 картинка персонажа
    if (anim_phase < 3u) {
        sprId = (u16)(RES_GFX_GUY_ANIM0 + anim_phase);
    } else {
      
        if (profile_id == 0u) return; // человека не рисуем
        sprId = opponent_sprite_res_from_profile(profile_id);
        if (sprId == 0u) return;
    }

    draw_rle_packed_sprite(
        2u,                         // transparency mask (as in original flow)
        base_y,
        (u16)(base_x_byte << 3),       // xByte -> pixels
        PTR_VRAM_7D00_OFFSCREEN,
        (const u8 far*)GET_GFX_PTR(sprId)
    );
}

static void beep_block(u16 anim_phase)
{
    u16 step = 1u;

    for (;;) {
        // tone = anim_phase*100 + step*10 + 50 
        u16 tone = (u16)((anim_phase * 100u) + (step * 10u) + 50u);
        tone = (u16)(tone * g_soundONOF_soundMultiplier);

        tp_sound(tone);
        pit_delay_ms(1u);
        tp_nosound();

        {
            u16 q = (u16)(step / 4u);
            u16 del = (u16)((anim_phase << 2) + q);
            pit_delay_ms(del);
        }

        if (step == 20u) break;
        ++step;
    }
}

static void build_player_label(u16 guy_index)
{
    const char *src;

    if (guy_index == 0u) src = str_Perviy_Igrok;
    else if (guy_index == 1u) src = str_Vtoroy_Igrok;
    else src = str_TretiyIgrok;

    strncpy((char*)g_buffer_string, src, sizeof(g_buffer_string) - 1u);
    g_buffer_string[sizeof(g_buffer_string) - 1u] = 0;
}

static void print_human_label(u16 base_x_byte, u16 base_y)
{
    u16 x = (u16)((base_x_byte + HUMAN_TEXT_XBYTE_ADD) * 8);

    print_text(gRes.fonts.f14, 0u, UI_ADVANCE_PX, UI_GLYPH_H_M1_F14,   // Третий игрок
               UI_TEXT_COLOR, (u16)(base_y + HUMAN_LINE1_DY), x,
               (const char*)g_buffer_string);

    print_text(gRes.fonts.f14, 0u, UI_ADVANCE_PX, UI_GLYPH_H_M1_F14,  // ЭТО ВЫ!
               UI_TEXT_COLOR, (u16)(base_y + HUMAN_LINE2_DY),
               (u16)(x + HUMAN_LINE2_DX), str_Eto_Vy);
}

static void print_opponent_labels(u16 base_x_byte, u16 base_y, u16 profile_id)
{
    const char *name = opponent_name_from_profile(profile_id);

    print_text(gRes.fonts.f14, 0u, UI_ADVANCE_PX, UI_GLYPH_H_M1_F14,
               UI_TEXT_COLOR, (u16)(base_y - OPP_LINE1_DY_NEG),
               (u16)(base_x_byte << 3), (const char*)g_buffer_string);

    print_text(gRes.fonts.f14, 0u, UI_ADVANCE_PX, UI_GLYPH_H_M1_F14,
               UI_TEXT_COLOR, (u16)(base_y - OPP_LINE2_DY_NEG),
               (u16)((base_x_byte << 3) + OPP_LINE2_DX), name);
}

 
//seg000:53B3                   draw_opponents proc near 

void draw_opponents(void)
{
    u16 guy_index;

    yakubovich_talks((YakTalkId)YT_INTRO_PARTICIPANTS);

    for (guy_index = 0u; guy_index <= 2u; ++guy_index) {
        u16 profile_id = pick_profile_id_for_guy(guy_index);

        u16 base_x_byte = 0u;
        u16 base_y     = 0u;
        pick_base_pos_for_guy(guy_index, &base_x_byte, &base_y);

        // Animation phases: 0..3
        {
            u16 anim_phase;

            for (anim_phase = 0u; anim_phase <= 3u; ++anim_phase) {
                u16 screen_offset  =    (u16)(base_x_byte + (u16)(base_y * EGA_BPL));
             
                u16 vram_offscreen_addr = (u16)(EGA_OFFSCREEN_BASE + screen_offset);

                // стираем фон за экраном
                ega_latch_fill(vram_offscreen_addr, GUY_FILL_WBYTES, GUY_FILL_ROWS, GUY_FILL_COLOR);

                
                draw_guy_frame_into_offscreen(anim_phase, profile_id, base_x_byte, base_y);

                //   offscreen -> onscreen   только для компьютерных игроков
                if (guy_index < 2u) {
                    ega_vram_move_blocks(
                        (u16)(EGA_OFFSCREEN_BASE + screen_offset),
                        screen_offset,
                        GUY_FILL_WBYTES,
                        GUY_FILL_ROWS
                    );
                }

                beep_block(anim_phase);
            }
        }

        build_player_label(guy_index);

        if (profile_id == 0u) {
            print_human_label(base_x_byte, base_y);
        } else {
            print_opponent_labels(base_x_byte, base_y, profile_id);
        }
    }

    loop_wait_and_get_keys(0u);
    yak_anim_close_mouth();
}
