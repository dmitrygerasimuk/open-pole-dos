#ifndef UI_KEYMAP_H
#define UI_KEYMAP_H

#include "typedefs.h" 

// Возвращает CP866 uppercase букву, или пробел (0x20) если не распознано 
u8 ui_map_input_letter(u8 ch);
 u8 ui_upcase_ascii(u8 ch);

#endif
