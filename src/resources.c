// resources_load.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>   
#include <dos.h>      

#include "resources.h"
#include "debuglog.h"
#include "pole_dialog.h"   
#include "typedefs.h"
#include "globals.h"

extern Resources gRes;

// -----------------------------
 

#define READBUF_SIZE        256u

// POLE.FNT sizes (bytes)
#define POLE_FONT_6_SIZE    ((u16)0x0600u)
#define POLE_FONT_8_SIZE    ((u16)0x0800u)
#define POLE_FONT_14_SIZE   ((u16)0x0E00u)


// POLE.PIC
#define PIC_COUNT           8u
#define PSTR10_MAX          10u

static void die_file_not_found(const char *name)
{
    DBG("FATAL: file not found: %s\n", name ? name : "(null)");
    printf("File %s not found\n", name ? name : "(null)");
    exit(1);
}

 
// читаем в буфер size байт из файла 
static u16 read_into_far(FILE *fp, u8 far *dst, u16 size)
{
    u8  buf[READBUF_SIZE];
    u16 left = size;

    while (left != 0u) {
        u16 chunk = (left > (u16)sizeof(buf)) ? (u16)sizeof(buf) : left;

        if (fread(buf, 1, chunk, fp) != chunk) {
            return 0u;
        }

        _fmemcpy(dst, buf, chunk);
        dst  += chunk;
        left = (u16)(left - chunk);
    }

    return 1u;
}

 
// POLE.FNT - шрифты
 

static void pole_fnt_init(PoleFonts *pf)
{
    if (!pf) return;
    pf->f6 = 0;
    pf->f8 = 0;
    pf->f14 = 0;
}

static void pole_fnt_free(PoleFonts *pf)
{
    if (!pf) return;

    if (pf->f6)  { _ffree(pf->f6);  pf->f6  = 0; }
    if (pf->f8)  { _ffree(pf->f8);  pf->f8  = 0; }
    if (pf->f14) { _ffree(pf->f14); pf->f14 = 0; }
}

// 1 успех 0 неудача
static u16 load_pole_fnt(PoleFonts *pf, const char *path)
{
    FILE *fp;

    if (!pf || !path) return 0u;

    pole_fnt_init(pf);

    DBG("load_pole_fnt: open %s\n", path);
    fp = fopen(path, "rb");
    if (!fp) {
        DBG("load_pole_fnt: open FAIL\n");
        return 0u;
    }

    pf->f6  = (u8 far*)_fmalloc(POLE_FONT_6_SIZE);
    pf->f8  = (u8 far*)_fmalloc(POLE_FONT_8_SIZE);
    pf->f14 = (u8 far*)_fmalloc(POLE_FONT_14_SIZE);

    DBG("load_pole_fnt: alloc f6=%Fp (0x600) f8=%Fp (0x800) f14=%Fp (0xE00)\n",
        (void far*)pf->f6, (void far*)pf->f8, (void far*)pf->f14);

    if (!pf->f6 || !pf->f8 || !pf->f14) {
        DBG("load_pole_fnt: alloc FAIL\n");
        fclose(fp);
        pole_fnt_free(pf);
        return 0u;
    }

    if (!read_into_far(fp, pf->f6,  POLE_FONT_6_SIZE)) {
        DBG("load_pole_fnt: read f6 FAIL\n");
        fclose(fp);
        pole_fnt_free(pf);
        return 0u;
    }

    if (!read_into_far(fp, pf->f8,  POLE_FONT_8_SIZE)) {
        DBG("load_pole_fnt: read f8 FAIL\n");
        fclose(fp);
        pole_fnt_free(pf);
        return 0u;
    }

    if (!read_into_far(fp, pf->f14, POLE_FONT_14_SIZE)) {
        DBG("load_pole_fnt: read f14 FAIL\n");
        fclose(fp);
        pole_fnt_free(pf);
        return 0u;
    }

    fclose(fp);
    DBG("load_pole_fnt: OK\n");
    return 1u;
}

 
// POLE.LIB  - графические ассеты в rle формате
 

static void free_pole_lib(Resources *R)
{
    unsigned i;

    if (!R) return;

    for (i = 1u; i <= (u16)R->res_count; ++i) {   
        if (R->res_ptr[i]) {
            _ffree(R->res_ptr[i]);
            R->res_ptr[i] = 0;
        }
        R->res_sectors[i] = 0;
    }

    R->res_count = 0;
}


