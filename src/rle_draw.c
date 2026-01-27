#include "typedefs.h"
#include "debuglog.h"
#include "ega_draw.h"
#include "rle_draw.h"
 


#include <stddef.h>

// u16 little-endian из far буфера
static inline u16 rd_u16le_far(const u8 far *p)
{
 
    return (u16)p[0] | ((u16)p[1] << 8);
}

// читает width/height из RLE-заголовка  
static inline void rle_get_wh(const u8 far *data, u16 *out_w, u16 *out_h)
{
    if (!data) {
        *out_w = 0;
        *out_h = 0;
        return;
    }

    // header: u16 w, u16 h, u16 filler  
    *out_w = rd_u16le_far(data + 0);
    *out_h = rd_u16le_far(data + 2);
}

 
  void draw_rle_packed_sprite(
    u8 transparent,
    u16 y,
    u16 x,
    u8 far *vram,
    const u8 far *data
) { 
  u16 w,h;

  if (!data) {
        DBG("drawRLEPackedStaff: data is NULL\n");
        return;
    }
     rle_get_wh(data, &w, &h);
   
// DBG("drawRLEPackedStaff: x=%u, y=%u, trans=0x%02X, data=%FP\n", x, y, transparent,data);  

  
    draw_rle_packed_sprite_inline(transparent, y, x, vram, data);
       { 
    if (FP_OFF(vram) > EGA_OFFSCREEN_BASE) { 
        DBGL("drawRLEPackedStaff: RLEOFF_%X rect: x:%u,y:%u,w:%u,h=%u,(calcoff:%04X),base_off:%04X\n",(unsigned)(FP_OFF(data)),
        x, y%400, w, h, (unsigned)(FP_OFF(vram)),  (unsigned)(FP_OFF(vram)));
      SCRN_OFF(EGA_OFFSCREEN_BASE);
    } else {
        DBGL("drawRLEPackedStaff: RLE_%X rect: x:%u,y:%u,w:%u,h=%u,(calcoff:%04X)\n",(unsigned)(FP_OFF(data)),
        x, y, w, h, (unsigned)(FP_OFF(vram)));
       SCRN()
       ; }
    }
   
  
}


// draw_rle_packed_sprite_inline() рисует один спрайт в EGA VRAM из кастомного RLE-потока: 
// читает height из заголовка, переводит (x,y) в адрес ES:DI (формула y*80 + (x>>3)), 
// учитывает битовый сдвиг x&7, настраивает EGA в write mode 2 
// и дальше построчно разбирает команды RLE. В потоке есть два типа блоков: 
// literal (идёт серия цветов подряд) и repeat (один цвет повторяется N раз); 
// прозрачный цвет пропускает пиксели без записи, только двигает позицию.
//  По каждому пикселю/серии выставляется BitMask, делается latch-read и пишется цвет,
//  затем позиция продвигается по битам/байтам и по строкам (+80 байт на следующую строку).

/* вызов:
   AL = transparent
   BX = y
   CX = x
   ES:DI = vram
   DX:SI = data
*/
                         
