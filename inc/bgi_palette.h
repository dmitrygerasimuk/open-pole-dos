#ifndef BGI_PALETTE_H
#define BGI_PALETTE_H

// BIOS-only palette API.
// Values are 6-bit (0..63) in 16-color EGA/VGA modes.

#include "typedefs.h"

 
void bgi_palette_apply_default(void);

 
void bgi_palette_apply_startup(void);


void bgi_palette_set_pal_for_mono_screen(void);

// Set a palette slot:
// logical_color_0_15: 0..15
// val0_63:            0..63 (6-bit)
void bgi_palette_set_slot(u16 logical_color_0_15, u16 val0_63);

// Set overscan/border: 0..63 (6-bit)
void bgi_palette_set_overscan(u16 val0_63);

#endif
