#if !defined(_COMMON_H)
#define _COMMON_H

#include <stdint.h>
#include <stddef.h>
#include "hw.h"

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;

typedef int64_t i64;
typedef uint64_t u64;

typedef unsigned char byte;

typedef float f32;
typedef double f64;

typedef int32_t fp;

typedef size_t usize;

#ifdef _DEBUG
#define pre(p)  {if(!(p))hw_message(p)}
#define post(p) {if(!(p))hw_message(p)}
#define inv(p)  {if(!(p))hw_message(p)}
#else
#define pre(p)
#define post(p)
#define inv(p)
#endif

#define iff(p, q) (p) == (q)
#define implies(p, q) (!(p) || (q))

#define cache_align __declspec(align(64))    // assume 64 byte cacheline size.
//#define cache_align 

#define array_count(a) sizeof((a)) / sizeof((a)[0])

#define defer(start, end) \
    for (int _defer = ((start), 0); !_defer; (_defer = 1), (end))

typedef enum {false, true} bool;

cache_align typedef struct
{
   u32 x, y;
} c_point;

cache_align typedef struct
{
   f32 x, y;
} c_f32_point;

cache_align typedef struct
{
   f32 w, h;
} c_area;

#endif
