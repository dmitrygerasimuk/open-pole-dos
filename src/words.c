// words.c 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "words.h"
#include "debuglog.h"
#include "globals.h"
#include "cstr.h"
#include "random.h"

#define POLE_OVL_NAME   "pole.ovl"
#define REC_SIZE        0x15   // Reset(..., 0x15) -> record size 21 bytes
 
static void rec_to_cstr(char *dst, unsigned cap, const unsigned char *rec, unsigned rec_sz)
{
    unsigned i, n;

    if (!dst || cap == 0) return;

    dst[0] = '\0';
    if (!rec || rec_sz == 0) return;

    //  Pascal string: [len][chars...]
    if (rec[0] <= (unsigned char)(rec_sz - 1)) {
        n = (unsigned)rec[0];
        if (n > cap - 1) n = cap - 1;

        for (i = 0; i < n; ++i) dst[i] = (char)rec[1 + i];
        dst[n] = '\0';
        return;
    }

    // Иначе: копируем как C-string  
    n = rec_sz;
    if (n > cap - 1) n = cap - 1;

    for (i = 0; i < n; ++i) {
        if (rec[i] == 0) break;
        dst[i] = (char)rec[i];
    }
    dst[i] = '\0';

    // уберем пробелы справа
    while (i > 0 && (unsigned char)dst[i - 1] == (unsigned char)' ') {
        dst[--i] = '\0';
    }
}

static void cstr_upper_decypher_tp(char *s)
{
    unsigned char *p = (unsigned char *)s;
    if (!p) return;

    while (*p) {
        unsigned char c = *p;
            c = (unsigned char)(c - 0x20);
        *p++ = c;
    }
}

int words_load_word_and_randomize(void)
{
    FILE *fp = NULL;
    unsigned char rec[REC_SIZE];
    u16 count = 0;
    u16 pick = 0;
    u16 rindex;
    unsigned i;

    DBG("words: loadWordAndRandomize() start\n");

    fp = fopen(POLE_OVL_NAME, "rb");
    if (!fp) {
        DBG("words: ERROR: can't open %s\n", POLE_OVL_NAME);
        return -1;
    }

    // Read first record -> count string
    if (fread(rec, 1, REC_SIZE, fp) != REC_SIZE) {
        DBG("words: ERROR: can't read header record (size=%u)\n", (unsigned)REC_SIZE);
        fclose(fp);
        return -2;
    }

    {
        char tmp[32];
        rec_to_cstr(tmp, sizeof(tmp), rec, REC_SIZE);
        count = my_atoi(tmp);
        DBG("words: header count string='%s' -> count=%u\n", tmp, (unsigned)count);
    }

    if (count == 0) {
        DBG("words: ERROR: count=0 (bad file?)\n");
        fclose(fp);
        return -3;
    }

    // уникальный рандомный номер который еще не использовали
    for (;;) {
        pick = (u16)(tp_random(count) + 1u);

        if (pick == g_used_word_idx[0]) continue;
        if (pick == g_used_word_idx[1]) continue;
        if (pick == g_used_word_idx[2]) continue;

        break;
    }

    rindex = (g_round_wins < 3 ) ? g_round_wins : 0 ;
    g_used_word_idx[rindex] = pick;

    DBG("words: picked index=%u (g_round_wins=%u -> slot=%u)\n",
        (unsigned)pick, (unsigned)g_round_wins, (unsigned)rindex);

    // гоним по всему файлу 
    for (i = 1u; i <= pick; ++i) { // паскалевский цикл с 1
        // Word
        if (fread(rec, 1, REC_SIZE, fp) != REC_SIZE) {
            DBG("words: ERROR: can't read word record at i=%u\n", (unsigned)i);
            fclose(fp);
            return -4;
        }
        rec_to_cstr(g_current_game_word, sizeof(g_current_game_word), rec, REC_SIZE);

        // Theme
        if (fread(rec, 1, REC_SIZE, fp) != REC_SIZE) {
            DBG("words: ERROR: can't read theme record at i=%u\n", (unsigned)i);
            fclose(fp);
            return -5;
        }
        rec_to_cstr(g_buffer_string_ovl, sizeof(g_buffer_string_ovl), rec, REC_SIZE);
    }

    fclose(fp);

    DBG("words: raw word='%s'\n", g_current_game_word);
    DBG("words: raw theme='%s'\n", g_buffer_string_ovl);

    //  sub 0x20 в оригинале  
    cstr_upper_decypher_tp(g_current_game_word);
    cstr_upper_decypher_tp(g_buffer_string_ovl);

    DBG("words: final word='%s'\n", g_current_game_word);
    DBG("words: final theme='%s'\n", g_buffer_string_ovl);

    return 0;
}
