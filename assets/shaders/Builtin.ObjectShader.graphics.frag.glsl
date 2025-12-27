#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int32 : require
#extension GL_ARB_shader_draw_parameters : require
#extension GL_EXT_nonuniform_qualifier : require

#include "mesh.h"
#include "common.glsl"

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec3 in_world_frag_pos;
layout(location = 2) in vec2 in_uv;
layout(location = 3) flat in uint in_draw_ID;
layout(location = 4) in vec4 in_tangent;

layout(set = 0, binding = 1) readonly buffer mesh_draw_block
{
   mesh_draw draws[];
};

layout(set = 1, binding = 0)
uniform sampler2D textures[];

float ndc_to_linear_z(float ndc_z, float near, float far)
{
   float u = far * near;
   float l = (near - far)*ndc_z + far;

   float linear_z = u / l;

   return linear_z;
}

void main()
{
    if(!globals.draw_ground_plane)
    {
       mesh_draw draw = draws[in_draw_ID];
   
       vec4 albedo = vec4(1.0, 1.0, 1.0, 1);
       vec3 emissive = vec3(0.0);
       vec3 world_normal = vec3(0.0, 0.0, 1.0);

       if(draw.albedo != -1)
       {
          albedo = texture(textures[draw.albedo], in_uv).rgba;
          albedo.xyz = color_to_linear(albedo.xyz);
       }

       if(albedo.a < 0.5) discard;
       
       if(draw.emissive != -1)
       {
          emissive = texture(textures[draw.emissive], in_uv).rgb;
          emissive.xyz = color_to_linear(emissive.xyz);
       }
   
       if(draw.normal != -1)
       {
         vec3 normal_map = texture(textures[draw.normal], in_uv).rgb;
         normal_map = normalize(normal_map * 2.0 - 1.0); // Remap from [0,1] to [-1,1]

         vec3 normal = normalize(in_normal);
         vec3 tangent = normalize(in_tangent.xyz);
         tangent = normalize(tangent - (normal * dot(normal, tangent)));  // Re-orthogonalize

         vec3 bitangent = cross(normal, tangent) * in_tangent.w;
         bitangent = normalize(bitangent);

         world_normal = normalize(normal_map.x * tangent.xyz + normal_map.y * bitangent + normal_map.z * normal);
       }

       vec3 sun_dir = normalize(vec3(0, 1, 0));
       vec3 camera_pos = vec3(0); // TODO: camera as uniform
       vec3 view_dir = normalize(camera_pos - in_world_frag_pos); // camera position in world space

       float lambert_term = max(dot(world_normal, sun_dir), 0.0);

       vec3 ambient = 0.1 * albedo.rgb;

       float visibility = 1.0;
       vec3 linear_color = albedo.rgb * lambert_term * visibility + emissive + ambient;
       out_color = vec4(linear_color, albedo.a);

       if(globals.draw_normals)
       {
          // normal debugging
          vec3 linear_color = world_normal;
          out_color = vec4(linear_color * 0.5 + 0.5, 1);
       }
    }
    else
    {
       // draw the ground plane with water-like glow
       vec3 procedural_center = vec3(0.0, 0.0, 0.0);
       float glow_intensity = 3.0;
       
       float dist = length(in_world_frag_pos - procedural_center);
       
       // bright center + longer aura
       float core  = exp(-dist * 0.15);  // small, bright dot
       float halo  = exp(-dist * 3.05);  // larger glow spread
       float falloff = core + 0.9 * halo;
       
       // water-like glow color
       vec3 glow_color = vec3(0.0, 0.6, 0.9) * falloff * glow_intensity; // cyan-blue
       vec3 base_color = vec3(0.05, 0.08, 0.25); // dark blue base for water
       
       vec3 linear_color = clamp(base_color + glow_color, 0.0, 1.0);
       
       float t = (sin(float(globals.time)) + 1) * 0.5f; //  map from [-1, 1] to [0, 2] and scale down by 2 to [0, 1]

       float min_alpha = 0.25f;   // dont go below this
       float max_alpha = 0.50f;   // dont go above this
       float water_alpha = mix(min_alpha, max_alpha, t);

       out_color = vec4(linear_color, water_alpha);
    }

    out_color.xyz = color_to_srgb(out_color.xyz);
}
