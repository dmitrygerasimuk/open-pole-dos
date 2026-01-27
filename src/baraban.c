#include <dos.h>      
#include <math.h>     
#include <stdlib.h>   

#include "baraban.h"      
#include "resources.h"    
#include "res_gfx.h"      
#include "rle_draw.h"    
#include "ega_draw.h"    
#include "debuglog.h"      
#include "sys_delay.h"    
#include "globals.h"      
#include "yakubovich.h"   
#include "sound.h"        
#include "input.h"        
#include "boss_mode.h"    
#include "random.h"



extern void ega_latch_fill(u16 offset, u16 row_count_minus_1, u16 count_bytes, u8 color);
extern void draw_rle_packed_sprite( u8 transparent, u16 y, u16 x, u8 far *vram,const u8 far *data);


// ------------------------------------------------------------
//  LUT storage
 
// Предвычисленные смещения по окружности для 32 фаз.
// Здесь хранится смещение в пикселях, чтобы на каждом кадре не вызывать sin/cos.
static s16 g_dx32[32];
static s16 g_dy32[32];

// Округление  
static int iround_double(double v)
{
    return (v >= 0.0) ? (int)(v + 0.5) : (int)(v - 0.5);
}

// ------------------------------------------------------------
 

// dseg:0050 barabanTable (16 words)
 
static const u16 kBarabanTable[BARABAN_SECTORS] = {
    4u, 5u, 12u, 6u, 14u, 7u, 13u, 6u,
    12u, 8u, 11u, 5u, 10u, 7u, 9u, 8u
};

// dseg:0070 barabanTablePrizes (11 words, индекс 0..10)
// Значения   по логике игры: очки, множители, смерть, приз.
static const s16 kBarabanPrizeTable[11] = {
    (s16)0x0064, // 100 плюс
    (s16)0x0005, // 5
    (s16)0x000A, // 10
    (s16)0x000F, // 15
    (s16)0x0014, // 20
    (s16)0xFFFF, // -1 (смерть)
    (s16)0x00CA, // 202 (x2)
    (s16)0x00CC, // 204 (x4)
    (s16)0x0000, // 0 (фига)
    (s16)0x012C, // 300 (приз)
    (s16)0x0019  // 25
};

// typeIndex (type-4) -> sprite resource id
// type лежит в [4..14], значит typeIndex = 0..10.
static const u16 kTypeIndexToResId[11] = {
    RES_GFX_BARABAN_SLOT_PLUS,  // 100
    RES_GFX_BARABAN_SLOT_5,     // 5
    RES_GFX_BARABAN_SLOT_10,    // 10
    RES_GFX_BARABAN_SLOT_15,    // 15
    RES_GFX_BARABAN_SLOT_20,    // 20
    RES_GFX_BARABAN_SLOT_DEATH, // -1
    RES_GFX_BARABAN_SLOT_X2,    // 202
    RES_GFX_BARABAN_SLOT_X4,    // 204
    RES_GFX_BARABAN_SLOT_FIGA,  // 0
    RES_GFX_BARABAN_SLOT_PRIZE, // 300
    RES_GFX_BARABAN_SLOT_25     // 25
};

// ------------------------------------------------------------
 
// Выбор одного из 4 фонов барабана   (&3 = мод 4).
static inline void far *baraban_bg_ptr(u16 bg_idx)
{
    switch (bg_idx & 3u) {
    default:
    case 0: return gRes.res_ptr[RES_GFX_BARABAN_BG0];
    case 1: return gRes.res_ptr[RES_GFX_BARABAN_BG1];
    case 2: return gRes.res_ptr[RES_GFX_BARABAN_BG2];
    case 3: return gRes.res_ptr[RES_GFX_BARABAN_BG3];
    }
}

 
// LUT init: считаем dx/dy для 32 фаз через sin/cos один раз
 

