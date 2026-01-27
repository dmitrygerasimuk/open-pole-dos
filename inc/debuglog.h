#ifndef DEBUGLOG_H
#define DEBUGLOG_H

#include "typedefs.h"



 


void dbg_init(int argc, char **argv);
void dbg_set_graphics(int on);
int  dbg_is_enabled(void);

void dbg_printf(const char *tag, const char *fmt, ...);
void dbg_flush(void);
void dbg_free(void);
void dbg_service(void);
 


/* 1) Чистый break в отладчик: INT 3 */
 
inline void dbg_break(void);
#pragma aux dbg_break = "int 3";


void dbg_hex(const void far *p, unsigned len);

 
int dbg_screenshot_pcx(const char *path);


// x_px должен быть кратен 8 
int dbg_screenshot_pcx_vram_rect(const char *path,
                                 unsigned base_off,
                                 unsigned x_px, unsigned y,
                                 unsigned w_px, unsigned h);

// offscreen начинается с base_off, x=0,y=0,w=640  
int dbg_screenshot_pcx_offscreen(const char *path, unsigned base_off);

int dbg_screenshot_pcx_auto(const char *path_opt);
int dbg_screenshot_pcx_offscreen_auto(const char *path_opt, unsigned base_off);

void dbg_hexdump(const char *title, const void far *p, u16 len);
void dbg_check_stack(void);


#define FPTR(fn) ((void (__far*)())(fn))

#define DBG_HEX(p,l)  dbg_hex((const void far *)(p), (unsigned)(l))

/* Красивые теги */
#define DBG(fmt, ...)      dbg_printf("DBG",  fmt, ##__VA_ARGS__)
#define INF(fmt, ...)      dbg_printf("INF",  fmt, ##__VA_ARGS__)
#define WRN(fmt, ...)      dbg_printf("WRN",  fmt, ##__VA_ARGS__)
#define ERR(fmt, ...)      dbg_printf("ERR",  fmt, ##__VA_ARGS__)
#define EVENT(fmt, ...)      dbg_printf("EVENT",  fmt, ##__VA_ARGS__)


#define SCRN()     dbg_screenshot_pcx_auto(NULL)
#define SCRN_OFF(base_off)  dbg_screenshot_pcx_offscreen_auto(NULL, base_off)



// откуда вызвали 
#define DBGL(fmt, ...)     dbg_printf("DBG",  "%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__)


#define DBG_ADDR(name) \
    DBG("%-28s @%04X:%04X size=%u\n", #name, \
        (u16)FP_SEG((void far*)&(name)), (u16)FP_OFF((void far*)&(name)), (u16)sizeof(name))

#define DBG_U16(name) \
    DBG("%-28s @%04X:%04X size=%u val=%u (0x%04X)\n", #name, \
        (u16)FP_SEG((void far*)&(name)), (u16)FP_OFF((void far*)&(name)), (u16)sizeof(name), \
        (u16)(name), (u16)(name))

#define DBG_U8(name) \
    DBG("%-28s @%04X:%04X size=%u val=%u (0x%02X)\n", #name, \
        (u16)FP_SEG((void far*)&(name)), (u16)FP_OFF((void far*)&(name)), (u16)sizeof(name), \
        (u16)(name), (u16)(name))

#define DBG_S16(name) \
    DBG("%-28s @%04X:%04X size=%u val=%d (0x%04X)\n", #name, \
        (u16)FP_SEG((void far*)&(name)), (u16)FP_OFF((void far*)&(name)), (u16)sizeof(name), \
        (s16)(name), (u16)(name))

#define DBG_STRING_VAR(var) \
    dbg_hexdump(#var, (void far *)(var), (u16)(strlen((const char *)(var)) + 1u))


#endif
