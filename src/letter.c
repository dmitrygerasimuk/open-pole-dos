// letter.c - порт pick_letter_player + sector_PLUS_open_letter  

 

#include <dos.h>      
#include <stdlib.h>   
#include <string.h>  
#include <stdio.h>    

#include "letter.h"
#include "resources.h"
#include "bgi_mouse.h"
#include "sound.h"
#include "input.h"
#include "sys_delay.h"
#include "debuglog.h"
#include "ega_draw.h"
#include "ega_text.h"
#include "pole_dialog.h"
#include "rle_draw.h"
#include "yakubovich.h"
#include "res_gfx.h"
#include "random.h"

// font for bubble "Буква X!"
#define LETTER_FONT_PTR            (gRes.fonts.f14)
#define LETTER_FONT_H_MINUS_1      13u
#define LETTER_ADVANCE_PX          8u

 
#define UI_BACKUP_OFF              0x59B0u
#define UI_BACKUP_W_PX             640u   
 
#define UI_BACKUP_H_PX             60u

// размеры бекапа для пузыря w=0x14 bytes, h=0x28 rows 
 // 20 bytes * 8
#define BUBBLE_SAVE_W_PX           160u  
#define BUBBLE_SAVE_H_PX           40u

//  w=0x18 px  h=0x1A rows)
#define HAND_BLIT_W_PX             24u
#define HAND_BLIT_H_PX             26u

// compile-time checks: widths must be divisible by 8
enum { UI_BACKUP_W_PX_DIV8_OK   = 1 / ((UI_BACKUP_W_PX   % 8u) ? 0 : 1) };
enum { BUBBLE_SAVE_W_PX_DIV8_OK = 1 / ((BUBBLE_SAVE_W_PX % 8u) ? 0 : 1) };
enum { HAND_BLIT_W_PX_DIV8_OK   = 1 / ((HAND_BLIT_W_PX   % 8u) ? 0 : 1) };

#define UI_BACKUP_WBYTES           ((u16)(UI_BACKUP_W_PX / 8u))     // 0x50
#define UI_BACKUP_HROWS            ((u16)(UI_BACKUP_H_PX))          // 0x3C

#define BUBBLE_SAVE_WBYTES         ((u16)(BUBBLE_SAVE_W_PX / 8u))   // 0x14
#define BUBBLE_SAVE_HROWS          ((u16)(BUBBLE_SAVE_H_PX))        // 0x28

#define HAND_BLIT_WBYTES           ((u16)(HAND_BLIT_W_PX / 8u))     // 3
#define HAND_BLIT_HROWS            ((u16)(HAND_BLIT_H_PX))          // 0x1A

// (x = letterIndex*0x14, y=0x13A)
#define PICK_Y                     314u
#define PICK_X_STEP_PX             20u

 
#define MOUSE_LEFT_THRESH          0x0131u
#define MOUSE_RIGHT_THRESH         0x014Fu

 
#define MOUSE_SET_X                320u
#define MOUSE_SET_Y                100u

// пузырь и текст
#define BUBBLE_SPR_Y               110u
#define BUBBLE_TEXT_Y              118u
#define BUBBLE_TEXT_X_ADD          6u

#define PLAYER_BUBBLE_X(p_)        (g_player_bubble_x[(p_)])
#define PLAYER_X(p_)               (g_score_x[(p_)])

// полоска с буквами
#define REVEAL_Y                   332u
#define REVEAL_X_STEP_PX           20u

// sector_PLUS_open_letter-local 
#define LR_BACKUP_OFF              0x0320u
#define _LR_BACKUP_WBYTES          0x0050u
#define _LR_BACKUP_H               0x003Cu

#define LR_PICK_Y                  0x000Cu
#define LR_PICK_X_BASE             0x00C8u
#define LR_PICK_X_STEP_PX          0x0010u   

#define _LR_HAND_WBYTES            0x0003u
#define _LR_HAND_H                 0x001Au

// Pascal-style  врапперы
#define OPENED_GET(pos1_)          (g_word_state.opened[(u16)(pos1_) - 1u])
#define OPENED_SET(pos1_, v_)      (g_word_state.opened[(u16)(pos1_) - 1u] = (u8)(v_))

 
// UI: bubble “Буква X!”
 
