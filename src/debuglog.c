#include "debuglog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <dos.h>
#include <io.h>
#include <stdarg.h>

#include "sys_delay.h"
#include "globals.h"
#include "resources.h"
#include "ega_text.h"
#include "ega_draw.h"

#define DBG_BUF_CAP        (20u * 1024u)
#define DBG_FLUSH_RESERVE  1024u
#define DBG_TMP_CHUNK      512u

#define DEFAULT_LOG_NAME   "openpole.log"

static u16 get_cs(void);
static u16 get_ds(void);
static u16 get_ss(void);
static u16 get_es(void);
static u16 get_sp(void);
static u16 get_bp(void);

#pragma aux get_cs = "mov ax, cs" value [ax] modify [ax];
#pragma aux get_ds = "mov ax, ds" value [ax] modify [ax];
#pragma aux get_ss = "mov ax, ss" value [ax] modify [ax];
#pragma aux get_es = "mov ax, es" value [ax] modify [ax];
#pragma aux get_sp = "mov ax, sp" value [ax] modify [ax];
#pragma aux get_bp = "mov ax, bp" value [ax] modify [ax];

void stack_probe_init(void);
u16  stack_probe_used_bytes(void);
void stack_probe_dump(const char *tag);

static char g_filebuf[2048];

static int      g_enabled        = 0;
static int      g_no_screenshots = 1;
static int      g_in_gfx         = 0;
static char    *g_buf            = NULL;
static unsigned g_len            = 0;
static unsigned g_cap            = 0;

static u32 g_t0_cycles = 0;
static int g_ts_on     = 1;

static u32 g_last_now   = 0;
static u32 g_wraps      = 0;
static u32 g_base_now   = 0;
static u32 g_base_wraps = 0;

static FILE         *g_fp         = NULL;
static int           g_to_file    = 0;
static int           g_need_flush = 0;
static unsigned long g_dropped    = 0;

static const char *g_log_path = NULL;

static int g_panic    = 0;
static int g_flushing = 0;

typedef struct EgaSnap {
    u8 seq2;
    u8 gc4;
    u8 gc5;
    u8 gc8;
} EgaSnap;

static u8   seq_get(u8 idx)        { out_dx_al(0x3C4, idx); return (u8)in_dx_al(0x3C5); }
static void seq_set(u8 idx, u8 v)  { out_dx_al(0x3C4, idx); out_dx_al(0x3C5, v); }

static u8   gc_get(u8 idx)         { out_dx_al(0x3CE, idx); return (u8)in_dx_al(0x3CF); }
static void gc_set(u8 idx, u8 v)   { out_dx_al(0x3CE, idx); out_dx_al(0x3CF, v); }

static void ega_snap_save(EgaSnap *s)
{
    s->seq2 = seq_get(0x02);
    s->gc4  = gc_get(0x04);
    s->gc5  = gc_get(0x05);
    s->gc8  = gc_get(0x08);
}

static void ega_snap_restore(const EgaSnap *s)
{
    seq_set(0x02, s->seq2);
    gc_set(0x04, s->gc4);
    gc_set(0x05, s->gc5);
    gc_set(0x08, s->gc8);
}

static void ega_snap_set_safe_read(void)
{
    seq_set(0x02, 0x0F);
    gc_set(0x05, 0x00);
    gc_set(0x08, 0xFF);
}

static int file_exists(const char *path)
{
    return (path && path[0] && access(path, 0) == 0);
}

static int ask_overwrite_append(const char *path)
{
    int c;

    text_mode_restore();
    fputs("\n[DBG] Log file exists: ", stdout);
    fputs(path, stdout);
    fputs("\nOverwrite (Y) / Append (A) / Cancel (N)? ", stdout);
    fflush(stdout);

    c = getch();
    fputc(c, stdout);
    fputc('\n', stdout);

    if (c == 'y' || c == 'Y') return 1;
    if (c == 'a' || c == 'A') return 2;
    return 0;
}

