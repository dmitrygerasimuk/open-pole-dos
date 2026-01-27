#ifndef UI_CHOICE_H
#define UI_CHOICE_H

#include "typedefs.h"
#include "globals.h"


#define UI_DST_BASE      0x4B00u   // uiOffs
#define UI_STRIP_OFFS    0xC800u   // used by spin copy in ui_choice

#define FONT_ADV_PIX     8u
#define FONT_GH_M1_F14   0x000Du   // 14px font => 13

#define UI_FILL_COLOR    EGA_COLOR_LIGHT_GRAY
#define UI_ROWS_M1       0x0014u
#define UI_W_BYTES_50    0x0050u
#define UI_W_BYTES_1A    0x001Au
#define UI_W_BYTES_14    0x0014u

// sprite ids
#define RLE_TRANSPARENT_HAND_SPRITE EGA_COLOR_GREEN  // 2

typedef enum UiChoiceAction {
    UI_ACT_CONTINUE = 0u,      // дефолтная сторона: продолжаем  
    UI_ACT_MODE1_SKAZHU_KRUCHU    = 1u,  //  альтернативный выбор для mode=1  
    UI_ACT_MODE2_PRIZ_ILI_IGRAT   = 2u,   // альтернативный выбор для mode=2  
    UI_ACT_MODE3_PRIZ_ILI_DENGI   = 3u
} UiChoiceAction;

typedef enum UiHandChoiceAction {
    UI_HAND_CHOSE_LEFT_SIDE  = 0u,   //  дефолтная сторона: продолжаем  
    UI_HAND_CHOSE_RIGHT_SIDE = 1u,   // альтернативный выбор для mode=1 
} UiHandChoiceAction;


UiChoiceAction ui_choice(UiChoiceAction mode);
static UiHandChoiceAction ui_draw_hand_and_wait(void);
 
void ui_draw_rle(u16 x, u16 y, u8 transparent, u16 vram_off, u16 gfx_id);
 void ega_print_centered_f14(u16 y, u8 color, const char *s);
 
#endif