static u16 bubble_vram_off_for_player(PlayerId player)
{
    // disasm: ax = playerX; idiv 8; ax += 0x2260
    u16 px = (u16)PLAYER_BUBBLE_X(player);
    return (u16)(0x2260u + (u16)(px / 8u));
}

static void show_letter_bubble(u16 letterIndex)
{
    u16 bubbleOff = bubble_vram_off_for_player(g_active_player);
    u16 bubbleXpx = (u16)PLAYER_BUBBLE_X(g_active_player);

    // сохраняем фон под пузырём в оффскрин-скретч (A000:7D00 + bubbleOff)
    ega_vram_move_blocks(
        bubbleOff,
        (u16)(EGA_OFFSCREEN_BASE + bubbleOff),
        (u16)BUBBLE_SAVE_HROWS,
        (u16)BUBBLE_SAVE_WBYTES
    );

    // рисуем спрайт пузыря
    draw_rle_packed_sprite(
        2u,
        (u16)BUBBLE_SPR_Y,
        bubbleXpx,
        PTR_VRAM_A000,
        GET_GFX_PTR(RES_GFX_TALK_BUBBLE)
    );

    // собираем строку "Буква X!"
    {
        char dst[32];
        sprintf(dst, "%s%c%s",
                STR_BUKVA,
                (char)kAlphabet32Pas[1u + letterIndex],
                STR_BUKVA_EXCLAMATION);

        // печать текста внутри пузыря
        print_text(
            (const u8 __far*)LETTER_FONT_PTR,
            0u,
            (u16)LETTER_ADVANCE_PX,
            (u16)LETTER_FONT_H_MINUS_1,
            (u8)EGA_COLOR_BLACK,
            (u16)BUBBLE_TEXT_Y,
            (u16)(bubbleXpx + (u16)BUBBLE_TEXT_X_ADD),
            (const char*)dst
        );
    }

    loop_wait_and_get_keys(0x28u);

    // восстановление фона
    ega_vram_move_blocks(
        (u16)(EGA_OFFSCREEN_BASE + bubbleOff),
        bubbleOff,
        (u16)BUBBLE_SAVE_HROWS,
        (u16)BUBBLE_SAVE_WBYTES
    );
}

// ------------------------------------------------------------
 
 

static void reveal_anim_for_letterIndex(u16 letterIndex)
{
    u16 iPos;

    for (iPos = 0u; iPos <= 4u; ++iPos) {
        u16 x = (u16)(letterIndex * (u16)REVEAL_X_STEP_PX);
        u16 y = (u16)REVEAL_Y;

        if (iPos <= 3u) {
            u16 rid;
            switch (iPos) {
                default:
                case 0u: rid = RES_GFX_REVEAL_0; break;
                case 1u: rid = RES_GFX_REVEAL_1; break;
                case 2u: rid = RES_GFX_REVEAL_2; break;
                case 3u: rid = RES_GFX_REVEAL_3; break;
            }

            draw_rle_packed_sprite(
                0x10u,
                y,
                x,
                PTR_VRAM_A000,
                GET_GFX_PTR(rid)
            );
        } else {
            // финальный кадр: заливаем прямоугольник (WM2)
            ega_gc_write(0x05u, 0x00u);
            ega_wm2_bar((u8 __far*)MK_FP(EGA_VIDEO_SEGMENT, 0),
                        (int)x, (int)y,
                        (int)(x + 0x13u), (int)(y + 0x12u),
                        (u8)EGA_COLOR_LIGHT_GRAY);
        }

        // озвучка: 10 коротких "пиков"
        {
            u16 beepStep;
            for (beepStep = 1u; beepStep <= 10u; ++beepStep) {
                u16 tone = (u16)((iPos * 0x64u) + (beepStep * 0x0Au) + 0x32u);
                tone = (u16)(tone * (u16)g_soundONOF_soundMultiplier);

                tp_sound(tone);
                pit_delay_ms(1u);
                tp_nosound();

                // DELAY((beepStep/5) + (iPos*4))
                pit_delay_ms((u16)((beepStep / 5u) + (iPos * 4u)));
            }
        }
    }
}

 
 