static u16 load_pole_lib(Resources *R, const char *path)
{
    FILE *fp;
    u8  header[POLE_LIB_HEADER_SIZE];
    unsigned i;

    if (!R || !path) return 0u;

    DBG("load_pole_lib: open %s\n", path);
    fp = fopen(path, "rb");
    if (!fp) {
        DBG("load_pole_lib: open FAIL\n");
        return 0u;
    }

    if (fread(header, 1, POLE_LIB_HEADER_SIZE, fp) != POLE_LIB_HEADER_SIZE) {
        DBG("load_pole_lib: header read FAIL\n");
        fclose(fp);
        return 0u;
    }

    R->res_count = header[0];
    if (R->res_count > POLE_MAX_RES) R->res_count = POLE_MAX_RES;

    DBG("load_pole_lib: res_count=%u\n", (unsigned)R->res_count);

    for (i = 1u; i <= (u16)R->res_count; ++i) { // надо бы с нуля начать конечно, теперь все ресурсы смещены на один относительно оригинала
                                                // todo: можно бы было поправить, когда я всем спрайтам проставлю имена
        u8  sizeByte = header[i];
        u16 bytes    = (u16)sizeByte << 7; // sectors * 128

        R->res_sectors[i] = sizeByte;
        R->res_ptr[i]     = 0;

        if (sizeByte == 0u) {
            DBG("load_pole_lib: res[%u] sectors=0 -> NULL\n", (unsigned)i);
            continue;
        }

        R->res_ptr[i] = _fmalloc(bytes);
        if (!R->res_ptr[i]) {
            DBG("load_pole_lib: res[%u] alloc FAIL (%u bytes)\n", (unsigned)i, (unsigned)bytes);
            fclose(fp);
            free_pole_lib(R);
            return 0u;
        }

        if (!read_into_far(fp, (u8 far*)R->res_ptr[i], bytes)) {
            DBG("load_pole_lib: res[%u] read FAIL (%u bytes)\n", (unsigned)i, (unsigned)bytes);
            fclose(fp);
            free_pole_lib(R);
            return 0u;
        }
    }

    fclose(fp);
    DBG("load_pole_lib: OK\n");
    return 1u;
}

 
// POLE.PIC - таблица рекордов
 

static void pstr10_set_cp866(PolePicRec *r, const u8 *bytes, u8 len)
{
    u8 i;

    if (!r) return;

    if (len > (u8)PSTR10_MAX) len = (u8)PSTR10_MAX;

    r->len = len;

    for (i = 0u; i < (u8)PSTR10_MAX; ++i) r->name[i] = ' ';
    for (i = 0u; i < len; ++i) r->name[i] = (char)bytes[i];
}

static void polepic_clamp_len(PolePicRec pic[PIC_COUNT])
{
    unsigned i;
    for (i = 0u; i < (u16)PIC_COUNT; ++i) {
        if (pic[i].len > (u8)PSTR10_MAX) pic[i].len = (u8)PSTR10_MAX;
    }
}

static void polepic_fill_default(PolePicRec pic[PIC_COUNT])
{
    unsigned i;

    DBG("polepic_fill_default\n");

    for (i = 1u; i <= (u16)PIC_COUNT; ++i) {
        PolePicRec *r = &pic[i - 1u];

        if (i & 1u) pstr10_set_cp866(r, S_LISTIEV_PIC,     (u8)strlen(S_LISTIEV_PIC));
        else        pstr10_set_cp866(r, S_YAKUBOVICH_PIC,  (u8)strlen(S_YAKUBOVICH_PIC));

        r->score = (u16)(0x0087u - (u16)(i * 0x000Fu));
        DBG("  pic[%u]: score=%u\n", (unsigned)i, (unsigned)r->score);
    }

    polepic_clamp_len(pic);
}

 
static u16 load_pole_pic(Resources *R, const char *path)
{
    FILE *fp;

    if (!R || !path) return 1u;

    DBG("load_pole_pic: open %s\n", path);

    fp = fopen(path, "rb");
    if (!fp) {
        // если нет файла то ничего, но мы его будет генерить только если доберемся до финала
        DBG("load_pole_pic: open FAIL -> defaults (no create)\n");
        polepic_fill_default(R->pic);
        return 1u;
    }

   
    if (fread(R->pic, UI_HOF_REC_SIZE, PIC_COUNT, fp) != PIC_COUNT) {
        DBG("load_pole_pic: read FAIL -> defaults (no rewrite)\n");
        fclose(fp);
        polepic_fill_default(R->pic);
        return 1u;
    }

    fclose(fp);

    // клампим длину
    polepic_clamp_len(R->pic);

    DBG("load_pole_pic: OK\n");
    return 1u;
}

 
 

s16 loadResourcesFromFiles(Resources *R,
                           const char *pole_lib_path,
                           const char *pole_fnt_path,
                           const char *pole_pic_path)
{
    DBG("loadResourcesFromFiles: lib=%s fnt=%s pic=%s\n",
        pole_lib_path ? pole_lib_path : "(null)",
        pole_fnt_path ? pole_fnt_path : "(null)",
        pole_pic_path ? pole_pic_path : "(null)");

    if (!R) {
        DBG("loadResourcesFromFiles: R==NULL\n");
        return 0;
    }

    memset(R, 0, sizeof(*R));

    if (!load_pole_lib(R, pole_lib_path)) {
        die_file_not_found(pole_lib_path);
        return 0;
    }

    if (!load_pole_fnt(&R->fonts, pole_fnt_path)) {
        die_file_not_found(pole_fnt_path);
        return 0;
    }

    (void)load_pole_pic(R, pole_pic_path);

  

    return 1;
}

void freeResources(Resources *R)
{
    DBG("freeResources\n");

    if (!R) return;

    free_pole_lib(R);
    pole_fnt_free(&R->fonts);

    DBG("freeResources: OK\n");
}
