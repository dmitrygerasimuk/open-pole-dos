// globals.c
#include "globals.h"
#include "debuglog.h"
#include "res_gfx.h"
#include "pole_dialog.h"

#include <string.h>
#include <dos.h>  /* FP_SEG/FP_OFF */

#define SCORE_X_P1 388u
#define SCORE_Y_P1 120u

#define SCORE_X_P2 82u
#define SCORE_Y_P2 120u

#define SCORE_X_P3 320u
#define SCORE_Y_P3 294u

#define PLAYER_BUBBLE_X_P1 410u
#define PLAYER_BUBBLE_X_P2 130u
#define PLAYER_BUBBLE_X_P3 240u

u16      g_score_x[4]     = { 0u, SCORE_X_P1, SCORE_X_P2, SCORE_X_P3 };
u16      g_score_y[4]     = { 0u, SCORE_Y_P1, SCORE_Y_P2, SCORE_Y_P3 };
PlayerId g_active_player  = PLAYER_HUMAN;

s16       g_score_table[4] = { 25, 0, 0, 0 };
WordState g_word_state     = { 1, { 0 }, { 0 } };
u8        g_current_letter = 0;

u16 g_player_bubble_x[4] = { 0u, PLAYER_BUBBLE_X_P1, PLAYER_BUBBLE_X_P2, PLAYER_BUBBLE_X_P3 };

u8 g_buffer_string[64]     = { 0 };
u8 g_buffer_string_ovl[64] = { 0 };   /* Pascal string: theme */

u8  g_mPressedKey    = 7;
u16 g_space_pressed  = 0;
s16 g_mouse_present  = 0;
u16 g_mouse_buttons  = 0;
u16 g_round_wins     = 0;    // round number

u16  g_used_word_idx[3]      = { 0, 0, 0 }; // Таблица уже использованных индексов слов по раундам: [0],[1],[2]
                               // (в дизасме это  ds: 06AA,06AC,06AE). 
char g_current_game_word[64] = { 0 };// загаданное слово

u16  g_soundONOF_soundMultiplier = 1;
char g_answer_entered[21]         = { 0 };

u16 g_wheel_anim_counter = 0;  /* ds:0970 */
u16 g_wheelSector        = 0;  /* ds:0972 */

u16 g_used_letter[35] = { 0 }; /* 0/1 */
u16 g_player_shuffle[3 * 2 + 1] = { 0, 1u, 2u, 3u, 4u, 5u, 6u };

const u8 kAlphabet32Pas[33] = {
    0x20,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F
};

u16 g_guessed_letters_right  = 0; // если три подряд - шкатулка
u16 g_game_status_zeroifwon  = 0; // многофункциональный локал как и в оригинале 
 // 0 — обычный ход продолжаетс /  человек выиграл раунд
 // 1 — проигрыш/вылет (не человек выиграл) или введен неправильный ответ
 // 2 — выбираем взять приз
 // 3 - приз или деньги


 
u8 g_baraban_lut_table_xy_inited   = 0;

OpponentProfile g_opponents[] = {
    { RES_GFX_PYATOCHOK_SPRITE, STR_PYATOCHOK }, /* "ПЯТАЧОК" */
    { RES_GFX_VINNI_SPRITE,    STR_VINNIPUH  }, /* "ВИННИ-ПУХ" */
    { RES_GFX_KROLIK_SPRITE,   STR_KROLIK    }, /* "КРОЛИК" */
    { RES_GFX_IAIA_SPRITE,     STR_IAIA      }, /* "ИА-ИА" */
    { RES_GFX_KARLSON_SPRITE,  STR_KARLSON   }, /* "КАРЛСОН" */
    { RES_GFX_SOVA_SPRITE,     STR_SOVA      },
    // add more here
};

u16 g_opponents_count(void)
{
    return (u16)(sizeof(g_opponents) / sizeof(g_opponents[0]));
}

/*
 QWERTY -> ЙЦУКЕН (CP866 uppercase).
 В оригинале это были массивы по адресам 0x294 и 0x2B6, длина 0x21, индекс 1..0x21.
 */