static u16 pick_letter_human(void)
{
    s16 letterIndex = 0;
    s16 selectionConfirmed = 0;
    s16 lastIndex = -1;

    u8 scan = 0u;

    u16 cursorXpx;
    u16 cursorY;
    u16 currOff;
    u16 prevOff;

    // бэкап большой области интерфейса
    ega_vram_move_blocks(
        (u16)UI_BACKUP_OFF,
        (u16)(EGA_OFFSCREEN_BASE + (u16)UI_BACKUP_OFF),
        (u16)UI_BACKUP_HROWS,
        (u16)UI_BACKUP_WBYTES
    );

    if ((s16)g_mouse_present > 0) {
        bgi_mouse_set_pos((u16)MOUSE_SET_X, (u16)MOUSE_SET_Y);
    }

    flushKeyboard();

    cursorY   = (u16)PICK_Y;
    cursorXpx = 0u;
    currOff   = ega_get_offset_from_xy(cursorXpx, cursorY);
    prevOff   = currOff;

    for (;;) {
        u16 buttons = 0u;
        u16 mx = 0u, my = 0u;

         
        g_mPressedKey = KEY_DEFAULT;
        scan = 0u;

        if (input_tp_readkey_nb(&g_mPressedKey, &scan)) {
            input_process_game_sound_and_exit_control_keys();

            if (g_mPressedKey == 0u) {
                if (scan == KEY_LEFT)  --letterIndex;
                if (scan == KEY_RIGHT) ++letterIndex;
            }
        }

        if ((s16)g_mouse_present > 0) {
            mouse_query(&buttons, &mx, &my);
            g_mouse_buttons = buttons;
            g_space_pressed = (buttons & 1u) ? 1u : 0u;

            if (mx < (u16)MOUSE_LEFT_THRESH) {
                --letterIndex;
                bgi_mouse_set_pos((u16)MOUSE_SET_X, (u16)MOUSE_SET_Y);
            } else if (mx > (u16)MOUSE_RIGHT_THRESH) {
                ++letterIndex;
                bgi_mouse_set_pos((u16)MOUSE_SET_X, (u16)MOUSE_SET_Y);
            }
        }

        // клампим
        if (letterIndex < 0)  letterIndex = 0;
        if (letterIndex > 31) letterIndex = 31;

        // ну совсем такой же код же 
        if (g_mPressedKey == KEY_SPACE || g_space_pressed == 1u) {
            if (g_used_letter[(u16)letterIndex] == 0u) {
                selectionConfirmed = 1;
            } else {
                tp_sound((u16)(1000u * (u16)g_soundONOF_soundMultiplier));
                pit_delay_ms(0x32u);
                tp_nosound();
            }
        }

        // рисуем
        if (letterIndex != lastIndex) {
            // убрать старую руку -  восстановить с оффскрина
            ega_vram_move_blocks(
                (u16)(EGA_OFFSCREEN_BASE + prevOff),
                prevOff,
                (u16)HAND_BLIT_HROWS,
                (u16)HAND_BLIT_WBYTES
            );

            cursorXpx = (u16)((u16)letterIndex * (u16)PICK_X_STEP_PX);
            currOff   = ega_get_offset_from_xy(cursorXpx, cursorY);

            // сохранить область под новой  рукой в оффскрин
            ega_vram_move_blocks(
                currOff,
                (u16)(EGA_OFFSCREEN_BASE + currOff),
                (u16)HAND_BLIT_HROWS,
                (u16)HAND_BLIT_WBYTES
            );

            // рисуем руку поверх
            draw_rle_packed_sprite(
                2u,
                cursorY,
                cursorXpx,
                PTR_VRAM_A000,
                GET_GFX_PTR(RES_GFX_HAND_SPRITE)
            );

            prevOff   = currOff;
            lastIndex = letterIndex;
        }

        pit_delay_ms(0x1Eu);
        if (selectionConfirmed) break;
    }

    // восстановление большой области UI
    ega_vram_move_blocks(
        (u16)(EGA_OFFSCREEN_BASE + (u16)UI_BACKUP_OFF),
        (u16)UI_BACKUP_OFF,
        (u16)UI_BACKUP_HROWS,
        (u16)UI_BACKUP_WBYTES
    );

    return (u16)letterIndex;
}

 

