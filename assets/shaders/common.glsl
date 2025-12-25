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
    bool draw_ground_plane;
    bool draw_normals;
} globals;

#define RAYTRACE 1
#define DEBUG 0

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

#endif
