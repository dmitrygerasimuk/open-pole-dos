#ifndef SOUND_H
#define SOUND_H

 
#include <conio.h>

#include "typedefs.h"
#include "globals.h"


/* Turbo Pascal compatible */
void tp_sound(u16 freq_hz);
void tp_nosound(void);

void sound_play6_notes(void);

#endif
