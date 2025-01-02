#if !defined(_COMMON_H)
#define _COMMON_H

#include <stdint.h>
#include <assert.h>

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

#define pre(p)  assert(p)
#define post(p) assert(p)
#define inv(p)  assert(p)

#define iff(p, q) (p) == (q)
#define implies(p, q) (!(p) || (q))

#define cache_align __declspec(align(64)) 

typedef enum {false, true} bool;

__declspec(align(64))	// Align to cache line.
typedef struct
{
   u32 x, y;
} co_point;

__declspec(align(64))	// Align to cache line.
typedef struct
{
   f32 x, y;
} co_f32_point;

__declspec(align(64))	// Align to cache line.
typedef struct
{
   f32 w, h;
} co_area;

#endif
