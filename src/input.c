// input.c 

#include <dos.h>
#include <conio.h>
#include <i86.h>
#include <stdlib.h>
#include <stdio.h>

#include "input.h"
#include "sys_delay.h"
#include "resources.h"
#include "debuglog.h"
#include "ega_draw.h"
#include "boss_mode.h"
#include "ega_text.h"
#include "bgi_palette.h"
#include "sound.h"
#include "globals.h"
#include "yakubovich.h"

 
 

void exit_game(void);
void bossMode(void);

void mouse_query(u16 *out_buttons, u16 *out_x, u16 *out_y)
{
    union REGS r;
    r.x.ax = 0x0003;
    int86(0x33, &r, &r);

    if (out_buttons) *out_buttons = (u16)r.x.bx;
    if (out_x)       *out_x       = (u16)r.x.cx;
    if (out_y)       *out_y       = (u16)r.x.dx;
}

// Turbo Pascal ReadKey()-like
u8 input_tp_readkey_nb(u8 *out_ascii, u8 *out_scan)
{
    u16 ax;
    u8  al, ah;

    if (!bios_kbhit()) return 0u;

    ax = bios_getkey_ax();
    al = (u8)(ax & 0xFFu);
    ah = (u8)((ax >> 8) & 0xFFu);

    if (al == 0u || al == 0xE0u) {
        *out_ascii = 0u;
        *out_scan  = ah;
    } else {
        *out_ascii = al;
        *out_scan  = 0u;
    }
    return 1u;
}

// returns 1 if keystroke available
int bios_kbhit(void)
{
    int has = 0;

    _asm {
        mov ah, 01h
        int 16h
        jz  no_key
        mov has, 1
        jmp done
    no_key:
        mov has, 0
    done:
    }

    return has;
}

// returns AX: AL=ascii, AH=scancode
u16 bios_getkey_ax(void)
{
    union REGS r;
    r.h.ah = 0x00; // INT 16h, AH=00h: read
    int86(0x16, &r, &r);
    return r.x.ax;
}

// flush all pending keystrokes from BIOS keyboard buffer (INT 16h)
void bios_flushkeyboard(void)
{
    _asm {
    flush_loop:
        mov ah, 01h        // check
        int 16h
        jz  done           // ZF=1 -> empty
        mov ah, 00h        // read (consume)
        int 16h
        jmp flush_loop
    done:
    }
}

void bossMode(void)
{
    //ega_draw_text_8xN(gRes.fonts.f14, 14, 20, 20, 2, 8, "BOSS MODE IS ON");
    ega_wm2_bar(MK_FP(0xA000, 0), 20, 20, 200, 30, 0);
    ega_draw_text_8xN(gRes.fonts.f8, 8, 20, 20, 2, 8, g_current_game_word);

    //boss_mode();
}

void exit_game(void)
{
    tp_nosound();
    freeResources(&gRes);
    dbg_set_graphics(0);
    text_mode_restore();

    EVENT("exit_game\n");
    DBG("exit_game: flush gfx buffer\n");
    printf("Seed for the game was: %u\n", (unsigned)g_seed);
 
 
    dbg_flush();
    dbg_free();
  
    exit(0);
}

void poll_mouse_buttons(void)
{
    union REGS r;
    r.x.ax = 0x0003; // int33 AX=3 -> BX buttons
    int86(0x33, &r, &r);

    g_mouse_buttons = (u16)r.x.bx;
    g_space_pressed = (g_mouse_buttons & 1u) ? 1u : 0u; // LMB as "space"
}

void flushKeyboard(void)
{
    // 1) consume one pending key like in original flow (and update global)
    if (bios_kbhit()) {
        unsigned ax = bios_getkey_ax();
        g_mPressedKey = (u8)(ax & 0xFFu); // ASCII
        // if you want scancodes: u8 sc = (u8)(ax >> 8);
    }

    g_mPressedKey = 7u;

    // 2) if mouse present, wait until buttons are released
    if ((s16)g_mouse_present > 0) {
        do {
            poll_mouse_buttons();
        } while (g_space_pressed != 0u);
    }
}

void input_process_game_sound_and_exit_control_keys(void)
{
    if (g_mPressedKey == KEY_ESC)     exit_game();
    if (g_mPressedKey == KEY_CTRL_S)  g_soundONOF_soundMultiplier = (u16)(1u - g_soundONOF_soundMultiplier);
    if (g_mPressedKey == KEY_TAB)     boss_mode();

    if (g_mPressedKey == KEY_CTRL_D) {
        ega_toggle_startaddr_hi_00_7d();
        flushKeyboard();
    }
}

void loop_wait_and_get_keys(u16 wait_time_in_50ms_chunks)
{
    flushKeyboard();

    for (;;) {
        if (wait_time_in_50ms_chunks > 0u) {
            --wait_time_in_50ms_chunks;
        }

        pit_delay_ms(50u);

        g_space_pressed = 0u;

        if ((s16)g_mouse_present > 0) {
            poll_mouse_buttons();
        }

        g_mPressedKey = 7u;

        if (bios_kbhit()) {
            unsigned ax = bios_getkey_ax();
            g_mPressedKey = (u8)(ax & 0xFFu); // ASCII
        }

        input_process_game_sound_and_exit_control_keys();

        if (g_mPressedKey == KEY_CTRL_D) { // Ctrl+D
            DBG("loop_waitAndgetKeys: Ctrl+D detected\n");
            ega_draw_text_8xN(gRes.fonts.f6, 6, 500, 20, 2, 4, g_current_game_word);
            //bgi_palette_set_pal_for_mono_screen();
            dbg_dump_globals();
            ega_toggle_startaddr_hi_00_7d();
        }

        if (g_mPressedKey == KEY_TAB) { // TAB
            DBG("loop_waitAndgetKeys: TAB detected\n");
            bossMode();
        }

        // exit by SPACE or mouse click
        if (g_mPressedKey == KEY_SPACE || g_space_pressed == 1u) break;

        // like in disasm: continue until arg0 == 1
        if (wait_time_in_50ms_chunks == 1u) break;
    }

    DBG("loop_waitAndgetKeys: end loop %u\n", (unsigned)wait_time_in_50ms_chunks);
}
