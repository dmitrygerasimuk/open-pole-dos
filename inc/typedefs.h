#ifndef TYPES_H
#define TYPES_H

// c99 or later 
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
  #include <stdint.h>
  typedef uint8_t  u8;
  typedef uint16_t u16;
  typedef uint32_t u32;
  typedef int8_t   s8;
  typedef int16_t  s16;
  typedef int32_t  s32;
#else
 // fallback old compiler
  typedef unsigned char  u8;
  typedef unsigned short u16;
  typedef unsigned long  u32;
  typedef signed char    s8;
  typedef signed short   s16;
  typedef signed long    s32;
#endif

// Compile-time size checks (C89/C90-friendly) 
typedef char u8_must_be_1[(sizeof(u8)  == 1) ? 1 : -1];
typedef char u16_must_be_2[(sizeof(u16) == 2) ? 1 : -1];
typedef char u32_must_be_4[(sizeof(u32) == 4) ? 1 : -1];
typedef char s8_must_be_1[(sizeof(s8)  == 1) ? 1 : -1];
typedef char s16_must_be_2[(sizeof(s16) == 2) ? 1 : -1];
typedef char s32_must_be_4[(sizeof(s32) == 4) ? 1 : -1];
// if something goes wrong compile will throw an error
#endif
