#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int32 : require
#extension GL_ARB_shader_draw_parameters : require

#include "mesh.h"
#include "common.glsl"

vec3 quad[4] = vec3[]
(
    vec3(-100.0f, -1.25f, -100.0f),  // top-left
    vec3(-100.0f, -1.25f,  100.0f),  // bottom-left
    vec3(100.0f,  -1.25f,   -100.0f), // top-right
    vec3(100.0f,  -1.25f,   100.0f)   // bottom-right
);

layout(set = 0, binding = 0) readonly buffer vertex_block
{
   vertex verts[];
};

layout(set = 0, binding = 1) readonly buffer mesh_draw_block
{
   mesh_draw draws[];
};

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec3 out_world_frag_pos;
layout(location = 2) out vec2 out_uv;
layout(location = 3) flat out uint out_draw_ID;
layout(location = 4) out vec4 out_tangent;

void main()
{
    int draw_ID = gl_DrawIDARB;

    vertex v;
    vec3 pos = vec3(0.f);
    vec3 normal = vec3(0.f);
    vec2 uv = vec2(0.f);
    vec4 world_pos = vec4(0.f);
    vec4 tangent = vec4(0.f);

    if(!globals.draw_ground_plane)
    {
        pos = vec3(verts[gl_VertexIndex].vx, verts[gl_VertexIndex].vy, verts[gl_VertexIndex].vz) * 1.f;
        normal = vec3(verts[gl_VertexIndex].nx, verts[gl_VertexIndex].ny, verts[gl_VertexIndex].nz);
        tangent = vec4(verts[gl_VertexIndex].tx, verts[gl_VertexIndex].ty, verts[gl_VertexIndex].tz, verts[gl_VertexIndex].tw);
        uv = vec2(verts[gl_VertexIndex].tu, verts[gl_VertexIndex].tv);

        world_pos = draws[draw_ID].world * vec4(pos, 1.0);
    }
    else
    {
        pos = quad[gl_VertexIndex];
        world_pos = vec4(pos, 1.0);
    }

    gl_Position = globals.projection * globals.view * world_pos;

    // Decode normal and transform to world space using inverse transpose
    normal = (normal - 127.5) / 127.5;
    tangent = (tangent - 127.5) / 127.5;

    mat3 normal_matrix = transpose(inverse(mat3(draws[draw_ID].world)));
    vec3 world_normal = normalize(normal_matrix * normal);
    vec3 world_tangent = normalize(normal_matrix * tangent.xyz);

    out_tangent = vec4(world_tangent, tangent.w);

    out_normal = world_normal;
    out_world_frag_pos = world_pos.xyz;
    out_uv = uv;
    out_draw_ID = draw_ID;
}
