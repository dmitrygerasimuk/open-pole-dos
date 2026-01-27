#include "ui_keymap.h"

#include "debuglog.h"  
#include "globals.h"   

// default = space
#define UI_DEFAULT_CHAR ((u8)0x20u)

u8 ui_upcase_ascii(u8 ch)
{
    return (ch >= (u8)'a' && ch <= (u8)'z') ? (u8)(ch - 0x20u) : ch;
}

u8 ui_map_input_letter(u8 ch)
{
    unsigned i;
    u8 out = UI_DEFAULT_CHAR;
    u8 up  = ui_upcase_ascii(ch);

#ifdef DBGON
    DBG("ui_map_input_letter: in=%02X up=%02X\n", (u16)ch, (u16)up);
#endif

    // 1) ASCII 
    if (up < 0x80u) {
        for (i = 1u; i < (u16)UI_KEYMAP_LEN; ++i) {
            if (g_ui_keymap_src[i] == up) {
                out = g_ui_keymap_dst[i];
#ifdef DBGON
                DBG("ui_map_input_letter: ascii-map hit i=%u %02X->%02X\n",
                    (u16)i, (u16)up, (u16)out);
#endif
                break;
            }
        }
    }

    //  CP866 uppercase А..Я passthru
    if (up >= 0x80u && up < 0xA0u) out = up;

    //  CP866 lowercase а..п -> А..П
    if (up >= 0xA0u && up < 0xB0u) out = (u8)(up - 0x20u);

    // CP866 lowercase р..я -> Р..Я
    if (up >= 0xE0u && up < 0xF0u) out = (u8)(up - 0x50u);

#ifdef DBGON
    DBG("ui_map_input_letter: out=%02X\n", (u16)out);
#endif
    return out;
}
