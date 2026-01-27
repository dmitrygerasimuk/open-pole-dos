#include "ui_choice.h"

#include <stdio.h>
#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <i86.h>

#include "globals.h"
#include "resources.h"
#include "bgi_mouse.h"
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

 

static inline u16 centered_x_from_len(const char *s)
{
    return (u16)(240u - (u16)((u16)strlen(s) * 4u));
}

void ega_print_centered_f14(u16 y, u8 color, const char *s)
{
    // print_text()  отцентрованный 
    u16 x = centered_x_from_len(s);
    print_text(gRes.fonts.f14, VRAM_OFFSET_ONSCREEN, FONT_ADV_PIX, FONT_GH_M1_F14, color, y, x, s);
}

static void ega_print_offscreen_f14(u16 base_off, u16 x, u16 y, u8 color, const char *s)
{
    // пишем за экран
    print_text(gRes.fonts.f14, (u16)(base_off + (u16)(x / 8u)), FONT_ADV_PIX, FONT_GH_M1_F14, color, y, 0u, s);
}

void ui_draw_rle(u16 x, u16 y, u8 transparent, u16 vram_off, u16 gfx_id) // нормальный враппер под человеческий порядок аргументов
{
    draw_rle_packed_sprite(
        transparent,
        x, y,
        vram_ptr(vram_off),
        (const u8 far*)GET_GFX_PTR(gfx_id)
    );
}

 
static UiHandChoiceAction ui_draw_hand_and_wait(void)
{
    u16 uiOffs = UI_DST_BASE;
    u16 accepted = 0;
    UiHandChoiceAction side = (UiHandChoiceAction)UI_HAND_CHOSE_RIGHT_SIDE;

    if ((s16)g_mouse_present > 0) {
        bgi_mouse_set_pos(320u, 100u);
    }

    flushKeyboard();

    while (!accepted) {
     
#ifdef DBGON
        DBGL("ui_choice: draw hand frame\n");
#endif
        ega_latch_fill((u16)(uiOffs + EGA_OFFSCREEN_BASE), UI_W_BYTES_1A, UI_ROWS_M1, UI_FILL_COLOR);

        // hand sprite into A000:7D00, y = side*0x3C + 0x1E, x = 0xF0, transparent=2
        {
            u16 hand_y = (u16)((u16)side * 0x003Cu + 0x001Eu);
            ui_draw_rle(0x00F0u, hand_y, RLE_TRANSPARENT_HAND_SPRITE, EGA_OFFSCREEN_BASE, RES_GFX_HAND_SPRITE);
        }

        // moveBlocksInPTR_VRAM_A000_Scr_Dest(uiOffs+0x7D00, uiOffs, 0x1A, 0x14)
        ega_vram_move_blocks((unsigned)(uiOffs + EGA_OFFSCREEN_BASE),
                             (unsigned)uiOffs,
                             (unsigned)UI_W_BYTES_1A,
                             (unsigned)UI_W_BYTES_14);

       
        g_mPressedKey = KEY_DEFAULT;

        if (bios_kbhit()) {
            unsigned ax = bios_getkey_ax();
            u8 al = (u8)(ax & 0xFFu);          // ASCII  
            u8 ah = (u8)((ax >> 8) & 0xFFu);   // scancode

            if (al == 0u || al == 0xE0u) {
                // extended
                if (ah == KEY_LEFT || ah == KEY_RIGHT) side ^= 1u;
                g_mPressedKey = 0u;
            } else {
                g_mPressedKey = al;
            }
        }

        input_process_game_sound_and_exit_control_keys();

      
        if ((s16)g_mouse_present > 0) {
            static u16 prev_buttons = 0;
            u16 buttons = 0;
            u16 mx = 0, my = 0;

            mouse_query(&buttons, &mx, &my);

            
            g_space_pressed = ((buttons & 1u) && !(prev_buttons & 1u)) ? 1u : 0u;
            prev_buttons = buttons;

            if (mx < 0x012Cu || mx > 0x0154u) {
                side ^= 1u;
                bgi_mouse_set_pos(320u, 100u);
            }
        }

        if (g_mPressedKey == 0x20u) {
            accepted = 1u;
        } else if ((s16)g_mouse_present > 0) {
            if (g_space_pressed == 1u) accepted = 1u;
        }

        pit_delay_ms(0x001Eu);
    }

  
#ifdef DBGON
    DBGL("ui_choice: cleanup\n");
#endif
    ega_latch_fill(uiOffs, UI_W_BYTES_1A, UI_ROWS_M1, UI_FILL_COLOR);

    return side;
}