static void dbg_panic(const char *msg)
{
    if (g_panic) return;
    g_panic = 1;

    text_mode_restore();

    fputs("\n[DBG:FATAL] ", stdout);
    fputs(msg, stdout);
    fputs("\n", stdout);
    fflush(stdout);

    if (g_to_file && g_fp) {
        fputs("\n[DBG:FATAL] ", g_fp);
        fputs(msg, g_fp);
        fputs("\n", g_fp);
        fflush(g_fp);
    }

    exit(1);
}

static void dbg_flush_to_sink(void);

static inline void put_u16le(unsigned char *p, unsigned v)
{
    p[0] = (unsigned char)(v & 0xFFu);
    p[1] = (unsigned char)((v >> 8) & 0xFFu);
}

static void pcx_rle_write(FILE *f, const unsigned char *src, unsigned len)
{
    unsigned i = 0;

    while (i < len) {
        unsigned char b = src[i];
        unsigned run = 1;

        while (run < 63u && (i + run) < len && src[i + run] == b) run++;

        if (run > 1u || ((b & 0xC0u) == 0xC0u)) {
            fputc((int)(0xC0u | (unsigned char)run), f);
            fputc((int)b, f);
        } else {
            fputc((int)b, f);
        }

        i += run;
    }
}

static unsigned long dbg_now_ms(void)
{
    u32 now = pit_now_cycles();

    if (now < g_last_now) {
        u32 back = (u32)(g_last_now - now);
        if (back > 0x80000000UL) g_wraps++;
        else now = g_last_now;
    }
    g_last_now = now;

    {
        unsigned long long now64  = ((unsigned long long)g_wraps << 32) | now;
        unsigned long long base64 = ((unsigned long long)g_base_wraps << 32) | g_base_now;
        unsigned long long d = now64 - base64;

        return (unsigned long)((d * 1000ULL) / 1193182ULL);
    }
}

static inline void fill_pcx_ega_palette16(unsigned char pal48[48])
{
    static const unsigned char ega16[16][3] = {
        {   0,   0,   0 }, {   0,   0, 170 }, {   0, 170,   0 }, {   0, 170, 170 },
        { 170,  85,   0 }, { 255, 170, 170 }, { 255, 170,  85 }, { 170, 170, 170 },
        {  85,  85,  85 }, {  85,  85, 255 }, {  85, 255,  85 }, {  85, 255, 255 },
        { 255,  85,  85 }, { 255,  85, 255 }, { 255, 255,  85 }, { 255, 255, 255 }
    };

    unsigned i;
    for (i = 0; i < 16u; i++) {
        pal48[i * 3u + 0u] = ega16[i][0];
        pal48[i * 3u + 1u] = ega16[i][1];
        pal48[i * 3u + 2u] = ega16[i][2];
    }
}

