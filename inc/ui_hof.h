#ifndef UI_HOF_H
#define UI_HOF_H

#include "typedefs.h" 
#include "resources.h"


#define HOF_REC_SIZE     ((u16)13u) 
#define HOF_NAME_MAX     ((u16)10u)
#define HOF_COUNT        ((u16)8u)
#define HOF_SCORE_OFF    ((u16)11u)



#define UI_NAME_MAX_CHARS          10u
 
#define UI_HOF_STORE_MAXLEN        10u

#define UI_VRAM_BYTES_PER_ROW      80u
#define UI_NAME_Y_BASE             195u
#define UI_NAME_Y_STEP             14u
#define UI_NAME_XBYTE_BASE         62u

#define UI_NAME_X_PIX_MAIN_BASE    496u
#define UI_NAME_X_PIX_SHAD_BASE    497u
#define UI_NAME_X_ADV_PIX          8u
#define UI_NAME_GLYPH_H            14u

#define UI_COLOR_SHADOW            EGA_COLOR_LIGHT_GRAY
#define UI_COLOR_MAIN              EGA_COLOR_WHITE
#define UI_COLOR_CLEAR             EGA_COLOR_BLACK
#define UI_COLOR_CURSOR_ON         EGA_COLOR_LIGHT_CYAN

#define UI_BLINK_DELAY_TICKS       25u
#define UI_BLINK_MAX               39u
#define UI_BLINK_DIV               20u
#define UI_CURSOR_VRAM_OFF_ADD     800u
#define UI_CURSOR_H                2u
#define UI_CURSOR_WBYTES           1u

#define HOF_PANEL_OFFS             ((u16)0xB25Cu)
#define HOF_PANEL_ROWS           ((u16)0x00A0u)
#define HOF_PANEL_BYTES           ((u16)0x0014u)

#define HOF_HDR_Y1                 170u
#define HOF_HDR_Y2                 184u

#define HOF_HDR_X_SH               489u
#define HOF_HDR_X_MN               488u

#define HOF_ROW_Y_BASE             195u
#define HOF_ROW_Y_STEP             14u

#define HOF_NAME_X_SH              481u
#define HOF_NAME_X_MN              480u

#define HOF_SCORE_X_SH             591u
#define HOF_SCORE_X_MN             590u

// Wipe (шторка): оффсеты оставляем hex
#define HOF_WIPE_VMAX              ((s16)0x50)       // 80
#define HOF_WIPE_STRIDE            ((u16)0x00A0u)    // 160 bytes per row
#define HOF_WIPE_MAXROW_BASE       ((u16)0x00A0u)    // 160 rows
#define HOF_WIPE_COUNTBYTES        ((u16)0x0013u)    // 19 bytes
#define HOF_WIPE_DST_BASE          ((u16)0x33CCu)    // screen window base
#define HOF_WIPE_SRC_BASE          ((u16)(HOF_WIPE_DST_BASE + EGA_OFFSCREEN_BASE)) // 0xB0CC


#define g_hof_name_table   ((u8*)&gRes.pic[0])


//  Ввод имени для рекорда
void ui_input_winner_name(u16 slot);

 
void ui_hall_of_fame_routine(void);


#endif
