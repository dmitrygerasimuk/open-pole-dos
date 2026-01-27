// yakubovich.c - анимация лица/глаз Якубовича и реплики

#include "yakubovich.h"
#include "debuglog.h"

#include <dos.h>       
 
#include <string.h>   

#include "resources.h"
#include "cstr.h"
#include "rle_draw.h"    
#include "ega_draw.h"    
#include "res_gfx.h"
#include "ega_text.h"
#include "pole_dialog.h"
#include "globals.h"
#include "typedefs.h"
#include "sys_delay.h"    
#include "sound.h"        
#include "random.h"

#define YAK_LINE_MAX 64u
static char g_yak_line1[YAK_LINE_MAX];
static char g_yak_line2[YAK_LINE_MAX];

// В оригинале всегда использовался 0x10 как «прозрачный цвет» (никогда не встречается в 0..15)
#define YAK_TRANSPARENT ((u8)0x10u)

 
// Сборка/рисование реплик Якубовича
 
static void yak_build_player_line(char *dst, u16 cap, u8 with_excl)
{
    cstr_clear(dst);

    if (g_active_player == 1)      cstr_copy(dst, cap, STR_PERVIY);
    else if (g_active_player == 2) cstr_copy(dst, cap, STR_VTOROI);
    else if (g_active_player == 3) cstr_copy(dst, cap, STR_TRETII);
    else                           cstr_copy(dst, cap, STR_PERVIY);

    cstr_cat(dst, cap, with_excl ? STR_IGROK_EXCL : STR_IGROK);
}

static void yak_print_centered_str(u16 y, const char *s)
{
    u16 len = s ? (u16)strlen(s) : 0;
    u16 x = (u16)(YAK_CENTER_X_BASE - (u16)(len * YAK_CENTER_CHAR_HALFWIDTH));

    ega_draw_text_8xN(gRes.fonts.f14, YAK_TEXT_GLYPH_H, x, y,
                      YAK_TEXT_COLOR, YAK_TEXT_XADVANCE,
                      s ? s : "");
}

static void yak_draw_sprite(ResGfxId id, u16 x, u16 y)
{
    const u8 far *spr = GET_GFX_PTR(id);
    if (!spr) return;
    draw_rle_packed_sprite(YAK_TRANSPARENT, y, x, PTR_VRAM_A000, spr);
}

static void yak_draw_sprite_bubble(ResGfxId id, u16 x, u16 y)
{
    const u8 far *spr = GET_GFX_PTR(id);
    if (!spr) return;
    draw_rle_packed_sprite(YAK_BUBBLE_MASK_COLOR, y, x, PTR_VRAM_A000, spr);
}

 
// Анимации
 
void yak_draw_eyes_face(void)
{
    unsigned i;

    sound_play6_notes();
    yak_draw_sprite(RES_YAKUBOVICH_ONLY_EYES_CLOSE, YAK_EYES_X, YAK_EYES_Y_OPEN);

    
    long_pit_delay_ms(200u); // длинная задержка в pit_delay_ms странно работает на некоторых устройствах todo

    yak_draw_sprite(RES_YAKUBOVICH_ONLY_EYES_OPEN,  YAK_EYES_X, YAK_EYES_Y_OPEN);
    long_pit_delay_ms(150u);

    yak_draw_sprite(RES_YAKUBOVICH_ONLY_EYES_CLOSE, YAK_EYES_X, YAK_EYES_Y_OPEN);
    long_pit_delay_ms(150u);

    // Открытый рот + оверлей глаз
    yak_draw_sprite(RES_YAKUBOVICH_FACE_MOUTH_OPEN, YAK_FACE_X, YAK_FACE_Y);
    yak_draw_sprite(RES_YAKUBOVICH_ONLY_EYES_CLOSE, YAK_EYES_X, YAK_EYES_Y_FACE);

    for (i = 1u; i <= YAK_MAIN_CYCLES; ++i) {
        u16 d;

        sound_play6_notes();

        yak_draw_sprite(RES_YAKUBOVICH_FACE_MOUTH_CLOSE, YAK_FACE_X, YAK_FACE_Y);
        yak_draw_sprite(RES_YAKUBOVICH_ONLY_EYES_OPEN,   YAK_EYES_X, YAK_EYES_Y_OPEN);

        // магические задержки из оригинала
        d = (u16)(tp_random(2u) * 200u + 100u);
        long_pit_delay_ms(d);

        yak_draw_sprite(RES_YAKUBOVICH_FACE_MOUTH_OPEN,  YAK_FACE_X, YAK_FACE_Y);

        d = (u16)(tp_random(2u) * 50u + 100u);
        long_pit_delay_ms(d);
    }
}

