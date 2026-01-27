#ifndef RESOURCES_H
#define RESOURCES_H
#include "globals.h"
#include "typedefs.h"


// хедер 128 байт


#define POLE_LIB_HEADER_SIZE 128
#define POLE_MAX_RES     127  // 128 байт, а в первом размер - но в оригинале мы еще ограничены константами в ds, просто 
                              // некуда будет положить указатель на ресурс

typedef struct PoleFonts {
    u8 far *f6;   // 0x600  8x6 (6*256)  
    u8 far *f8;   // 0x800  8x8 (8*256) похож на PU_VEGA_Pankov-8X8-AR.FNT  
    u8 far *f14;  // 0xE00  8x14 (14*256)похож на VEGA_Pankov_8x14   
} PoleFonts;

/* запись в POLE.PIC - 13(0x0D) байт :
   ShortString[10] => 1 byte len + 10 chars = 11 bytes,
   потом слово (2байта) по смещению +0x0B. Вместе - 13 */
#pragma pack(push, 1)
typedef struct PolePicRec {
    u8  len;
    char name[10];
    u16 score;
} PolePicRec;
#pragma pack(pop)

typedef struct Resources {
    /* POLE.LIB */
    u8  res_count;                   
    u8  res_sectors[POLE_MAX_RES+1];   // размер в байтах (1..count)  
    void far *res_ptr[POLE_MAX_RES+1]; // указатели (1..count) 

    /* POLE.FNT */
    PoleFonts fonts;

    /* POLE.PIC */
    PolePicRec pic[8];
} Resources;

extern Resources gRes;


s16  loadResourcesFromFiles(Resources *R,
                            const char *pole_lib_path,
                            const char *pole_fnt_path,
                            const char *pole_pic_path);

void freeResources(Resources *R);

#endif

