// boss_mode.c  - выкручиваем палитру в ноль

#include "boss_mode.h"

#include "typedefs.h"
#include "globals.h"
#include "input.h"
#include "bgi_palette.h"
#include "debuglog.h"

#include <i86.h>   // int86

static void boss_mouse_poll_lmb(void)
{
    union REGS r;

    // int33h fn 03h: position/buttons
    r.x.ax = 0x0003;
    int86(0x33, &r, &r);

    // BX bit0 = left button
    if ((r.x.bx & 1) != 0) {
        g_space_pressed = 1u;
    }
}

static void boss_blackout_now(void)
{
    u8 i;

    // slots 1..15 -> black
    for (i = 1u; i < 16u; ++i) {
        bgi_palette_set_slot((u16)i, 0u);
    }
    bgi_palette_set_overscan(0u);
}

void boss_mode(void)
{
    DBG("boss_mode: enter\n");

 
    boss_blackout_now();

    //  чтобы не выйти мгновенно из-за старого нажатия
    flushKeyboard();

 
    for (;;) {
        g_space_pressed = 0u;

        // мышь опциональна
        if ((s16)g_mouse_present > 0) {
            boss_mouse_poll_lmb();
        }

       
        g_mPressedKey = KEY_DEFAULT;

        if (bios_kbhit()) {
            u16 ax = (u16)bios_getkey_ax();
            u8  al = (u8)(ax & 0x00FFu);

            // расширенные клавиши пропускаем
            if (al == 0u || al == 0xE0u) {
                (void)bios_getkey_ax();
                al = 0u;
            }
            g_mPressedKey = al;
        }

        if (g_mPressedKey == KEY_ESC) {
            exit_game();
        }

        if (g_mPressedKey == KEY_CTRL_S) {
            g_soundONOF_soundMultiplier = (u16)(1u - g_soundONOF_soundMultiplier);
        }

        // условие выхода
        if (g_mPressedKey == KEY_SPACE || g_space_pressed == 1u) {
            break;
        }
    }

    // возвращаем палитру
    bgi_palette_apply_startup();

    DBG("boss_mode: exit (palette restored to startup)\n");
}