void yak_anim_close_mouth(void)   // закрыть рот и забекапить фон
{
    const u16 base = YAK_BUBBLE_BACKUP_BASE;

    ega_vram_move_blocks(
        (u16)(YAK_OFFSCR_DELTA + base),
        (u16)base,
        (u16)YAK_BUBBLE_MOVE_ROWCOUNT_M1,
        (u16)YAK_BUBBLE_MOVE_BYTESPERLINE
    );

    DBGL("Here\n");
    SCRN_OFF(EGA_OFFSCREEN_BASE);

    sound_play6_notes();

    yak_draw_sprite(RES_YAKUBOVICH_FACE_MOUTH_CLOSE, YAK_FACE_X, YAK_FACE_Y);
    pit_delay_ms(30u); 
    yak_draw_sprite(RES_YAKUBOVICH_ONLY_EYES_OPEN,   YAK_EYES_X, YAK_EYES_Y_OPEN);
}

void yak_animate_face(void)
{
    sound_play6_notes();

    yak_draw_sprite(RES_YAKUBOVICH_ONLY_EYES_OPEN,  YAK_EYES_X, YAK_EYES_Y_FACE);
    long_pit_delay_ms(100);

    yak_draw_sprite(RES_YAKUBOVICH_ONLY_EYES_CLOSE, YAK_EYES_X, YAK_EYES_Y_FACE);
    long_pit_delay_ms(100);
}

 
//  реплика в пузыре + две строки
 
