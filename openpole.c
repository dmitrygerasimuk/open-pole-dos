#include <stdio.h>
#include <dos.h>
#include <i86.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h> 
 
#include "asm.h"
#include "debuglog.h"
#include "random.h"
#include "rle_draw.h"
#include "resources.h"
#include "res_gfx.h"
#include "ega_text.h"
#include "sys_delay.h"
#include "ega_draw.h"
#include "input.h"
#include "baraban.h"
#include "bgi_mouse.h"
#include "bgi_palette.h"
#include "score.h"
#include "yakubovich.h"
#include "words.h"
#include "end_screen.h"
#include "sale_info.h"
#include "globals.h"
#include "ui_answer.h"
#include "ui_hof.h"
#include "ui_keymap.h"
#include "girl.h"
#include "moneybox.h"
#include "prize.h"
#include "letter.h"
#include "opponents.h"
#include "pole.h"


#define OPENPOLE_VERSION_STR "0.1.0"

Resources gRes;
extern void test_run_all();

static u8 bios_reports_fpu(void)
{
    union REGS r;
    int86(0x11, &r, &r);
    return (u8)((r.x.ax & 0x0002u) ? 1u : 0u);
}

u8 fpu_detect(void)
{
    if (!bios_reports_fpu()) return 0u;
    return 2u; //   есть x87 
}


 
void print_about(void) {
    int vga = is_vga();
    int fpu = fpu_detect();

    printf("OPENPOLE  Pole Chudes (DOS) C port by Dmitry Gerasimuk, 2025\n");
    printf("Original game by Vadim Bashurov, 1992\n");
    printf("https://github.com/dmitrygerasimuk/open-pole-dos\n");
    printf("Ver %s | OW C | %s %s | %s | FPU:%s",
           OPENPOLE_VERSION_STR,
           __DATE__, __TIME__,
           vga ? "VGA" : "EGA?",
           fpu ? "yes" : "no");
    if (g_seed) printf(" | Seed:%u", (unsigned)g_seed);
    printf("\n\n");

    printf("Args: -d/--debug  -l/--log=FILE  -s/--scr  -sd/--seed=NUM  -m/--mono\n");
    printf("Press any key...\n");
    loop_wait_and_get_keys(0u);
}


static int parse_u16_dec(const char *s, u16 *out)
{
    char *endp = NULL;
    unsigned long v;

    if (!s || !s[0]) return 0;

    v = strtoul(s, &endp, 10);
    if (!endp || *endp != '\0') return 0;
    if (v > 0xFFFFul) return 0;

    *out = (u16)v;
    return 1;
}

void parse_args(int argc, char **argv)
{
    unsigned i;

    for (i = 1; i < (u16)argc; i++) {
        const char *a = argv[i];
        const char *val = NULL;
        u16 v;

        if (!a) continue;

        //  указываем свой сид чтобы играть опеределенную партию, при 48457 первое слово ГЕРА
        if (!strcmp(a, "-sd") || !strcmp(a, "-SD") ||
            !strcmp(a, "--seed") || !strcmp(a, "--SEED")) {

            const char *next = (i + 1u < (u16)argc) ? argv[i + 1u] : NULL;
            if (next && next[0] && next[0] != '-') val = next;

            if (val && parse_u16_dec(val, &v)) {
                g_seed = v;
                
                i++; 
            }
            continue;
        }

           
        if ((!strncmp(a, "-sd=", 3) || !strncmp(a, "-SD=", 3)) &&
            a[3] && parse_u16_dec(a + 3, &v)) {
            g_seed = v;
            
            continue;
        }

      
        if ((!strncmp(a, "--seed=", 7) || !strncmp(a, "--SEED=", 7)) &&
            a[7] && parse_u16_dec(a + 7, &v)) {
            g_seed = v;
            
            continue;
        }

        // для монохромных EGA дисплеев
        if (!strcmp(a, "-m") || !strcmp(a, "-M") ||
            !strcmp(a, "--mono") || !strcmp(a, "--MONO")) {
            g_addon_mono_palette = 1;
            continue;
        }


         if (!strcmp(a, "-t") || !strcmp(a, "-T") ||
            !strcmp(a, "--test") || !strcmp(a, "--TEST")) {
            g_run_tests=1; 
            continue;
        }
    }
}



void printSeg() { 
        u16 cs = get_cs();
        u16 ds = get_ds();
        u16 ss = get_ss();
        u16 es = get_es();

        printf("SEGS(now): CS=%04X DS=%04X SS=%04X ES=%04X PSP=%04X SP=%04X\n",
            cs, ds, ss, es, (unsigned)_psp,get_sp());
}

extern void stackpat_report(void);
extern void stack_fill_pattern_early(void);

int main(int argc, char **argv)
{
    //  stack_fill_pattern_early(); // пока для дебага
    //  stackpat_report();
    //  printSeg();

    parse_args(argc, argv);
    dbg_init(argc, argv);
  
    INF("BIOS EGA MEM: %dKbytes\n",ega_bios_vram_kb());
    EVENT("Start\n");
    DBG("S: Compile time: "__DATE__" "__TIME__"\n");
    
    print_about();
    loadResourcesFromFiles(&gRes, "POLE.LIB", "POLE.FNT", "POLE.PIC");
   
    
    if (dbg_is_enabled()) {   puts("DEBUG: Press any key to begin\n");bios_getkey_ax();}
    dbg_set_graphics(1);
    

    if (g_seed) { 
        randomize(g_seed);
    } else {
        g_seed=random_seed();
        randomize(g_seed);
    }
   
    dbg_dump_globals();
    if (g_run_tests) { test_run_all(); exit_game(); }

    start_game();
    exit_game();
       
    return 0;
}


