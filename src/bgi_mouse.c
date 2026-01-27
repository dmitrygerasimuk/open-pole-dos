#include "bgi_mouse.h"
#include "debuglog.h"
#include "globals.h"
#include <dos.h>  


int bgi_mouse_present(void) { return g_mouse_present; }

 
static  void int33(union REGS *r)
{
    int86(0x33, r, r);
}

int bgi_mouse_init(void)
{
    union REGS r;
    r.x.ax = 0x0000;            
    int33(&r);

    // AX=0  если нет или  AX=FFFFh (обычно)  
    g_mouse_present = (r.x.ax != 0);
    g_mouse_buttons = r.x.bx;
    DBG("bgi_mouse_init: mouse_present = %X \n", g_mouse_present);
    return g_mouse_present;
}

void bgi_mouse_show(void)
{
    union REGS r;
    if (!g_mouse_present) return;
    r.x.ax = 0x0001;           // показать курсокр
    int33(&r);
}

void bgi_mouse_hide(void)
{
    union REGS r;
    if (!g_mouse_present) return;
    r.x.ax = 0x0002;           // спрятать курсор
    int33(&r);
}

void bgi_mouse_set_x_range(u16 min_x, u16 max_x)
{
    union REGS r;
    if (!g_mouse_present) return;
    r.x.ax = 0x0007;
    r.x.cx = min_x;
    r.x.dx = max_x;
    int33(&r);
}

void bgi_mouse_set_y_range(u16 min_y, u16 max_y)
{
    union REGS r;
    if (!g_mouse_present) return;
    r.x.ax = 0x0008;
    r.x.cx = min_y;
    r.x.dx = max_y;
    int33(&r);
}

int bgi_mouse_poll(MouseState *st)
{
    union REGS r;
    if (!g_mouse_present || !st) return 0;

    r.x.ax = 0x0003;            // позиция и кнопки
    int33(&r);

    st->buttons = r.x.bx;
    st->x = r.x.cx;
    st->y = r.x.dx;
    return 1;
}

int bgi_mouse_init_640x350(void)
{
    if (!bgi_mouse_init()) return 0;

    
    bgi_mouse_hide();
    bgi_mouse_set_x_range(0, 639);
    bgi_mouse_set_y_range(0, 349);

    return 1;
}

 void bgi_mouse_set_pos(u16 x, u16 y)
{
    union REGS r;
    r.x.ax = 0x0004;
    r.x.cx = x;
    r.x.dx = y;
    int86(0x33, &r, &r);
}
