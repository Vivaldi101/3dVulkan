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
    vec3 light_color = vec3(1.f);
    if(!globals.draw_ground_plane)
    {
       mesh_draw draw = draws[in_draw_ID];
   
       vec4 albedo = vec4(1.0, 1.0, 1.0, 1);
       vec3 emissive = vec3(0.0);
       vec3 world_normal = vec3(0.0, 0.0, 1.0);

       if(draw.albedo != -1)
         albedo = texture(textures[draw.albedo], in_uv).rgba;

       if(albedo.a < 0.5) discard;
   
       if(draw.emissive != -1)
         emissive = texture(textures[draw.emissive], in_uv).rgb;
   
       if(draw.normal != -1)
       {
         vec3 normal_map = texture(textures[draw.normal], in_uv).rgb;
         normal_map = normalize(normal_map * 2.0 - 1.0); // Remap from [0,1] to [-1,1]

         vec3 normal = normalize(in_normal);
         vec3 tangent = normalize(in_tangent.xyz);
         vec3 bitangent = cross(normal, tangent) * in_tangent.w;
         bitangent = normalize(bitangent);

         world_normal = normalize(normal_map.x * tangent.xyz + normal_map.y * bitangent + normal_map.z * normal);
       }

       vec3 ambient_color = vec3(1.0); // scale the light to reduce brightness

       vec3 sun_dir = normalize(vec3(1, 1, 1));
       float diffuse_factor = max(dot(world_normal, sun_dir), 0.0);
       
       // Combine diffuse + emissive in linear space
       vec3 color_linear = albedo.rgb * diffuse_factor * ambient_color + emissive;

       // Apply gamma correction (linear -> sRGB)
       //vec3 color_srgb = pow(color_linear, vec3(1.0 / 2.2));
       vec3 color_srgb = color_linear;

       out_color = vec4(color_srgb, albedo.a);
    }
    else
    {
      // draw the ground plane with some glow
      vec3 procedural_center = vec3(10.0, 7.0, 0.0);
      float glow_intensity = 3.0;
      
      float dist = length(in_world_frag_pos - procedural_center);
      
      // bright center + longer aura
      float core  = exp(-dist * .15);        // small, bright dot
      float halo  = exp(-dist * 3.05);       // larger glow spread
      float falloff = core + 0.9 * halo;     // adjust 0.5 for halo strength
      
      vec3 glow_color = vec3(0.95, 0.78, 0.0) * falloff * glow_intensity; // amber yellow
      //vec3 base_color = vec3(71.0/255, 58.0/255, 10.0/255);
      vec3 base_color = vec3(0.1176, 0.1176, 0.1176);
      vec3 final_color = clamp(base_color + glow_color, 0.0, 1.0);
      
      out_color = vec4(final_color, 1.0);
    }
}
