#if !defined(_COMMON_H)
#define _COMMON_H

layout(push_constant) uniform push_constants_block
{
    mat4 projection;
    mat4 view;
    double time;
    float near;
    float far;
    float ar;
    bool draw_normals;
    bool do_postprocess;
} globals;

vec3 color_to_srgb(vec3 color)
{
   float gamma = 2.2;
   return pow(color, vec3(1.0 / gamma));
}

vec3 color_to_linear(vec3 color)
{
   float gamma = 2.2;
   return pow(color, vec3(gamma));
}

struct plane
{
   vec3 n;  // any point x on the plane satisfy dot(n, x) = d
   float d; // othogonal distance from the origin dot(n, p) = d for a fixed point p on the plane
};

float plane_distance(plane pl, vec3 v)
{
   return dot(pl.n, v) - pl.d;
}

plane plane_create(vec3 p0, vec3 p1, vec3 p2)
{
   plane p;

   p.n = normalize(cross(p1 - p0, p2 - p0));
   p.d = dot(p.n, p0);

   return p;
}

plane plane_normal_create(vec3 n, vec3 p0)
{
   plane p;

   p.n = normalize(n);
   p.d = dot(p.n, p0);

   return p;
}

#define RAYTRACE 1
#define DEBUG 0   // TODO: make this toggleable

#endif