static u16 pick_letter_cpu(void)
{
    u16 len = (u16)G_WORDLEN;
    u16 pos;
    u16 unopened = 0u;
    u16 useCheat = 0u;

    for (pos = 1u; pos <= len; ++pos) {
        if (g_word_state.opened[pos - 1u] == 0u) ++unopened;
    }

    if (unopened == 0u) return 0xFFFFu;

   // читинг 
    if ((unopened * 2u) > len && unopened > 1u) {
        useCheat = 0u;
    } else {
        u16 r = tp_random((u16)(g_round_wins + 2u));
        useCheat = (r == 0u) ? 0u : 1u;
    }

    DBG("CPU pick_letter_cpu: len=%u unopened=%u useCheat=%u\n",
         len,  unopened,  useCheat);

    if (useCheat == 0u) {
        // случайная неиспользованная буква  
        for (;;) {
            u16 li = tp_random(32);
            if (g_used_letter[li] == 0u) return li;
        }
    }

    // чит: выбрать неоткрытую позицию слова, взять букву и найти её индекс в алфавите
    for (;;) {
        u16 p = (u16)(tp_random(len) + 1u); // 1..len
        u8  ch;
        unsigned i;

        if (g_word_state.opened[p - 1u] != 0u) continue;

        ch = (u8)g_current_game_word[p - 1u];

        for (i = 0u; i < 32u; ++i) {
            if (kAlphabet32Pas[1u + i] == ch) {
                if (g_used_letter[i] == 0u) return i;
                break;
            }
        }
    }
}

static void compute_matches_and_mark_opened(u16 letterIndex)
{
    u16 len = (u16)G_WORDLEN;
    u16 pos;

    g_current_letter = (u8)kAlphabet32Pas[1u + letterIndex];
    g_word_state.match_count = 0u;

    for (pos = 1u; pos <= len; ++pos) {
        u8 ch = (u8)g_current_game_word[pos - 1u];
        if (ch != g_current_letter) continue;
        if (g_word_state.opened[pos - 1u] != 0u) continue;

        if ((u16)g_word_state.match_count < 20u) {
            g_word_state.found[(u16)g_word_state.match_count] = pos;
        }

        ++g_word_state.match_count;
        g_word_state.opened[pos - 1u] = 1u;
    }
}

 
 

void clear_used_letters(void)
{
    unsigned i;
    for (i = 0u; i < 32u && i < (u16)(sizeof(g_used_letter) / sizeof(g_used_letter[0])); ++i) {
        g_used_letter[i] = 0u;
    }
}

