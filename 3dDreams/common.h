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

typedef unsigned char byte;

typedef float f32;
typedef double f64;

typedef int32_t fp;

#define pre(p)  assert(p)
#define post(p) assert(p)
#define inv(p)  assert(p)

#define iff(p, q) assert(p == q)
#define implies(p, q) assert(!(p) || (q))

typedef enum {false, true} bool;

#endif
