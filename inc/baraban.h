#ifndef BARABAN_H
#define BARABAN_H
#include "typedefs.h"



// 16 секторов
#define BARABAN_SECTORS                 16u
#define ARROW_OFFSET_IN_SECTORS         12u // смещение стрелки относительно нулевого сектора (крайний правый)

// 32 фазы для плавного вращения (секторов 16, значит шаг сектора = 2 фазы)
#define BARABAN_STEPS                   32u

#define BARABAN_RX                      84
#define BARABAN_RY                      63

 
#define BARABAN_CX                      0x0105u
#define BARABAN_CY                      0x00C4u

// фон  
#define BARABAN_BG_MODE                 3u
#define BARABAN_BG_X                    0x00A0u
#define BARABAN_BG_Y                    0x0073u

// элементы по окружности
#define BARABAN_ITEM_TRANSPARENCY       7u

// стрелка
#define BARABAN_ARROW_MODE_TRANSPARENCY 7u
#define BARABAN_ARROW_X                 0x010Bu
#define BARABAN_ARROW_Y                 0x0098u


// X-база окна барабана на экране (смещение в VRAM, байтовое)
#define WHEEL_XBASE_OFF        ((u16)0x2404u)

 

// Размер копируемого окна 
#define WHEEL_COPY_ROWS            ((u16)0x00ACu) // ширина   в байтах  
#define WHEEL_COPY_COUNBYTES       ((u16)0x001Cu) // высота  в строках

 

u16 baraban_spin(void); // return: g_wheelSector 
void baraban_draw(u16 barabanCounter);


#endif /* BARABAN_H */
