/* end_screen.c */
#include "end_screen.h"

#include <string.h>
#include "globals.h"
#include "pole_dialog.h"
#include "resources.h"    
#include "ega_text.h"     
 


extern void ega_draw_text_8xN(const u8 far *font, u16 glyph_h, u16 x, u16 y, u8 color, u16 xadvance, const char *s);

 
#define END_FOOTER_CENTER_X_BASE   ((u16)320u)   /* 0x140 */
#define END_FOOTER_Y1              ((u16)332u)   /* 0x14C */
#define END_FOOTER_Y2              ((u16)341u)   /* 0x155 */

#define END_FOOTER_FONT_H          ((u16)8u)
#define END_FOOTER_XADV            ((u16)8u)
#define END_FOOTER_COLOR           ((u8)0u)

#define END_FOOTER_FONT   (gRes.fonts.f8)

 
void end_screen_print_footer(void)
{
    u16 x1, x2;

    x1 = END_FOOTER_CENTER_X_BASE - 4*strlen(end_autorDimaBashurov);
    x2 = END_FOOTER_CENTER_X_BASE - 4*strlen(end_telephonArzamas);

    
    ega_draw_text_8xN(END_FOOTER_FONT, END_FOOTER_FONT_H,
                      x1, END_FOOTER_Y1,
                      END_FOOTER_COLOR, END_FOOTER_XADV,
                      end_autorDimaBashurov);

    ega_draw_text_8xN(END_FOOTER_FONT, END_FOOTER_FONT_H,
                      x2, END_FOOTER_Y2,
                      END_FOOTER_COLOR, END_FOOTER_XADV,
                      end_telephonArzamas);
}

