#version 460

#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int32 : require
#extension GL_ARB_shader_draw_parameters : require
#extension GL_EXT_nonuniform_qualifier : require

#include "mesh.h"
#include "common.glsl"

layout(location = 0) out vec4 out_color;
layout(location = 0) in vec3 in_world_pos;
layout(location = 1) in vec4 in_color;

void main()
{
   // draw the ground plane with water-like glow
       vec3 procedural_center = vec3(0.0, 0.0, 0.0);
       float glow_intensity = 3.0;
       
       float dist = length(in_world_pos - procedural_center);
       
       // bright center + longer aura
       float core  = exp(-dist * 0.15);  // small, bright dot
       float halo  = exp(-dist * 3.05);  // larger glow spread
       float falloff = core + 0.9 * halo;
       
       // water-like glow color
       vec3 glow_color = vec3(0.0, 0.6, 0.9) * falloff * glow_intensity; // cyan-blue
       vec3 base_color = vec3(0.05, 0.08, 0.25); // dark blue base for water
       
       vec3 linear_color = clamp(base_color + glow_color, 0.0, 1.0);
       
       // alpha based on time
       float t = (sin(float(globals.time)) + 1) * 0.5f; //  map from [-1, 1] to [0, 2] and scale down by 2 to [0, 1]

       float min_alpha = 0.30f;   // dont go below this
       float max_alpha = 0.325f;   // dont go above this
       float water_alpha = mix(min_alpha, max_alpha, t);

       out_color = vec4(linear_color, water_alpha);

#if DEBUG
   out_color = in_color;
#endif
}
