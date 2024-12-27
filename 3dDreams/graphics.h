
#if !defined(_GRAPHICS_H)
#define _GRAPHICS_H

#include "common.h"
#include "assert.h"

typedef struct vec3 { f32 x,y,z; } vec3;

#define vec3_cross(a, b, c) c.x = a.y*b.z - a.z*b.y; \
   c.y = a.z*b.x - a.x*b.z; \
   c.z = a.x*b.y - a.y*b.x;

#define vec3_dot(a, b) a.x*b.x + a.y*b.y + a.z*b.z

#define vec3_len2(a) vec3_dot(a, a)

// normal and othogonal distance to the origin
typedef struct gr_plane { vec3 n; f32 d; } gr_plane;

typedef struct gr_frustum { gr_plane l,r,t,b,n,f; } gr_frustum;

static void GR_frustum_create(gr_frustum* frustum)
{
   assert(frustum);
}

#endif