int dbg_screenshot_pcx(const char *path)
{
    EgaSnap snap;
    FILE *f;

    unsigned char hdr[128];
    unsigned char line[EGA_BPL];

    volatile unsigned char __far *vram =
        (volatile unsigned char __far*)MK_FP(0xA000, 0);

    unsigned y, plane;

    if (!g_enabled) return 0;
    if (g_no_screenshots) return 0;

    f = fopen(path, "wb");
    if (!f) return 0;

    DBG("SCRN: file:%s\n", path);

    _disable();
    ega_snap_save(&snap);
    ega_snap_set_safe_read();
    _enable();

    memset(hdr, 0, sizeof(hdr));
    hdr[0] = 0x0A;
    hdr[1] = 0x05;
    hdr[2] = 0x01;
    hdr[3] = 0x01;

    put_u16le(&hdr[4],  0);
    put_u16le(&hdr[6],  0);
    put_u16le(&hdr[8],  (unsigned)(EGA_W - 1));
    put_u16le(&hdr[10], (unsigned)(EGA_H - 1));

    put_u16le(&hdr[12], EGA_W);
    put_u16le(&hdr[14], EGA_H);

    fill_pcx_ega_palette16(&hdr[16]);

    hdr[64] = 0;
    hdr[65] = 4;
    put_u16le(&hdr[66], EGA_BPL);
    put_u16le(&hdr[68], 1);

    put_u16le(&hdr[70], EGA_W);
    put_u16le(&hdr[72], EGA_H);

    fwrite(hdr, 1, sizeof(hdr), f);

    for (y = 0; y < EGA_H; y++) {
        unsigned off = y * EGA_BPL;

        for (plane = 0; plane < 4u; plane++) {
            unsigned i;

            _disable();
            gc_set(0x05, 0x00);
            gc_set(0x04, (u8)plane);
            _enable();

            for (i = 0; i < EGA_BPL; i++) {
                line[i] = (unsigned char)vram[off + i];
            }

            pcx_rle_write(f, line, EGA_BPL);
        }
    }

    fclose(f);

    _disable();
    ega_snap_restore(&snap);
    _enable();

    return 1;
}

static unsigned pcx_bpl_even(unsigned byteWidth)
{
    return (byteWidth & 1u) ? (byteWidth + 1u) : byteWidth;
}

int dbg_screenshot_pcx_vram_rect(const char *path,
                                 unsigned base_off,
                                 unsigned x_px, unsigned y,
                                 unsigned w_px, unsigned h)
{
    EgaSnap snap;
    FILE *f;

    unsigned char hdr[128];
    unsigned char line[80];

    volatile unsigned char __far *vram =
        (volatile unsigned char __far*)MK_FP(0xA000, 0);

    unsigned byteX, byteW, pcxBPL;
    unsigned maxRows;
    unsigned yy, plane;

    unsigned char save_gc5 = ega_gc_read(5);

    if (!g_enabled) return 0;
    if (g_no_screenshots) return 0;

    if (!path || !path[0]) return 0;
    if ((x_px & 7u) != 0u) return 0;
    if (w_px == 0 || h == 0) return 0;

    DBG("SCRN: file:%s,base_off:%x\n", path, base_off);

    _disable();
    ega_snap_save(&snap);
    ega_snap_set_safe_read();
    _enable();

    byteX = x_px >> 3;
    byteW = (w_px + 7u) >> 3;

    if (byteX >= 80u) return 0;
    if (byteW == 0) return 0;
    if (byteX + byteW > 80u) return 0;

    maxRows = (0x10000u - base_off) / 80u;
    if (y >= maxRows) return 0;
    if (h > (maxRows - y)) h = (maxRows - y);

    pcxBPL = pcx_bpl_even(byteW);

    f = fopen(path, "wb");
    if (!f) return 0;

    memset(hdr, 0, sizeof(hdr));
    hdr[0] = 0x0A;
    hdr[1] = 0x05;
    hdr[2] = 0x01;
    hdr[3] = 0x01;

    put_u16le(&hdr[4],  0);
    put_u16le(&hdr[6],  0);
    put_u16le(&hdr[8],  (unsigned)(w_px - 1u));
    put_u16le(&hdr[10], (unsigned)(h - 1u));

    put_u16le(&hdr[12], (unsigned)w_px);
    put_u16le(&hdr[14], (unsigned)h);

    fill_pcx_ega_palette16(&hdr[16]);

    hdr[64] = 0;
    hdr[65] = 4;
    put_u16le(&hdr[66], pcxBPL);
    put_u16le(&hdr[68], 1);

    put_u16le(&hdr[70], (unsigned)w_px);
    put_u16le(&hdr[72], (unsigned)h);

    fwrite(hdr, 1, sizeof(hdr), f);

    ega_gc_write(5, (unsigned)(save_gc5 & ~0x08u));

    for (yy = 0; yy < h; yy++) {
        unsigned base = base_off + (y + yy) * 80u + byteX;

        for (plane = 0; plane < 4u; plane++) {
            unsigned i;

            _disable();
            gc_set(0x05, 0x00);
            gc_set(0x04, (u8)plane);
            _enable();

            for (i = 0; i < byteW; i++) {
                unsigned off = base + i;
                line[i] = (unsigned char)vram[off];
            }

            for (; i < pcxBPL; i++) line[i] = 0;

            pcx_rle_write(f, line, pcxBPL);
        }
    }

    fclose(f);

    _disable();
    ega_snap_restore(&snap);
    _enable();

    return 1;
}