const u8 g_ui_keymap_src[UI_KEYMAP_LEN] = {
    0,
    (u8)'`',  /* Ё */
    (u8)'Q',  /* Й */
    (u8)'W',  /* Ц */
    (u8)'E',  /* У */
    (u8)'R',  /* К */
    (u8)'T',  /* Е */
    (u8)'Y',  /* Н */
    (u8)'U',  /* Г */
    (u8)'I',  /* Ш */
    (u8)'O',  /* Щ */
    (u8)'P',  /* З */
    (u8)'[',  /* Х */
    (u8)']',  /* Ъ */
    (u8)'A',  /* Ф */
    (u8)'S',  /* Ы */
    (u8)'D',  /* В */
    (u8)'F',  /* А */
    (u8)'G',  /* П */
    (u8)'H',  /* Р */
    (u8)'J',  /* О */
    (u8)'K',  /* Л */
    (u8)'L',  /* Д */
    (u8)';',  /* Ж */
    (u8)'\'', /* Э */
    (u8)'Z',  /* Я */
    (u8)'X',  /* Ч */
    (u8)'C',  /* С */
    (u8)'V',  /* М */
    (u8)'B',  /* И */
    (u8)'N',  /* Т */
    (u8)'M',  /* Ь */
    (u8)',',  /* Б */
    (u8)'.'   /* Ю */
};

const u8 g_ui_keymap_dst[UI_KEYMAP_LEN] = {
    0,
    (u8)0xF0u, /* Ё */
    (u8)0x89u, /* Й */
    (u8)0x96u, /* Ц */
    (u8)0x93u, /* У */
    (u8)0x8Au, /* К */
    (u8)0x85u, /* Е */
    (u8)0x8Du, /* Н */
    (u8)0x83u, /* Г */
    (u8)0x98u, /* Ш */
    (u8)0x99u, /* Щ */
    (u8)0x87u, /* З */
    (u8)0x95u, /* Х */
    (u8)0x9Au, /* Ъ */
    (u8)0x94u, /* Ф */
    (u8)0x9Bu, /* Ы */
    (u8)0x82u, /* В */
    (u8)0x80u, /* А */
    (u8)0x8Fu, /* П */
    (u8)0x90u, /* Р */
    (u8)0x8Eu, /* О */
    (u8)0x8Bu, /* Л */
    (u8)0x84u, /* Д */
    (u8)0x86u, /* Ж */
    (u8)0x9Du, /* Э */
    (u8)0x9Fu, /* Я */
    (u8)0x97u, /* Ч */
    (u8)0x91u, /* С */
    (u8)0x8Cu, /* М */
    (u8)0x88u, /* И */
    (u8)0x92u, /* Т */
    (u8)0x9Cu, /* Ь */
    (u8)0x81u, /* Б */
    (u8)0x9Eu  /* Ю */
};

u8  g_addon_mono_palette = 0;

u16 g_seed      = 0;
u16 g_run_tests = 0;

void dbg_dump_globals(void)
{
    /* pointer sizes in your memory model */
    DBG("PTR SIZES: sizeof(void*)=%u  sizeof(void near*)=%u  sizeof(void far*)=%u\n",
        (u16)sizeof(void*),
        (u16)sizeof(void near*),
        (u16)sizeof(void far*));

    DBG("---- GLOBALS MAP ----\n");

    DBG_U16(g_active_player);

    dbg_hexdump("g_score_table", (void far*)g_score_table, (u16)sizeof(g_score_table));
    dbg_hexdump("g_player_bubble_x", (void far*)g_player_bubble_x, (u16)sizeof(g_player_bubble_x));

    DBG_ADDR(g_word_state);
    dbg_hexdump("g_word_state", (void far*)&g_word_state, (u16)sizeof(g_word_state));

    dbg_hexdump("g_score_x", (void far*)g_score_x, (u16)sizeof(g_score_x));
    dbg_hexdump("g_score_y", (void far*)g_score_y, (u16)sizeof(g_score_y));

    dbg_hexdump("g_buffer_string", (void far*)g_buffer_string, (u16)sizeof(g_buffer_string));
    DBG_U8(g_current_letter);

    DBG_ADDR(g_buffer_string_ovl);
    dbg_hexdump("g_buffer_string_ovl bytes",
                (void far*)g_buffer_string_ovl,
                (u16)(strlen((const char*)g_buffer_string_ovl) + 1u));

    DBG_U8(g_mPressedKey);
    DBG_U16(g_space_pressed);

    DBG("g_mouse_present              @%04X:%04X size=%u val=%d (0x%04X)\n",
        (u16)FP_SEG((void far*)&g_mouse_present),
        (u16)FP_OFF((void far*)&g_mouse_present),
        (u16)sizeof(g_mouse_present),
        g_mouse_present,
        (u16)g_mouse_present);

    DBG_U16(g_mouse_buttons);
    DBG_U16(g_round_wins);

    dbg_hexdump("g_used_word_idx", (void far*)g_used_word_idx, (u16)sizeof(g_used_word_idx));
    dbg_hexdump("g_current_game_word", (void far*)g_current_game_word, (u16)sizeof(g_current_game_word));

    DBG_U16(g_soundONOF_soundMultiplier);
    dbg_hexdump("g_answer_entered", (void far*)g_answer_entered, (u16)sizeof(g_answer_entered));

    DBG("---- END GLOBALS MAP ----\n");
}
