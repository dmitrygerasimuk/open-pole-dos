// cstr.c

#include "cstr.h"
#include "globals.h"

void cstr_copy(char *dst, u16 cap, const char *src)
{
    unsigned i = 0u;

    // dst обязателен, cap должен включать место под '\0'
    if (!dst || cap == 0u) return;

  
    if (!src) { dst[0] = '\0'; return; }

    // копируем максимум cap-1 байт, чтобы всегда завершить '\0'
    while (src[i] && (u16)(i + 1u) < cap) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

void cstr_cat(char *dst, u16 cap, const char *src)
{
    u16 dn = 0u;
    unsigned i  = 0u;

    // dst обязателен, cap должен включать место под '\0'
    if (!dst || cap == 0u) return;

   
    if (!src) return;

    // ищем конец dst, но не выходим за cap
    while (dn < cap && dst[dn]) ++dn;

    // если dst не был корректно завершён - принудительно завершаем
    if (dn >= cap) { dst[cap - 1u] = '\0'; return; }

    // дописываем максимум cap-1, чтобы сохранить '\0'
    while (src[i] && (u16)(dn + 1u) < cap) {
        dst[dn++] = src[i++];
    }
    dst[dn] = '\0';
}

 
 
void cstr_cat_s16(char *dst, u16 cap, s16 v)
{
    char buf[8];            
    char *p = buf + sizeof(buf);
    s32 t = (s32)v;
    u16 u;

    if (!dst || cap == 0u) return;

    *--p = '\0';

    // переводим в модуль и запоминаем знак
    if (t < 0) {
        t = -t;
        u = (u16)t;
        *--p = '-';
    } else {
        u = (u16)t;
    }

    // формируем цифры с конца
    do {
        *--p = (char)('0' + (u % 10u));
        u /= 10u;
    } while (u);

    cstr_cat(dst, cap, p);
}

// Копирует C-строку в глобальный g_buffer_string с \0
void write_to_g_buffer_str(const char *s)
{
    unsigned i = 0u;

    // NULL трактуем как пустую строку
    if (!s) { g_buffer_string[0] = 0; return; }

    //  sizeof-1, чтобы оставить место под \0
    while (s[i] && i < (u16)(sizeof(g_buffer_string) - 1u)) {
        g_buffer_string[i] = (u8)s[i];
        ++i;
    }
    g_buffer_string[i] = 0;
}

// Склеивает a + b в глобальный g_buffer_string  c \0
void concat_to_g_buffer_str(const char *a, const char *b)
{
    unsigned i = 0u;
    u16 j = 0u;

     
    if (!a) a = "";
    if (!b) b = "";

    while (a[i] && i < (u16)(sizeof(g_buffer_string) - 1u)) {
        g_buffer_string[i] = (u8)a[i];
        ++i;
    }

    while (b[j] && i < (u16)(sizeof(g_buffer_string) - 1u)) {
        g_buffer_string[i++] = (u8)b[j++];
    }

    g_buffer_string[i] = 0;
}