#pragma aux draw_rle_packed_sprite_inline = \
    /* prolog + локалки */ \
     "push bp" \
    "mov  bp, sp" \
    "sub  sp, 0x0A" \
    "push ds" \
    "mov  [bp-9], al"       /* transparent */ \
    "mov  ds, dx"           /* DS:SI -> data (DX:SI) */ \
    "cld" \
    /* RLE header: w/h */ \
    "lodsw"                 /* width, пропускаем */ \
    "lodsw"                 /* height */ \
    "mov  [bp-2], ax"       /* rows_left = height */ \
    "add  si, 2"            /* filler */ \
    \
    /* (x,y) -> VRAM offset: y*80 + (x>>3) */ \
    "push cx"               /* сохранить X */ \
    "mov  ax, bx"           /* AX = Y */ \
    "mov  cl, 6" \
    "shl  ax, cl"           /* Y*64 */ \
    "sub  cl, 2" \
    "shl  bx, cl"           /* Y*16 */ \
    "add  bx, ax"           /* Y*80 */ \
    "pop  cx"               /* вернуть X */ \
    "mov  ax, cx"           /* AX = X */ \
    "mov  ch, al"           /* для X&7 */ \
    "mov  cl, 3" \
    "shr  ax, cl"           /* X/8 */ \
    "add  bx, ax"           /* + (X>>3) */ \
    "add  di, bx"           /* ES:DI -> стартовый байт VRAM */ \
    \
    /* bitIndex = X & 7 */ \
    "and  ch, 7" \
    "mov  [bp-6], ch"       /* bitIndex (рабочий) */ \
    "mov  [bp-8], ch"       /* bitIndex (старт строки) */ \
    \
    /* EGA GC: write mode 2, BitMask -> DX=3CF */ \
    "mov  dx, 03CEh" \
    "mov  ax, 0205h" \
    "out  dx, ax" \
    "mov  al, 8" \
    "out  dx, al" \
    "inc  dx"               /* DX = 3CF */ \
    \
    /* состояние:
       ES:DI = VRAM позиция
       [bp-2] = rows_left
       [bp-4] = endPtr строки (ниже)
       [bp-6]/[bp-8] = bitIndex
       DX = 3CF (BitMask data)
    */ \
    \
    "loc_15EC8:"            /* новая RLE-строка */ \
    "push di"               /* base DI для строки */ \
    "mov  ch, [bp-8]" \
    "mov  [bp-6], ch" \
    "lodsw"                 /* delta до конца строки */ \
    "add  ax, si" \
    "mov  [bp-4], ax"       /* endPtr */ \
    "lodsb"                 /* первый control */ \
    \
    "loc_15ED6:"            /* цикл control-кодов */ \
    "cmp  al, 0FFh"         /* signed: >0x7F => literal, иначе RLE */ \
    "jle  short loc_15F07" \
    "mov  bl, al"           /* literal count */ \
    "mov  cl, [bp-6]" \
    "mov  ah, 80h"          /* маска 1 бита */ \
    "shr  ah, cl" \
    \
    "loc_15EE3:"            /* literal: BL пикселей */ \
    "lodsb"                 /* color */ \
    "cmp  al, byte ptr [bp-9]"     /* transparent? */ \
    "jz   short loc_15EF4" \
    "xchg al, ah"           /* AL=mask, AH=color */ \
    "out  dx, al"           /* BitMask */ \
    "mov  bh, es:[di]"      /* latch read */ \
    "mov  es:[di], ah"      /* write mode 2 */ \
    "mov  ah, al"           /* вернуть mask */ \
    \
    "loc_15EF4:"            /* шаг по битам/байтам */ \
    "inc  cl" \
    "ror  ah, 1" \
    "jnb  short loc_15EFD" \
    "inc  di" \
    "xor  cl, cl" \
    \
    "loc_15EFD:" \
    "dec  bl" \
    "jnz  short loc_15EE3" \
    "mov  [bp-6], cl" \
    "jmp  loc_15F97" \
    \
    "loc_15F07:"            /* RLE: повтор одного цвета */ \
    "sub  al, 80h"          /* count */ \
    "mov  bl, al" \
    "xor  ch, ch" \
    "mov  cl, [bp-6]" \
    "mov  al, 0FFh" \
    "shr  al, cl"           /* маска первого байта */ \
    "mov  ah, al" \
    "lodsb"                 /* color */ \
    "mov  bh, al" \
    "cmp  al, byte ptr [bp-9]"     /* transparent? */ \
    "jz   short loc_15F72" \
    "add  bl, [bp-6]"       /* учесть стартовый bitIndex */ \
    "cmp  bl, 7" \
    "ja   short loc_15F36" \
    "mov  cl, bl" \
    "xor  cl, 7" \
    "mov  al, 0FEh" \
    "shl  al, cl" \
    "and  al, ah" \
    "mov  cl, bl"           /* новый bitIndex */ \
    "jmp  short loc_15F3A" \
    \
    "loc_15F36:"            /* серия > 1 байта */ \
    "mov  al, ah" \
    "xor  cl, cl" \
    \
    "loc_15F3A:"            /* записать первый байт серии */ \
    "out  dx, al" \
    "mov  ah, es:[di]" \
    "mov  es:[di], bh" \
    "sub  bl, 8" \
    "cmp  bl, 0" \
    "jl   short loc_15F6C" \
    "inc  di" \
    "mov  al, 0FFh" \
    "out  dx, al"           /* дальше целые байты */ \
    "mov  al, bh" \
    \
    "loc_15F4F:"            /* полные байты */ \
    "cmp  bl, 7" \
    "jle  short loc_15F5A" \
    "stosb" \
    "sub  bl, 8" \
    "jmp  short loc_15F4F" \
    \
    "loc_15F5A:"            /* хвост (<8) */ \
    "mov  al, 0FEh" \
    "mov  cl, bl" \
    "xor  cl, 7" \
    "shl  al, cl" \
    "mov  cl, bl" \
    "out  dx, al" \
    "mov  ah, es:[di]" \
    "mov  es:[di], bh" \
    \
    "loc_15F6C:"            /* конец непрозрачной серии */ \
    "mov  [bp-6], cl" \
    "jmp  short loc_15F97" \
    \
    "loc_15F72:"            /* прозрачная серия: просто сдвиг */ \
    "add  bl, [bp-6]" \
    "mov  cl, bl" \
    "cmp  bl, 7" \
    "jle  short loc_15F7E" \
    "xor  cl, cl" \
    \
    "loc_15F7E:" \
    "sub  bl, 8" \
    "cmp  bl, 0" \
    "jl   short loc_15F94" \
    "inc  di" \
    \
    "loc_15F87:"            /* пропуск целых байтов */ \
    "cmp  bl, 7" \
    "jle  short loc_15F92" \
    "inc  di" \
    "sub  bl, 8" \
    "jmp  short loc_15F87" \
    \
    "loc_15F92:"            /* хвост по битам */ \
    "mov  cl, bl" \
    \
    "loc_15F94:"            /* сохранить новый bitIndex */ \
    "mov  [bp-6], cl" \
    \
    "loc_15F97:"            /* следующий control / конец строки */ \
    "lodsb" \
    "mov  bx, [bp-4]" \
    "cmp  bx, si" \
    "jz   short loc_15FA2" \
    "jmp  loc_15ED6" \
    \
    "loc_15FA2:"            /* следующая картинная строка */ \
    "pop  di" \
    "add  di, 50h"          /* +80 bytes */ \
    "dec  word ptr [bp-2]" \
    "jz   short loc_15FAE" \
    "jmp  loc_15EC8" \
    \
    "loc_15FAE:"            /* epilog */ \
    "pop  ds" \
    "mov  sp, bp" \
    "pop  bp" \
    "nop"                   /* pragma aux: RET не нужен */ \
    \
   __parm [__al] [__bx] [__cx] [__es __di] [__dx __si] \
    modify [__ax __bx __cx __dx __si __di __es];


 