int dbg_screenshot_pcx_offscreen(const char *path, unsigned base_off)
{
    unsigned maxRows = (0x10000u - base_off) / 80u;
    if (maxRows == 0) return 0;

    return dbg_screenshot_pcx_vram_rect(path, base_off, 0, 0, 640u, maxRows);
}

int dbg_screenshot_pcx_auto(const char *path_opt)
{
    char name[32];
    const char *p = path_opt;

    if (!p || !p[0]) {
        unsigned long ms = dbg_now_ms();
        unsigned long id = ms % 100000000UL;
        sprintf(name, "%08lu.pcx", id);
        p = name;
    }

    return dbg_screenshot_pcx(p);
}

int dbg_screenshot_pcx_offscreen_auto(const char *path_opt, unsigned base_off)
{
    char name[32];
    const char *p = path_opt;

    if (!p || !p[0]) {
        unsigned long ms = dbg_now_ms();
        unsigned long id = ms % 100000000UL;
        sprintf(name, "f%07lu.pcx", id);
        p = name;
    }

    return dbg_screenshot_pcx_offscreen(p, base_off);
}

static void out_char(unsigned char c)
{
    FILE *dst;

    if (!g_enabled) return;

    dst = (g_to_file && g_fp) ? g_fp : stdout;

    if (!g_in_gfx) {
        fputc(c, dst);
        return;
    }

    if (!g_buf || g_cap == 0) {
        dbg_panic("gfx logging active but buffer is NULL");
        return;
    }

    if (!g_flushing) {
        if (g_len >= (g_cap - DBG_FLUSH_RESERVE)) {
            g_flushing = 1;
            dbg_flush_to_sink();
            g_flushing = 0;
        }
    }

    if (g_len >= g_cap) {
        dbg_panic("debug buffer overflow (increase DBG_BUF_CAP or flush more often)");
        return;
    }

    g_buf[g_len++] = (char)c;

    if (g_len >= (g_cap - DBG_FLUSH_RESERVE)) {
        g_need_flush = 1;
    }
}

static inline void out_str(const char *s)
{
    while (*s) out_char((unsigned char)*s++);
}

static void out_timestamp(void)
{
    char tmp[32];
    u32 now;

    if (!g_ts_on) return;

    now = pit_now_cycles();

    if (now < g_last_now) {
        u32 back = (u32)(g_last_now - now);
        if (back > 0x80000000UL) g_wraps++;
        else now = g_last_now;
    }

    g_last_now = now;

    {
        unsigned long ms, frac;
        unsigned long long now64  = ((unsigned long long)g_wraps << 32) | now;
        unsigned long long base64 = ((unsigned long long)g_base_wraps << 32) | g_base_now;
        unsigned long long d = now64 - base64;

        ms   = (unsigned long)((d * 1000ULL) / 1193182ULL);
        frac = (unsigned long)(((d * 1000ULL) % 1193182ULL) * 1000ULL / 1193182ULL);

        sprintf(tmp, "+%lu.%03lu ", ms, frac);
        out_str(tmp);
    }
}

static void out_vsnprintf(const char *fmt, va_list ap)
{
    char tmp[512];
    int n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    if (n <= 0) return;
    tmp[sizeof(tmp) - 1] = 0;
    out_str(tmp);
}