void yakubovich_talks(YakTalkId id)
{
    const u16 base = YAK_BUBBLE_BACKUP_BASE;

    yak_draw_eyes_face();

    // бэкап прямоугольника под пузырем 
    ega_vram_move_blocks(
        (u16)base,
        (u16)(YAK_OFFSCR_DELTA + base),
        (u16)YAK_BUBBLE_MOVE_ROWCOUNT_M1,
        (u16)YAK_BUBBLE_MOVE_BYTESPERLINE
    );

    yak_draw_sprite_bubble(RES_YAKUBOVICH_BUBBLE_TALK, YAK_BUBBLE_X, YAK_BUBBLE_Y);

    cstr_clear(g_yak_line1);
    cstr_clear(g_yak_line2);

    switch (id) {
    case YT_TURN_SPIN:
        yak_build_player_line(g_yak_line1, (u16)sizeof(g_yak_line1), 1u);
        cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), STR_VRASHCHAYTE);
        break;

    case YT_AFTER_SPIN_SECTOR:
        if (g_score_table[0] > 0 && g_score_table[0] < 100) {
            cstr_copy(g_yak_line1, (u16)sizeof(g_yak_line1), STR_U_VAS);
            cstr_cat_s16(g_yak_line1, (u16)sizeof(g_yak_line1), (s16)g_score_table[0]);
            cstr_cat(g_yak_line1, (u16)sizeof(g_yak_line1), STR_OCHKOV_EXCL);
            cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), STR_NAZOVITE);
        } else if (g_score_table[0] == 0) {
            cstr_copy(g_yak_line1, (u16)sizeof(g_yak_line1), STR_U_VAS_0);
            cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), STR_UVY_PEREHOD);
        } else if (g_score_table[0] < 0) {
            cstr_copy(g_yak_line1, (u16)sizeof(g_yak_line1), STR_VSE_DENGI_SGORELI);
            cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), STR_UVY_PEREHOD);
        } else if ((g_score_table[0] / 100) == 2) {
            cstr_copy(g_yak_line1, (u16)sizeof(g_yak_line1), STR_DENGI_UDVAIV);
            cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), STR_NAZOVITE);
        } else if (g_score_table[0] == 100) {
            cstr_copy(g_yak_line1, (u16)sizeof(g_yak_line1), STR_SEKTOR_PLUS);
            cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), STR_OTKROY_LYUBUYU);
        } else if (g_score_table[0] == 300) {
            cstr_copy(g_yak_line1, (u16)sizeof(g_yak_line1), STR_SEKTOR_PRIZ);
            cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), STR_PRIZ_ILI_IGRAEM);
        }
        break;

    case YT_LETTER_RESULT:
        if (g_word_state.match_count > 0) {
            cstr_copy(g_yak_line1, (u16)sizeof(g_yak_line1), STR_BRAVO2);
            cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), STR_EST_TAKAYA);
        } else {
            cstr_copy(g_yak_line1, (u16)sizeof(g_yak_line1), STR_NET_TAKOI);
            cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), STR_PEREHOD_HODA);
        }
        break;

    case YT_ROUND_WIN:
        yak_build_player_line(g_yak_line1, (u16)sizeof(g_yak_line1), 0u);
        cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), STR_VYIGRAL_RAUND);
        break;

    case YT_3LETTERS_BONUS:
        cstr_copy(g_yak_line1, (u16)sizeof(g_yak_line1), STR_ZA_3_BUKVY_PREMIYA);
        cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), STR_VNESITE_SHKATULKI);
        break;

    case YT_WHERE_MONEY:
        cstr_copy(g_yak_line1, (u16)sizeof(g_yak_line1), STR_GDE_DENGI);
        cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), STR_VYBIRAYTE);
        break;

    case YT_BOX_GUESSED_RIGHT:
        cstr_copy(g_yak_line1, (u16)sizeof(g_yak_line1), STR_BRAVO3);
        cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), STR_VY_OTGADALI);
        break;

    case YT_BOX_EMPTY:
        cstr_copy(g_yak_line1, (u16)sizeof(g_yak_line1), STR_UVY_ETA);
        cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), STR_SHKATULKA_PUSTA);
        break;

    case YT_WORD_CHOSEN_START:
        cstr_copy(g_yak_line1, (u16)sizeof(g_yak_line1), STR_NACHINAEM_IGRU);
        cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), STR_ZAGADANO_SLOVO);
        break;

    case YT_THEME:
        cstr_copy(g_yak_line1, (u16)sizeof(g_yak_line1), STR_TEMA);
        cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), g_buffer_string_ovl);
        break;

    case YT_YOU_ARE_CORRECT:
        cstr_copy(g_yak_line1, (u16)sizeof(g_yak_line1), STR_VY_SOVERSHENNO);
        cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), STR_PRAVY);
        break;

    case YT_WRONG_ELIM:
        cstr_copy(g_yak_line1, (u16)sizeof(g_yak_line1), STR_NEPRAVILNO_VY);
        cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), STR_POKIDAETE);
        break;

    case YT_FINAL_WIN:
        cstr_copy(g_yak_line1, (u16)sizeof(g_yak_line1), STR_POZDRAVLYAYU);
        cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), STR_VYIGRALI_FINAL);
        break;

    case YT_FINAL_LOSE:
        cstr_copy(g_yak_line1, (u16)sizeof(g_yak_line1), STR_K_SOZHALENIYU);
        cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), STR_POKIDAETE2);
        break;

    case YT_IF_SO_NAME_LETTER:
        cstr_copy(g_yak_line1, (u16)sizeof(g_yak_line1), STR_ESLI_TAK_TO);
        cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), STR_NAZOVITE_BUKVU_DOT);
        break;

    case YT_AD_BREAK:
        cstr_copy(g_yak_line1, (u16)sizeof(g_yak_line1), STR_REKLAMNAYA);
        cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), STR_PAUZA);
        break;

    case YT_PRIZE_OR_RUBLES:
        cstr_copy(g_yak_line1, (u16)sizeof(g_yak_line1), STR_PRIZ_ILI);
        cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), g_buffer_string);
        cstr_cat(g_yak_line2, (u16)sizeof(g_yak_line2), STR_RUBLEI_Q);
        break;

    case YT_TAKE_MONEY:
        cstr_copy(g_yak_line1, (u16)sizeof(g_yak_line1), STR_ZABIRAYTE);
        cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), STR_SVOI_DENGI);
        break;

    case YT_INTRO_PARTICIPANTS:
        cstr_copy(g_yak_line1, (u16)sizeof(g_yak_line1), STR_PREDSTAVLYAYU);
        cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), STR_UCHASTNIKOV);
        break;

    case YT_TAKE_PRIZE:
        cstr_copy(g_yak_line1, (u16)sizeof(g_yak_line1), STR_ZABIRAYTE);
        cstr_copy(g_yak_line2, (u16)sizeof(g_yak_line2), STR_PRIZ_EXCL);
        break;

    default:
        break;
    }

    yak_print_centered_str(YAK_LINE1_Y, g_yak_line1);
    yak_print_centered_str(YAK_LINE2_Y, g_yak_line2);

    yak_animate_face();
}

void yak_draw_yakubovich(u8 far *vram)
{
    draw_rle_packed_sprite(7, (u16)YAK_Y,       (u16)YAK_X,       vram,
                           GET_GFX_PTR(RES_GFX_YAKUBOVICH_152x152));
    draw_rle_packed_sprite(YAK_TRANSPARENT, (u16)YAK_MOUTH_Y, (u16)YAK_MOUTH_X, vram,
                           GET_GFX_PTR(RES_YAKUBOVICH_FACE_MOUTH_CLOSE));
    draw_rle_packed_sprite(YAK_TRANSPARENT, (u16)YAK_EYES_Y,  (u16)YAK_EYES_X,  vram,
                           GET_GFX_PTR(RES_YAKUBOVICH_ONLY_EYES_OPEN));
}