void pick_letter_player(void)
{
    u16 letterIndex;

    if (g_active_player == (u16)PLAYER_HUMAN) {
        letterIndex = pick_letter_human();
    } else {
        letterIndex = pick_letter_cpu();
        if (letterIndex == 0xFFFFu) {
            DBG("WORD IS OPENED\n");
            return;
        }
        show_letter_bubble(letterIndex);
    }

    // помечаем букву использованной
    g_used_letter[letterIndex] = 1u;

    // анимация раскрытия на нижней полосе
    reveal_anim_for_letterIndex(letterIndex);

    // ищем совпадения и выставляем opened[]
    compute_matches_and_mark_opened(letterIndex);

    yakubovich_talks((YakTalkId)YT_LETTER_RESULT);
    loop_wait_and_get_keys(0x28u);
    yak_anim_close_mouth();
}

 
void sector_PLUS_open_letter(void)
{
    char msgStr[0x118u];

    u16 wordLen;
    s16 done;
    s16 selectedPos;     

    u16 beepStep;
    u16 iPos;

    u16 cursorY;
    u16 cursorX;

    u16 prevVramOff;
    u16 currVramOff;
    u16 scratchOff;

     
    //  человек выбирает позицию
   
    if (g_active_player == (u16)PLAYER_HUMAN) {
        currVramOff = (u16)LR_BACKUP_OFF;

        // сохранить область: src=0x0320 -> dst=7D00+0x0320
        ega_vram_move_blocks(
            currVramOff,
            (u16)(EGA_OFFSCREEN_BASE + currVramOff),
            (u16)_LR_BACKUP_H,
            (u16)_LR_BACKUP_WBYTES
        );

        if ((s16)g_mouse_present > 0) {
            bgi_mouse_set_pos((u16)MOUSE_SET_X, (u16)MOUSE_SET_Y);
        }

        selectedPos = 1;
        done = 0;
        flushKeyboard();

        scratchOff = 0xBB80u;

        cursorX = (u16)(((u16)selectedPos << 4) + (u16)LR_PICK_X_BASE);
        cursorY = (u16)LR_PICK_Y;

        currVramOff = (u16)((u16)(cursorY * (u16)EGA_BPL) + (u16)(cursorX >> 3));
        prevVramOff = currVramOff;

        for (;;) {
            u16 buttons = 0u, mx = 0u, my = 0u;
            u8  scan = 0u;

            currVramOff = (u16)((u16)(cursorY * (u16)EGA_BPL) + (u16)(cursorX >> 3));

            // подложку под курсором берём из OFFSCREEN и складываем в scratchOff
            ega_vram_move_blocks(
                (u16)(EGA_OFFSCREEN_BASE + currVramOff),
                scratchOff,
                (u16)_LR_HAND_H,
                (u16)_LR_HAND_WBYTES
            );

            // рисуем руку в OFFSCREEN
            draw_rle_packed_sprite(
                2u,
                cursorY,
                cursorX,
                PTR_VRAM_7D00_OFFSCREEN,
                GET_GFX_PTR(RES_GFX_HAND_SPRITE)
            );

            // копируем руку на экран
            ega_vram_move_blocks(
                (u16)(EGA_OFFSCREEN_BASE + currVramOff),
                currVramOff,
                (u16)_LR_HAND_H,
                (u16)_LR_HAND_WBYTES
            );

            // восстанавливаем предыдущую позицию руки на экране
            ega_vram_move_blocks(
                (u16)(EGA_OFFSCREEN_BASE + prevVramOff),
                prevVramOff,
                (u16)_LR_HAND_H,
                (u16)_LR_HAND_WBYTES
            );

            prevVramOff = currVramOff;

            // восстанавливаем OFFSCREEN под новой рукой из scratchOff
            ega_vram_move_blocks(
                scratchOff,
                (u16)(EGA_OFFSCREEN_BASE + prevVramOff),
                (u16)_LR_HAND_H,
                (u16)_LR_HAND_WBYTES
            );

            // клавиатура
            g_mPressedKey = KEY_DEFAULT;
            scan = 0u;

            if (input_tp_readkey_nb(&g_mPressedKey, &scan)) {
                input_process_game_sound_and_exit_control_keys();

                if (g_mPressedKey == 0u) {
                    if (scan == KEY_LEFT)  --selectedPos;
                    if (scan == KEY_RIGHT) ++selectedPos;
                }
            }

            // мышь
            if ((s16)g_mouse_present > 0) {
                mouse_query(&buttons, &mx, &my);
                g_mouse_buttons = buttons;
                g_space_pressed = (buttons & 1u) ? 1u : 0u;

                if (mx < (u16)MOUSE_LEFT_THRESH) {
                    --selectedPos;
                    bgi_mouse_set_pos((u16)MOUSE_SET_X, (u16)MOUSE_SET_Y);
                } else if (mx > (u16)MOUSE_RIGHT_THRESH) {
                    ++selectedPos;
                    bgi_mouse_set_pos((u16)MOUSE_SET_X, (u16)MOUSE_SET_Y);
                }
            }

            // клампим
            wordLen = (u16)G_WORDLEN;
            if (selectedPos < 1) selectedPos = 1;
            if ((u16)selectedPos > wordLen) selectedPos = (s16)wordLen;

            cursorX = (u16)(((u16)selectedPos << 4) + (u16)LR_PICK_X_BASE);
            cursorY = (u16)LR_PICK_Y;

            // подтверждение только если позиция ещё закрыта - если нет писк
            if (g_mPressedKey == KEY_SPACE || ((s16)g_mouse_present > 0 && g_space_pressed == 1u)) {
                if (OPENED_GET((u16)selectedPos) == 0u) {
                    done = 1;
                } else {
                    tp_sound((u16)(1000u * (u16)g_soundONOF_soundMultiplier));
                    pit_delay_ms(0x32u);
                    tp_nosound();
                }
            }

            pit_delay_ms(0x1Eu);
            if (done > 0) break;
        }

        // выход: короткий звук и подчищаем руку
        tp_sound((u16)(0x64u * (u16)g_soundONOF_soundMultiplier));
        ega_vram_move_blocks(
            (u16)(EGA_OFFSCREEN_BASE + currVramOff),
            currVramOff,
            (u16)_LR_HAND_H,
            (u16)_LR_HAND_WBYTES
        );
        tp_nosound();
    }
   
    // ветка: CPU выбирает позицию
 
    else {
        wordLen = (u16)G_WORDLEN;

        do {
            selectedPos = (s16)(tp_random(wordLen) + 1u);
        } while (OPENED_GET((u16)selectedPos) != 0u);

        // пузырь "<NN>-я буква"
        {
            u16 bubbleOff = bubble_vram_off_for_player(g_active_player);
            u16 bubbleXpx = (u16)PLAYER_BUBBLE_X(g_active_player);

            ega_vram_move_blocks(
                bubbleOff,
                (u16)(EGA_OFFSCREEN_BASE + bubbleOff),
                (u16)BUBBLE_SAVE_HROWS,
                (u16)BUBBLE_SAVE_WBYTES
            );

            draw_rle_packed_sprite(
                2u,
                (u16)BUBBLE_SPR_Y,
                bubbleXpx,
                PTR_VRAM_A000,
                GET_GFX_PTR(RES_GFX_TALK_BUBBLE)
            );

            sprintf(msgStr, "%u%s", (u16)selectedPos, STR_AYA_BUKVA);

            print_text(
                (const u8 __far*)LETTER_FONT_PTR,
                0u,
                (u16)LETTER_ADVANCE_PX,
                (u16)LETTER_FONT_H_MINUS_1,
                0u,
                (u16)BUBBLE_TEXT_Y,
                (u16)(bubbleXpx + 6u),
                msgStr
            );

            loop_wait_and_get_keys(0x28u);

            ega_vram_move_blocks(
                (u16)(EGA_OFFSCREEN_BASE + bubbleOff),
                bubbleOff,
                (u16)BUBBLE_SAVE_HROWS,
                (u16)BUBBLE_SAVE_WBYTES
            );
        }
    }

    // --------------------------------------------------------
 
    g_current_letter = (u8)g_current_game_word[(u16)selectedPos - 1u];
    g_word_state.match_count = 0u;

    wordLen = (u16)G_WORDLEN;
    for (iPos = 1u; iPos <= wordLen; ++iPos) {
        u8 ch = (u8)g_current_game_word[iPos - 1u];
        if (ch != g_current_letter) continue;
        if (OPENED_GET(iPos) != 0u) continue;

        if (g_word_state.match_count < 20u) {
            g_word_state.found[g_word_state.match_count] = (u16)iPos;
        }
        ++g_word_state.match_count;
        OPENED_SET(iPos, 1u);
    }

    // индекс текущей буквы в алфавите (0..31)
    iPos = 0u;
    while (iPos < 32u) {
        if (kAlphabet32Pas[1u + iPos] == g_current_letter) break;
        ++iPos;
    }
    if (iPos >= 32u) iPos = 0u;

    g_used_letter[iPos] = 1u;

    cursorX = (u16)(iPos * (u16)REVEAL_X_STEP_PX);
    cursorY = (u16)REVEAL_Y;

    // анимация раскрытия: 4 кадра + бипы
    for (iPos = 0u; iPos < 4u; ++iPos) {
        if (iPos < 3u) {
            u16 rid = (iPos == 0u) ? RES_GFX_REVEAL_0 :
                      (iPos == 1u) ? RES_GFX_REVEAL_1 :
                                     RES_GFX_REVEAL_2;

            draw_rle_packed_sprite(
                0x10u,
                cursorY,
                cursorX,
                PTR_VRAM_A000,
                GET_GFX_PTR(rid)
            );
        } else {
            ega_gc_write(0x05u, 0x00u);
            ega_wm2_bar((u8 __far*)MK_FP(EGA_VIDEO_SEGMENT, 0),
                        (int)cursorX, (int)cursorY,
                        (int)(cursorX + 0x13u),
                        (int)(cursorY + 0x12u),
                        7u);
        }

        for (beepStep = 1u; beepStep <= 10u; ++beepStep) {
            u16 tone = (u16)((iPos * 0x64u) + (beepStep * 0x0Au) + 0x32u);
            u16 del  = (u16)((beepStep / 5u) + (iPos * 4u));

            tone = (u16)(tone * (u16)g_soundONOF_soundMultiplier);

            tp_sound(tone);
            pit_delay_ms(1u);
            tp_nosound();
            pit_delay_ms(del);
        }
    }
}