static void init_baraban_xy_tables(void)
{
    // PI руками: M_PI может отсутствовать в watcom/math.h
    const double PI = 3.14159265358979323846;
    int i;

    DBG_U8(g_baraban_lut_table_xy_inited);

   
    _disable();

    for (i = 0; i < 32; ++i) {
        // angle = 2*pi*i/32 => 32 равномерных фаз по окружности
        double ang = (2.0 * PI * (double)i) / 32.0;

        // Округление 
        int dx = iround_double(cos(ang) * (double)BARABAN_RX);
        int dy = iround_double(sin(ang) * (double)BARABAN_RY);

        DBG("Filling LUT table with i=%d: angle=%.4f rad => dx=%d, dy=%d\n", i, ang, dx, dy);

        // open watcom где-то ломает fpu вызовы 
        if ((s16)dx == (s16)0x8000 || (s16)dy == (s16)0x8000) {
            DBG("LUT BAD: i=%d ang=%lf dx=%d dy=%d\n", i, ang, dx, dy);
        }
 
        g_dx32[i] = (s16)dx;
        g_dy32[i] = (s16)dy;
    }

    _enable();

    g_baraban_lut_table_xy_inited = (u8)1u;

    DBG_U8(g_baraban_lut_table_xy_inited);

    // Дамп всей таблицы (название историческое)
    dbg_hexdump("g_dx32[i]", (void far*)g_dx32, (u16)sizeof(g_dx32));
    dbg_hexdump("g_dy32[i] ", (void far*)g_dy32, (u16)sizeof(g_dy32));
}

// ------------------------------------------------------------
 
static void baraban_draw_to_vram_lut(u16 barabanCounter, u16 vram_page_off)
{
    u8 far *vram = (u8 far*)MK_FP(0xA000, vram_page_off);
    unsigned i;

    // Инициализируем LUT один раз, при первом вызове
    if ((u8)g_baraban_lut_table_xy_inited == 0u) init_baraban_xy_tables();

    //   фон барабана  
    {
        // (barabanCounter + 3) & 3  => 0..3
        u16 bg = (u16)((barabanCounter + 3u) & 3u);
        void far *src = baraban_bg_ptr(bg);

        draw_rle_packed_sprite(
            BARABAN_BG_MODE,
            BARABAN_BG_Y,
            BARABAN_BG_X,
            vram,
            src
        );
    }

    // --- 16 элементов по окружности ---
    for (i = 0; i < BARABAN_SECTORS; ++i) {
        // idx 0..31: 32 фазы, cекторов 16 => (2*i + counter), &31 = модуль 32
        u16 idx = (u16)((2u * i + barabanCounter) & 31u);

        // Позиция на эллипсе
        int x = BARABAN_CX + g_dx32[idx];
        int y = BARABAN_CY + g_dy32[idx];

        // type 4..14 -> typeIndex 0..10
        u16 type = kBarabanTable[i];
        u16 typeIndex = (u16)(type - 4u);

        // Спрайт сектора по типу
        void far *spr = gRes.res_ptr[kTypeIndexToResId[typeIndex]];

        draw_rle_packed_sprite(
            BARABAN_ITEM_TRANSPARENCY,
            (u16)y,
            (u16)x,
            vram,
            spr
        );
    }

    // --- стрелка  
    draw_rle_packed_sprite(
        BARABAN_ARROW_MODE_TRANSPARENCY,
        BARABAN_ARROW_Y,
        BARABAN_ARROW_X,
        vram,
        gRes.res_ptr[RES_GFX_BARABAN_ARROW]
    );
}

//  seg000:2B4E                   baraban_draw    

void baraban_draw(u16 barabanCounter)
{
    baraban_draw_to_vram_lut(barabanCounter, 0x0000u);
}

// ------------------------------------------------------------
 // seg000:2C6A                   baraban_spinAndSelect 