void dbg_init(int argc, char **argv)
{
    int i;

    g_enabled        = 0;
    g_in_gfx         = 0;
    g_len            = 0;
    g_need_flush     = 0;
    g_dropped        = 0;
    g_fp             = NULL;
    g_to_file        = 0;
    g_log_path       = NULL;

    for (i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!a) continue;

        if (!strcmp(a, "-d") || !strcmp(a, "-D") ||
            !strcmp(a, "--debug") || !strcmp(a, "--DEBUG")) {
            g_enabled  = 1;
            g_log_path = DEFAULT_LOG_NAME;
            continue;
        }

        if (!strcmp(a, "-scr") || !strcmp(a, "-s") ||
            !strcmp(a, "--scr") || !strcmp(a, "--scr")) {
            g_no_screenshots = 0;
            continue;
        }

        if (!strcmp(a, "--log")) {
            const char *next = (i + 1 < argc) ? argv[i + 1] : NULL;

            if (next && next[0] && next[0] != '-') {
                g_log_path = next;
                i++;
            } else {
                g_log_path = DEFAULT_LOG_NAME;
            }
            continue;
        }

        if (!strncmp(a, "--log=", 6)) {
            const char *p = a + 6;
            g_log_path = (p && p[0]) ? p : DEFAULT_LOG_NAME;
            continue;
        }
    }

    if (!g_enabled) return;

    setvbuf(stdout, NULL, _IONBF, 0);

    g_cap = DBG_BUF_CAP;
    g_buf = (char*)malloc(g_cap);
    if (!g_buf) {
        g_cap = 0;
        dbg_panic("debug buffer malloc failed (stack too big / not enough near heap)");
    }

    if (g_log_path && g_log_path[0]) {
        const char *mode = "wb";

        if (file_exists(g_log_path)) {
            int choice = ask_overwrite_append(g_log_path);
            if (choice == 0) {
                g_log_path = NULL;
            } else if (choice == 2) {
                mode = "ab";
            } else {
                mode = "wb";
            }
        }

        if (g_log_path) {
            g_fp = fopen(g_log_path, mode);
            if (g_fp) {
                g_to_file = 1;
                setvbuf(g_fp, g_filebuf, _IOFBF, sizeof(g_filebuf));
            } else {
                dbg_panic("cannot open log file");
            }
        }
    }

    out_str("[INF] debug enabled\n");

    g_t0_cycles  = pit_now_cycles();
    g_base_now   = g_t0_cycles;
    g_last_now   = g_base_now;
    g_wraps      = 0;
    g_base_wraps = 0;

    {
        unsigned cs = get_cs();
        unsigned ds = get_ds();
        unsigned ss = get_ss();
        unsigned es = get_es();

        INF("SEGS(now): CS=%04X DS=%04X SS=%04X ES=%04X PSP=%04X\n",
            cs, ds, ss, es, (unsigned)_psp);

        DBG("CODE: dbg_init=%Fp dbg_printf=%Fp dbg_screenshot=%Fp\n",
            FPTR(dbg_init), FPTR(dbg_printf), FPTR(dbg_screenshot_pcx));
    }

    fflush(stdout);
    if (g_to_file && g_fp) fflush(g_fp);
}

void dbg_set_graphics(int on)
{
    if (!g_enabled) return;

    if (g_in_gfx && !on) {
        dbg_flush_to_sink();
    }

    g_in_gfx = on ? 1 : 0;
}

static void dbg_flush_to_sink(void)
{
    FILE *dst;
    size_t wr;

    if (!g_buf || !g_len) return;

    dst = (g_to_file && g_fp) ? g_fp : stdout;

    wr = fwrite(g_buf, 1, g_len, dst);
    if (wr != (size_t)g_len) {
        dbg_panic("fwrite failed while flushing debug buffer");
        return;
    }

    fflush(dst);

    g_len = 0;
    g_need_flush = 0;
}

