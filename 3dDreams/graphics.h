
#if !defined(_GRAPHICS_H)
#define _GRAPHICS_H

#include "common.h"
#include "assert.h"
#include <math.h>

#define DEG_TO_RAD(degrees) ((degrees) * (3.14159265358979323846f / 180.0f))

__declspec(align(64))	// Align to cache line.
typedef struct { f32 x,y,z; } vec3;

#define vec3_cross(a, b, c) (c).x = (a).y*(b).z - (a).z*(b).y; (c).y = (a).z*(b).x - (a).x*(b).z; (c).z = (a).x*(b).y - (a).y*(b).x;

//static vec3 vec3_cross(const vec3* a, const vec3* b) 
//{
   //vec3 v = {(a)->y*(b)->z - (a)->z*(b)->y, (a)->z*(b)->x - (a)->x*(b)->z, (a)->x*(b)->y - (a)->y*(b)->x};
   //return v;
//}

#define vec3_dot(a, b) (a).x*(b).x + (a).y*(b).y + (a).z*(b).z

static vec3 vec3_sub(const vec3* a, const vec3* b) 
{
   vec3 v = {b->x - a->x, b->y - a->y, b->z - a->z};
   return v;
}

#define vec3_len2(a) vec3_dot((a), (a))
#define vec3_len(a) sqrtf(vec3_len2((a)))

#define vec3_normalize(a) { f32 l = vec3_len((a)); (a).x /= l; (a).y /= l; (a).z /= l;}

// normal and othogonal distance to the origin
typedef struct gr_plane { vec3 n; f32 d; } gr_plane;

typedef struct gr_frustum { gr_plane l,r,t,b,n,f; } gr_frustum;

static void GR_plane_create(gr_plane* plane, const vec3* a, const vec3* b, const vec3* c)
{
   vec3 ab = vec3_sub(a, b);
   vec3 ac = vec3_sub(a, c);
   vec3 n;

   vec3_cross(ab, ac, n);
   vec3_normalize(n);

   plane->n = n;
   plane->d = -vec3_dot(*a, n);
}

static void GR_frustum_create(gr_frustum* frustum, f32 w, f32 h, f32 hfov)
{
   f32 xr, xl, yb, yt;
   f32 z = w / (2.0f*tanf(DEG_TO_RAD(hfov/2.0f)));
   const vec3 origin = {0.0f, 0.0f, 0.0f};
   vec3 vlb, vlt, vrb, vrt, vtl, vtr, vbl, vbr;

   xl = -w / (2*z);
   xr = w / (2*z);
   yb = -h / (2*z);
   yt = h / (2*z);

   // The above were used with an implicit z-viewspace distance of one. We need to negate it for right-handed camera space
   z = -1.0f;

   vlb.x = xl;    vlt.x = xl; 
   vlb.y = yb;    vlt.y = yt;
   vlb.z = z;     vlt.z = z;

   vrb.x = xr;    vrt.x = xr;
   vrb.y = yb;    vrt.y = yt;
   vrb.z = z;     vrt.z = z;

   vtl.x = xl;    vtr.x = xr;
   vtl.y = yt;    vtr.y = yt;
   vtl.z = z;     vtr.z = z;

   vbl.x = xl;    vbr.x = xr;
   vbl.y = yb;    vbr.y = yb;
   vbl.z = z;     vbr.z = z;

   GR_plane_create(&frustum->l, &origin, &vlt, &vlb);
   GR_plane_create(&frustum->r, &origin, &vrb, &vrt);
   GR_plane_create(&frustum->t, &origin, &vtr, &vtl);
   GR_plane_create(&frustum->b, &origin, &vbl, &vbr);
}

#endif
