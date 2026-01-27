#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>
#include "typedefs.h"


#define KEY_ESC     ((u8)0x1Bu)
#define KEY_CTRL_S  ((u8)0x13u)
#define KEY_SPACE   ((u8)0x20u)
#define KEY_TAB     ((u8)0x09u)
#define KEY_DEFAULT ((u8)7u)
#define KEY_CTRL_D  ((u8)0x04u)
#define KEY_BKSP    ((u8)0x08u)
#define KEY_ENTER   ((u8)0x0Du)

#define KEY_EXTENDED_MARKER ((u8)0xE0u)

#define KEY_LEFT    ((u8)0x4Bu)
#define KEY_RIGHT   ((u8)0x4Du)

#define KEY_0 ((u8)'0')
#define KEY_1 ((u8)'1')
#define KEY_2 ((u8)'2')
#define KEY_3 ((u8)'3')
#define KEY_4 ((u8)'4')
#define KEY_5 ((u8)'5')
#define KEY_6 ((u8)'6')
#define KEY_7 ((u8)'7')
#define KEY_8 ((u8)'8')
#define KEY_9 ((u8)'9')

#define KEY_A ((u8)'A')
#define KEY_B ((u8)'B')
#define KEY_C ((u8)'C')
#define KEY_D ((u8)'D')
#define KEY_E ((u8)'E')
#define KEY_F ((u8)'F')
#define KEY_X ((u8)'X')

/* if you also want lowercase as aliases */
#define KEY_a ((u8)'a')
#define KEY_b ((u8)'b')
#define KEY_c ((u8)'c')
#define KEY_d ((u8)'d')
#define KEY_e ((u8)'e')
#define KEY_f ((u8)'f')




void flushKeyboard(void);


 void bios_flushkeyboard(void);
 u16 bios_getkey_ax(void);
 int bios_kbhit(void);


 
   
void loop_wait_and_get_keys(u16 arg0);   // ждем или выходим по клавише и отрабатываем шорткаты tab, esc, etc
void poll_mouse_buttons(void);
void exit_game(void);



  void mouse_int33_from_words(void);
 
 void mouse_query(u16 *out_buttons, u16 *out_x, u16 *out_y);
 u8 input_tp_readkey_nb(u8 *out_ascii, u8 *out_scan);
 
 
 void input_process_game_sound_and_exit_control_keys(void);

#endif