UiChoiceAction ui_choice(UiChoiceAction mode)
{
    u16 spinFrame;
    u16 ui_choiceSide;

   
#ifdef DBGON
    DBGL("ui_choice: prepare strip\n");
#endif
    ega_latch_fill(UI_STRIP_OFFS, UI_W_BYTES_50, UI_W_BYTES_14, UI_FILL_COLOR);

    ui_draw_rle(0x010Eu, 0x0002u, RLE_TRANSPARENT_HAND_SPRITE, EGA_OFFSCREEN_BASE, RES_GFX_MONEY_SNICKERS_SPRITE);
    ui_draw_rle(0x010Eu, 0x0044u, RLE_TRANSPARENT_HAND_SPRITE, EGA_OFFSCREEN_BASE, RES_GFX_MONEY_SNICKERS_SPRITE);

    // labels (into offscreen base 0x7D00, with x=0x0C or 0x12; y=0x122/0x131)
    if (mode == (UiChoiceAction)UI_ACT_MODE1_SKAZHU_KRUCHU) {
        ega_print_offscreen_f14(EGA_OFFSCREEN_BASE, 0x000Cu, 0x0122u, 0u, kVB_SkazhuKruchu);
        ega_print_offscreen_f14(EGA_OFFSCREEN_BASE, 0x000Cu, 0x0131u, 0u, kVB_SlovoBaraban);
    } else if (mode == (UiChoiceAction)UI_ACT_MODE2_PRIZ_ILI_IGRAT) {
        ega_print_offscreen_f14(EGA_OFFSCREEN_BASE, 0x0012u, 0x0122u, 0u, kVB_BeruBudu);
        ega_print_offscreen_f14(EGA_OFFSCREEN_BASE, 0x0012u, 0x0131u, 0u, kVB_PrizIgrat);
    } else if (mode == (UiChoiceAction)UI_ACT_MODE3_PRIZ_ILI_DENGI) {
        ega_print_offscreen_f14(EGA_OFFSCREEN_BASE, 0x0012u, 0x0122u, 0u, kVB_BeruBeru);
        ega_print_offscreen_f14(EGA_OFFSCREEN_BASE, 0x0012u, 0x0131u, 0u, kVB_PrizDengi);
    }

    // spin
    spinFrame = 0x0028u;
    while (spinFrame != 0u) {
        // freq = (1000 - spinFrame*20) * soundMultiplier
        {
            u16 base = (u16)(0x03E8u - (u16)(spinFrame * 0x0014u));
            u16 freq = (u16)(base * g_soundONOF_soundMultiplier);
            tp_sound(freq);
        }

        // moveBlocksInPTR_VRAM_A000_Scr_Dest(0xC800, 0x4B00 + (spinFrame*2)*0x50, 0x50 - spinFrame*2, 0x14)
        {
            u16 dest   = (u16)(UI_DST_BASE + (u16)((spinFrame << 1) * UI_W_BYTES_50));
            u16 maxRow = (u16)(UI_W_BYTES_50 - (u16)(spinFrame << 1));

            ega_vram_move_blocks((unsigned)UI_STRIP_OFFS,
                                 (unsigned)dest,
                                 (unsigned)maxRow,
                                 (unsigned)UI_W_BYTES_14);
        }

        tp_nosound();
        pit_delay_ms(spinFrame);

        --spinFrame;
    }

    // нарисовать руку и ждать
    ui_choiceSide = (u16)ui_draw_hand_and_wait();

    // clear 0x4B00 block
#ifdef DBGON
    DBGL("ui_choice: clear dst\n");
#endif
    ega_latch_fill(UI_DST_BASE, UI_W_BYTES_50, UI_W_BYTES_14, UI_FILL_COLOR);

    // if ui_choiceSide==1 => return 0 else return mode
    if (ui_choiceSide == UI_HAND_CHOSE_RIGHT_SIDE) return UI_ACT_CONTINUE;
    return mode;
}