void dbg_service(void)
{
    if (!g_enabled) return;

    if (g_need_flush) dbg_flush_to_sink();
}

int dbg_is_enabled(void) { return g_enabled; }

void dbg_printf(const char *tag, const char *fmt, ...)
{
    va_list ap;

    if (!g_enabled) return;

    out_timestamp();

    out_char('[');
    out_str(tag);
    out_str("] ");

    va_start(ap, fmt);
    out_vsnprintf(fmt, ap);
    va_end(ap);

    if (!g_in_gfx) fflush(stdout);
}

void dbg_flush(void)
{
    unsigned i;

    if (!g_enabled) return;

    if (g_buf && g_len) {
        for (i = 0; i < g_len; i++) {
            unsigned char c = (unsigned char)g_buf[i];
            fputc(c, stdout);
        }
    }

    fflush(stdout);
    if (g_to_file && g_fp) fflush(g_fp);

    g_len = 0;
}

void dbg_hex(const void far *p, unsigned len)
{
    const unsigned char far *b = (const unsigned char far*)p;
    unsigned i, j;

    if (!g_enabled) return;

    out_str("[HEX] ");
    {
        char tmp[64];
        sprintf(tmp, "ptr=%Fp len=%u\n", (void far*)p, len);
        out_str(tmp);
    }

    for (i = 0; i < len; i += 16) {
        char line[160];
        unsigned n = (len - i > 16) ? 16 : (len - i);
        unsigned pos = 0;

        pos += sprintf(line + pos, "  %04X: ", i);

        for (j = 0; j < 16; j++) {
            if (j < n) pos += sprintf(line + pos, "%02X ", (unsigned)b[i + j]);
            else       pos += sprintf(line + pos, "   ");
        }

        pos += sprintf(line + pos, " |");
        for (j = 0; j < n; j++) {
            unsigned char c = b[i + j];
            line[pos++] = (c >= 32 && c < 127) ? (char)c : '.';
        }
        line[pos++] = '|';
        line[pos++] = '\n';
        line[pos] = 0;

        out_str(line);
    }

    if (!g_in_gfx) fflush(stdout);
}

void dbg_free(void)
{
    if (g_fp) {
        dbg_flush_to_sink();
        fclose(g_fp);
        g_fp = NULL;
    }

    if (g_buf) free(g_buf);

    g_buf = NULL;
    g_cap = 0;
    g_len = 0;
}

void dbg_hexdump(const char *title, const void far *p, u16 len)
{
    const u8 far *b = (const u8 far*)p;
    u16 i, j;

    DBG("%s @%04X:%04X size=%u\n", title,
        (u16)FP_SEG(b), (u16)FP_OFF(b), (u16)len);

    for (i = 0; i < len; i = (u16)(i + 16)) {
        char line[128];
        u16 pos = 0;

        pos += (u16)sprintf(line + pos, "  +%04X: ", (u16)i);

        for (j = 0; j < 16; ++j) {
            if ((u16)(i + j) < len) pos += (u16)sprintf(line + pos, "%02X ", (u16)b[i + j]);
            else                    pos += (u16)sprintf(line + pos, "   ");
        }

        pos += (u16)sprintf(line + pos, " |");

        for (j = 0; j < 16; ++j) {
            if ((u16)(i + j) < len) {
                u8 c = b[i + j];
                line[pos++] = (char)((c >= 32) ? c : '.');
            } else {
                line[pos++] = ' ';
            }
        }

        line[pos++] = '|';
        line[pos++] = '\n';
        line[pos] = 0;

        DBG("%s", line);
    }
}

void dbg_check_stack(void)
{
    DBGL("STACK: SS=%04X SP=%04X BP=%04X\n",
         (u16)get_ss(),
         (u16)get_sp(),
         (u16)get_bp());
}
