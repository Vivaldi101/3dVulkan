#version 460

#extension GL_EXT_mesh_shader : require
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int32 : require
#extension GL_ARB_shading_language_include: require
#extension GL_ARB_shader_draw_parameters : require
#extension GL_GOOGLE_include_directive: require

#include "mesh.h"
#include "common.glsl"

layout(local_size_x = 1) in;
layout(triangles) out;

// Adjust these limits if needed
layout(max_vertices = 256, max_primitives = 256) out;

const uint SUBDIV = 11;

// output to fragment shader
layout(location = 0) out vec3 out_world_pos[];
layout(location = 1) out vec4 out_color[];

// water quad in world positions
const vec3 ground_plane[4] = vec3[](
    vec3(-25.0, 1.0, -25.0),
    vec3(-25.0, 1.0,  25.0),
    vec3( 25.0, 1.0, -25.0),
    vec3( 25.0, 1.0,  25.0)
);

// bilinear interpolation
vec3 quad_lerp(float u, float v)
{
    vec3 a = mix(ground_plane[0], ground_plane[2], u);
    vec3 b = mix(ground_plane[1], ground_plane[3], u);
    return mix(a, b, v);
}

float wave(vec2 xz, float time)
{
    float base = sin(xz.x*0.7 + time) * cos(xz.y*0.4 + time) * 0.3;
    float detail = sin(xz.x*2.0 + sin(xz.y*1.5 + time)) * 0.15;
    return base + detail;
}

uint32_t hash_index(uint32_t a)
{
   a = (a+0x7ed55d16) + (a<<12);
   a = (a^0xc761c23c) ^ (a>>19);
   a = (a+0x165667b1) + (a<<5);
   a = (a+0xd3a2646c) ^ (a<<9);
   a = (a+0xfd7046c5) + (a<<3);
   a = (a^0xb55a4f09) ^ (a>>16);

   return a;
}

void main()
{
    int draw_ID = gl_DrawIDARB;

    uint mi = gl_WorkGroupID.x;    // global meshlet index
    uint ti = gl_LocalInvocationID.x;     // thread index

#if DEBUG
    uint h = hash_index(mi);
    vec3 meshlet_color = vec3(float(h & 255), float((h >> 8) & 255), float((h >> 16) & 255)) / 255.f;
#endif

    uint vertCount = (SUBDIV + 1) * (SUBDIV + 1);
    uint triCount  = SUBDIV * SUBDIV * 2;

    SetMeshOutputsEXT(vertCount, triCount);

    float time = float(globals.time);

    // generate vertices
    uint vid = 0;
    for (uint y = 0; y <= SUBDIV; ++y)
    {
        for (uint x = 0; x <= SUBDIV; ++x)
        {
            float u = float(x) / float(SUBDIV);
            float v = float(y) / float(SUBDIV);

            vec3 pos = quad_lerp(u, v);

            // water displacement
            pos.y += wave(pos.xz + vec2(1.0,1.0), time);
            pos.x += wave(pos.yz + vec2(0.0,1.0), time);
            pos.z += wave(pos.xy + vec2(1.0,0.0), time);

            mat4 view_proj = globals.projection * globals.view;
            gl_MeshVerticesEXT[vid].gl_Position = view_proj * vec4(pos, 1);
            out_world_pos[vid] = pos;

#if DEBUG
      out_color[vid] = vec4(meshlet_color, 1.0);
#endif
            vid++;
        }
    }

    // generate triangles
    uint pid = 0;
    for (uint y = 0; y < SUBDIV; ++y)
    {
        for (uint x = 0; x < SUBDIV; ++x)
        {
            uint i0 =  y      * (SUBDIV + 1) + x;
            uint i1 =  y      * (SUBDIV + 1) + x + 1;
            uint i2 = (y + 1) * (SUBDIV + 1) + x;
            uint i3 = (y + 1) * (SUBDIV + 1) + x + 1;

            gl_PrimitiveTriangleIndicesEXT[pid++] = uvec3(i0, i2, i1);
            gl_PrimitiveTriangleIndicesEXT[pid++] = uvec3(i1, i2, i3);
        }
    }
}
