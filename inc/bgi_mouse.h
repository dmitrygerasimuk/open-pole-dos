#ifndef BGI_MOUSE_H
#define BGI_MOUSE_H

#include "typedefs.h"
#include "globals.h"

typedef struct MouseState {
    u16 x;         
    u16 y;        
    u16 buttons;  // bit0=left, bit1=right, bit2=middle
} MouseState;

// Возвращает 1, если есть мышь
int  bgi_mouse_init(void);

//   reset + hide + диапазон 
int  bgi_mouse_init_640x350(void);

 
void bgi_mouse_show(void);
void bgi_mouse_hide(void);

// Ограничения координат курсора  int 33
void bgi_mouse_set_x_range(u16 min_x, u16 max_x);
void bgi_mouse_set_y_range(u16 min_y, u16 max_y);

//  int 33
int  bgi_mouse_poll(MouseState *st);

// быстрая проверка наличия мыши
int  bgi_mouse_present(void);

//  позиция курсора  через int 33
void bgi_mouse_set_pos(u16 x, u16 y);

#endif // BGI_MOUSE_H
