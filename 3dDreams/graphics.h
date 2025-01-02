
#if !defined(_GRAPHICS_H)
#define _GRAPHICS_H

#include "common.h"
#include "assert.h"
#include <math.h>

enum
{
   G_PLANE_FRONT, G_PLANE_BACK, G_PLANE_ON, G_PLANE_SPLIT
};

#define DEG_TO_RAD(degrees) ((degrees) * (3.14159265358979323846f / 180.0f))

cache_align typedef struct { f32 x,y,z; } vec3;

// TODO: Also maybe we should remove these defines and just do functions..
#define vec3_cross(a, b, c) (c).x = (a).y*(b).z - (a).z*(b).y; (c).y = (a).z*(b).x - (a).x*(b).z; (c).z = (a).x*(b).y - (a).y*(b).x;

#if 0
static vec3 vec3_cross(const vec3* a, const vec3* b) 
{
 vec3 v = {(a)->y*(b)->z - (a)->z*(b)->y, (a)->z*(b)->x - (a)->x*(b)->z, (a)->x*(b)->y - (a)->y*(b)->x};
 return v;
}
#endif

// Should move the vector math to common math file

#define vec3_dot(a, b) (a).x*(b).x + (a).y*(b).y + (a).z*(b).z

static vec3 vec3_sub(const vec3* a, const vec3* b) 
{
   vec3 v = {b->x - a->x, b->y - a->y, b->z - a->z};
   return v;
}

static vec3 vec3_add(const vec3* a, const vec3* b) 
{
   vec3 v = {b->x + a->x, b->y + a->y, b->z + a->z};
   return v;
}

#define vec3_len2(a) vec3_dot((a), (a))
#define vec3_len(a) sqrtf(vec3_len2((a)))

#define vec3_normalize(a) { f32 l = vec3_len((a)); (a).x /= l; (a).y /= l; (a).z /= l;}

// normal and othogonal distance to the origin
typedef struct g_plane { vec3 n; f32 d; } g_plane;

typedef struct g_frustum { g_plane l,r,t,b,n,f; } g_frustum;

static int G_plane_classify_vertex_side(g_plane* plane, f32 vertex[3])
{
   // normal plane equation
   f32 p = plane->n.x*vertex[0] + plane->n.y*vertex[1] + plane->n.z*vertex[2] + plane->d;

   if(p > 0.0f) return G_PLANE_FRONT;
   if(p < 0.0f) return G_PLANE_BACK;
   return G_PLANE_ON;
}

static int G_plane_classify_face_side(g_plane* plane, f32 face[3*3])
{
   int fn = 0, bn = 0, i;
   for(i = 0; i < 3; ++i) // per vertex of face
   {
      switch(G_plane_classify_vertex_side(plane, face + (i*3)))
      {
      case G_PLANE_FRONT:
         ++fn; 
         break;
      case G_PLANE_BACK:
         ++bn; 
         break;
      case G_PLANE_ON:
         ++fn; ++bn; 
         break;
      }
   }
   if(fn == 3) return G_PLANE_FRONT;
   if(bn == 3) return G_PLANE_BACK;
   return G_PLANE_SPLIT;
}

static void G_plane_create(g_plane* plane, const vec3* a, const vec3* b, const vec3* c)
{
   vec3 ab = vec3_sub(a, b);
   vec3 ac = vec3_sub(a, c);
   vec3 n;

   vec3_cross(ab, ac, n);
   vec3_normalize(n);

   plane->n = n;
   plane->d = -vec3_dot(*a, n);
}

static void G_frustum_create(g_frustum* frustum, f32 w, f32 h, f32 hfov)
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

   G_plane_create(&frustum->l, &origin, &vlt, &vlb);
   G_plane_create(&frustum->r, &origin, &vrb, &vrt);
   G_plane_create(&frustum->t, &origin, &vtr, &vtl);
   G_plane_create(&frustum->b, &origin, &vbl, &vbr);
}

#endif