u16 baraban_spin(void)
{
    //  0..4 — стадия замедления, 4 = быстро
    u16 slowdownStage  = 0u;

    //  сколько тиков длится текущая стадия
    u16 randomStageLen = 0u;

    //  текущий тик внутри стадии
    u16 stageTick      = 0u;

    for (;;) {
        u16 didStep = 0u;  

        pit_delay_ms(1u);

        
        if (slowdownStage > 0u) {
            if (slowdownStage == 4u) {
                ++g_wheel_anim_counter;
                didStep = 1u;
            } else if (slowdownStage == 3u) {
                if ((stageTick % 2u) == 0u) {
                    ++g_wheel_anim_counter;
                    didStep = 1u;
                }
            } else if (slowdownStage == 2u) {
                if ((stageTick % 3u) == 0u) {
                    ++g_wheel_anim_counter;
                    didStep = 1u;
                }
            } else if (slowdownStage == 1u) {
                if ((stageTick % 4u) == 0u) {
                    ++g_wheel_anim_counter;
                    didStep = 1u;
                }
            }
        }

        // При остановке добиваем счётчик до чётного, чтобы попадание было строго в сектор  
        if (slowdownStage == 0u && randomStageLen > 0u) {
            if ((g_wheel_anim_counter & 1u) != 0u) ++g_wheel_anim_counter;
        }

        
        if (g_wheel_anim_counter >= BARABAN_STEPS) {
            g_wheel_anim_counter = (u16)(g_wheel_anim_counter - BARABAN_STEPS);
        }

        // Очищаем окно на оффскрине (A000:7D00 + xbase)  
        ega_latch_fill((u16)(WHEEL_XBASE_OFF + EGA_OFFSCREEN_BASE),  WHEEL_COPY_ROWS, WHEEL_COPY_COUNBYTES, 7u);
       


        // Рисуем барабан целиком в оффскрин (A000:7D00)
        baraban_draw_to_vram_lut(g_wheel_anim_counter, EGA_OFFSCREEN_BASE);

        // Щелчок при шаге
        if (didStep != 0u) {
            u16 bx = (u16)(slowdownStage * 100u);
            u16 ax = 0u;

            if (randomStageLen >= stageTick) {
                ax = (u16)(randomStageLen - stageTick);
            }

            ax = (u16)(ax * 10u);
            ax = (u16)(ax + 0x0037u);
            ax = (u16)(ax + bx);

            if (g_soundONOF_soundMultiplier != 0u) tp_sound(ax);
            else tp_sound(0u);
        }

        // stageTick увеличиваем только во время стадий замедления
        if (slowdownStage > 0u) ++stageTick;

        // Переход стадии: новая длина 5..14, стадия уменьшается, stageTick сбрасывается
        if (stageTick > randomStageLen) {
            randomStageLen = (u16)(tp_random(10u) + 5u);
            if (slowdownStage > 0u) --slowdownStage;
            stageTick = 0u;
        }

        pit_delay_ms(3u);
        tp_nosound();

        // Копируем готовое окно из оффскрина в экран
        ega_vram_move_blocks(
            (u16)(WHEEL_XBASE_OFF + EGA_OFFSCREEN_BASE),
            WHEEL_XBASE_OFF,
            WHEEL_COPY_ROWS,
            WHEEL_COPY_COUNBYTES
        );

         
        g_mPressedKey = 0;

        if (bios_kbhit()) {
            unsigned ax = bios_getkey_ax();
            g_mPressedKey = (u8)(ax & 0xFFu);
        }

        input_process_game_sound_and_exit_control_keys();

        // Старт раскрутки
        if (slowdownStage == 0u && randomStageLen == 0u) {
            if (g_active_player < 3u) {
                loop_wait_and_get_keys(0x0032u);
            }

            yak_anim_close_mouth();

            slowdownStage  = 4u;
            randomStageLen = (u16)(tp_random(0x14u) + 0x0Au);
            stageTick      = 0u;
        }

        // Условия остановки 
        if (slowdownStage != 0u) continue;
        if (randomStageLen == 0u) continue;
        if ((g_wheel_anim_counter & 1u) != 0u) continue;

        break;
    }

    
    {
        //  0..31, визуально 16 секторов => /2
        u16 q = (u16)(g_wheel_anim_counter / 2u);
        u16 m = 0u;

        // Привязка стрелки к сектору: смещение стрелки относительно нулевого сектора
        m = (u16)((ARROW_OFFSET_IN_SECTORS - q + BARABAN_SECTORS) % BARABAN_SECTORS);

        DBG("COMPUTEPRIZE: counter=%u q=%u   m(sector)=%d\n",
            (u16)g_wheel_anim_counter, (u16)q, (int)m);

        g_wheelSector = (u16)m;

        // type 4..14 -> idx 0..10 -> prize
        {
            u16 type = kBarabanTable[g_wheelSector];
            u16 idx  = (u16)(type - 4u);

            // prize кладём в g_score_table[0]  
            g_score_table[0] = (u16)kBarabanPrizeTable[idx];

            DBG("COMPUTEPRIZE: type=%u idx=%u prize(g_score_table[0])=%u\n",
                (u16)type, (u16)idx, (u16)g_score_table[0]);
        }
    }

    // финальные пики
    {
        u16 x = 1u;

        for (;;) {
            // tone = 1000 - x*30 
            s16 tone = (s16)1000 - (s16)(x * 30u);
            if (tone < 0) tone = 0;

            if (g_soundONOF_soundMultiplier != 0u) tp_sound((u16)tone);
            else { DBG("barabam_spinAndSelect: nosound branch"); }

            pit_delay_us((u16)((1000u * x) / 5u));
            pit_delay_us((u16)((1000u * x) / 3u));

            if (g_soundONOF_soundMultiplier != 0u) tp_nosound();

            // 0x1E шагов (30)
            if (x == 30u) break;
            ++x;
        }
    }

    // возвращаем сектор (0..15)
    return g_wheelSector;
}
