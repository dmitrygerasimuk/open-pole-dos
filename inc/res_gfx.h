#ifndef RES_GFX_H
#define RES_GFX_H

#include <stdint.h>


#define RES_GFX_TRANSPARENT_GRAY 7u
#define RES_GFX_NO_TRANSPARENCY 0x10u
 

#define RLE_TRANSPARENT_SPRITE_57_RUBLES 2u



#define GET_GFX_PTR(id) (gRes.res_ptr[(id)])

typedef enum ResGfxId
{
 
    RES_GFX_BARABAN_BG0               = 1,
    RES_GFX_BARABAN_BG1               = 2,
    RES_GFX_BARABAN_BG2               = 3,
    RES_GFX_BARABAN_BG3               = 4,

   
    RES_GFX_BARABAN_SLOT_PLUS         = 5,
    RES_GFX_BARABAN_SLOT_5            = 6,
    RES_GFX_BARABAN_SLOT_10           = 7,
    RES_GFX_BARABAN_SLOT_15           = 8,
    RES_GFX_BARABAN_SLOT_20           = 9,
    RES_GFX_BARABAN_SLOT_DEATH        = 10,
    RES_GFX_BARABAN_SLOT_X2           = 11,
    RES_GFX_BARABAN_SLOT_X4           = 12,
    RES_GFX_BARABAN_SLOT_FIGA         = 13,
    RES_GFX_BARABAN_SLOT_PRIZE        = 14,
    RES_GFX_BARABAN_SLOT_25           = 15,
    RES_GFX_BARABAN_SLOT_50           = 16,
    RES_GFX_BARABAN_ARROW             = 17,

  
    RES_GFX_REVEAL_0                  = 18,
    RES_GFX_REVEAL_1                  = 19,
    RES_GFX_REVEAL_2                  = 20,
    RES_GFX_REVEAL_3                  = 21,

 
    RES_GFX_PYATOCHOK_SPRITE          = 22,
    RES_GFX_VINNI_SPRITE              = 23,

    RES_GFX_GIRL_WALK_SPRITE_0        = 24,
    RES_GFX_GIRL_WALK_SPRITE_1        = 25,
    RES_GFX_GIRL_WALK_SPRITE_2        = 26,
    RES_GFX_GIRL_WALK_SPRITE_3        = 27,
 
    RES_GFX_YAKUBOVICH_152x152        = 28,
    RES_GFX_MONEY_SNICKERS_SPRITE     = 29,
    RES_GFX_HAND_SPRITE               = 30,
    RES_GFX_MONEY_RUBLES_SPRITE       = 31,

 
    RES_GFX_RANDOM_BACK_TILE_0        = 32,
    RES_GFX_RANDOM_BACK_TILE_1        = 33,
    RES_GFX_RANDOM_BACK_TILE_2        = 34,

   
    RES_GFX_BACKGROUND_GREEN_LAMP     = 35,
    RES_YAKUBOVICH_BUBBLE_TALK        = 36,
    RES_GFX_MONEYBOX_CLOSED           = 37,
    RES_GFX_MONEYBOX_OPENED           = 38,
    RES_GFX_MONEYBOX_MONEY            = 39,
    RES_GFX_TALK_BUBBLE               = 40,

    RES_GFX_BACKGROUND_BLUE_WALL_LEFT = 41,
    RES_GFX_BACKGROUND_BLUE_WALL_RIGHT= 42,

 
    RES_YAKUBOVICH_FACE_MOUTH_OPEN    = 43,
    RES_YAKUBOVICH_ONLY_EYES_OPEN     = 44,
    RES_YAKUBOVICH_ONLY_EYES_CLOSE    = 45,
    RES_YAKUBOVICH_FACE_MOUTH_CLOSE   = 46,

 
    RES_GFX_TITLE_POLE                = 47,
    RES_GFX_TITLE_CHUDES              = 48,

 
    RES_GFX_KROLIK_SPRITE             = 49,
    RES_GFX_IAIA_SPRITE               = 50,
    RES_GFX_KARLSON_SPRITE            = 51,
    RES_GFX_SOVA_SPRITE               = 52,

 
    RES_GFX_BLUE_PANEL_REKLAMA        = 53,
    RES_GFX_GUY_ANIM0                 = 54,
    RES_GFX_GUY_ANIM1                 = 55,
    RES_GFX_GUY_ANIM2                 = 56,

    RES_GFX_RUBLE_STANDALONE          = 57,
} ResGfxId;

#define RES_GFX_NULL_SPRITE RES_GFX_BARABAN_SLOT_DEATH // дешевая и сердитая отладка


#endif
