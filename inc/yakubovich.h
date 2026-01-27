// yakubovich.h  

#ifndef YAKUBOVICH_H
#define YAKUBOVICH_H

#include "typedefs.h"
#include "ega_draw.h"

#define YAK_OFFSCR_DELTA              EGA_OFFSCREEN_BASE // 0x7D00
#define YAK_BUBBLE_BACKUP_BASE        ((u16)0x2BFBu)      // 0x2BFB

 
#define YAK_BUBBLE_MOVE_BYTESPERLINE  ((u16)21u)          // 0x15
#define YAK_BUBBLE_MOVE_ROWCOUNT_M1   ((u16)190u)         // 0xBE

 
#define YAK_CENTER_X_BASE             ((u16)554u)         // 0x22A
#define YAK_CENTER_CHAR_HALFWIDTH     ((u16)4u)

#define YAK_TEXT_XADVANCE             ((u16)8u)
#define YAK_TEXT_GLYPH_H              ((u16)14u)
#define YAK_TEXT_COLOR                EGA_COLOR_BLACK

 
#define YAK_LINE1_Y                   ((u16)145u)         // 0x91
#define YAK_LINE2_Y                   ((u16)158u)         // 0x9E

#define YAK_X           YAK_FULL_X
#define YAK_Y           YAK_FULL_Y

#define YAK_MOUTH_X     0x01FF
#define YAK_MOUTH_Y     0x00AD

#define YAK_FULL_X            ((u16)480u)
#define YAK_FULL_Y            ((u16)172u)

// позиции спрайтов якубовича
#define YAK_FACE_X                    ((u16)511u)         // 0x1FF
#define YAK_FACE_Y                    ((u16)173u)         // 0x0AD

#define YAK_EYES_X                    ((u16)532u)         // 0x214
#define YAK_EYES_Y                    ((u16)209u)         // 0x0D1
#define YAK_EYES_Y_OPEN               ((u16)209u)         // 0x0D1
#define YAK_EYES_Y_FACE               ((u16)201u)         // 0x0C9

#define YAK_BUBBLE_X                  ((u16)476u)         // 0x1DC
#define YAK_BUBBLE_Y                  ((u16)142u)         // 0x08E

#define YAK_MAIN_CYCLES               ((u16)3u)

 
#define YAK_BUBBLE_MASK_COLOR         EGA_COLOR_BLUE


typedef enum {
 
  YT_TURN_SPIN          = 1,   /* "<N>-й игрок!  Вращайте барабан!" */
  YT_AFTER_SPIN_SECTOR  = 2,   /* 0..99: "У вас <N> очков!  Назовите букву!"; 
                                  0: "У вас 0 очков.. Увы! Переход хода.."; 
                                  <0: "Все деньги сгорели! Увы! Переход хода.."; 
                                  202 или 204: "Деньги удваиваются! Назовите букву!"; 
                                  100: "Сектор ПЛЮС! Открой любую букву!"; 
                                  300: "Сектор ПРИЗ! Приз или играем?" */
  YT_LETTER_RESULT      = 3,   /* "Браво!! Есть такая буква!"  /  "Нет такой буквы! Переход хода.." */
  YT_ROUND_WIN          = 4,   /* "N-й игрок  выиграл раунд!" */
  YT_3LETTERS_BONUS     = 5,   /* "За 3 буквы премия! Внесите шкатулки!" */
  YT_WHERE_MONEY        = 6,   /* "Где деньги? Выбирайте!" */
  YT_BOX_GUESSED_RIGHT  = 7,   /* "Браво!!! Вы отгадали!" */
  YT_BOX_EMPTY          = 8,   /* "Увы! Эта шкатулка пуста!" */
  YT_WORD_CHOSEN_START  = 9,   /* "Начинаем игру! Загадано слово:" */
  YT_THEME              = 10,  /* "Тема:" + g_buffer_string_ovl */
  YT_YOU_ARE_CORRECT    = 11,  /* "Вы совершенно правы!!" */
  YT_WRONG_ELIM         = 12,  /* "Неправильно! Вы покидаете игру!" */
  YT_FINAL_WIN          = 13,  /* "Поздравляю! Вы  выиграли финал!" */
  YT_FINAL_LOSE         = 14,  /* "К сожалению Вы  покидаете игру.." */
  YT_IF_SO_NAME_LETTER  = 15,  /* "Если так, то  назовите букву." */
  YT_AD_BREAK           = 16,  /* "РЕКЛАМНАЯ  ПАУЗА!" */
  YT_PRIZE_OR_RUBLES    = 17,  /* "ПРИЗ или  N рублей?" */
  YT_TAKE_MONEY         = 18,  /* "Забирайте  свои деньги!" */
  YT_INTRO_PARTICIPANTS = 19,  /* "Представляю  участников!" */
  YT_TAKE_PRIZE         = 20   /* "Забирайте  свой ПРИЗ!" */
} YakTalkId;


 
void yak_draw_eyes_face(void);

 
void yak_anim_close_mouth(void);
void yak_animate_face(void);

void yakubovich_talks(YakTalkId id);
void yak_draw_yakubovich(u8 far *vram);

#endif // YAKUBOVICH_H
