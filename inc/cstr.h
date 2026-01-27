// cstr.h 

#ifndef CSTR_H_
#define CSTR_H_

#include "typedefs.h"

 
static inline void cstr_clear(char *dst);

// Простой atoi для беззнакового диапазона u16: читает ведущие пробелы, потом цифры.
static inline u16 my_atoi(const char *s);

// Копирование/конкатенация с ограничением по cap.
void cstr_copy(char *dst, u16 cap, const char *src);
void cstr_cat(char *dst, u16 cap, const char *src);
void cstr_cat_s16(char *dst, u16 cap, s16 v);

// Запись/конкатенация в глобальный буфер  
void write_to_g_buffer_str(const char *s);
void concat_to_g_buffer_str(const char *a, const char *b);

static inline void cstr_clear(char *dst)
{
    if (dst) dst[0] = '\0';
}

// Укладывает один символ в буфер out[2] и добавляет '\0'.
 
static inline char *cstr_char_to_str_buf(char out[2], unsigned char ch)
{
    out[0] = (char)ch;
    out[1] = '\0';
    return out;
}

// Перевод i32 в строку в маленький буфер с учётом cap.
 
static inline void i32_to_cstr(char *dst, unsigned cap, int v)
{
    char tmp[16];
    unsigned n = 0;
    int neg = 0;

    if (!dst || cap == 0) return;

    if (v == 0) {
        if (cap >= 2) { dst[0] = '0'; dst[1] = '\0'; }
        else dst[0] = '\0';
        return;
    }

    if (v < 0) { neg = 1; v = -v; }

    // Пишем цифры в обратном порядке во временный буфер 
    while (v > 0 && n < (unsigned)sizeof(tmp)) {
        tmp[n++] = (char)('0' + (v % 10));
        v /= 10;
    }

    // Добавляем знак минус в конец tmp, чтобы потом развернуть вместе с цифрами.
    if (neg && n < (unsigned)sizeof(tmp)) tmp[n++] = '-';

 
    {
        unsigned i = 0;
        while (n > 0 && i + 1 < cap) {
            dst[i++] = tmp[--n];
        }
        dst[i] = '\0';
    }
}

// Преобразует строку в число u16.
 
static inline u16 my_atoi(const char *s)
{
    unsigned v = 0;
    int any = 0;

    if (!s) return 0;

    while (*s && (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')) s++;

    while (*s >= '0' && *s <= '9') {
        any = 1;
        v = v * 10u + (unsigned)(*s - '0');
        s++;
    }

    return any ? (u16)v : 0;
}

#endif // CSTR_H_
