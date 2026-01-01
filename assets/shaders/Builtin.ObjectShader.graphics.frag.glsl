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
layout(location = 1) in vec3 in_world_pos;
layout(location = 2) in vec2 in_uv;
layout(location = 3) flat in uint in_draw_ID;
layout(location = 4) in vec4 in_tangent;
layout(location = 5) in vec3 in_camera_pos;

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
       mesh_draw draw = draws[in_draw_ID];
   
       vec4 albedo = vec4(1.0, 0.0, 0.0, 1);
       vec3 emissive = vec3(0, 0, 0);
       vec3 world_normal = vec3(0.0, 0.0, 0.0);
       float metal_roughness = 0.f;

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

       if(draw.metal != -1)
          metal_roughness = texture(textures[draw.emissive], in_uv).g;
   
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

       float shininess = mix(128.0, 4.0, metal_roughness); // rough = wide lobe

       //vec3 light_pos = vec3(0.0, 4.0, 0.0);
       vec3 light_pos = in_camera_pos;
       //vec3 light_dir = -in_camera_pos;
       //vec3 light_dir = normalize(vec3(0.0, -1.0, 0.0));

       vec3 V = normalize(in_camera_pos - in_world_pos);
       vec3 L = normalize(light_pos - in_world_pos);
       //vec3 L = -light_dir;
       vec3 H = normalize(L + V);                        // half vector for GGX / Blinn

       vec3 N = world_normal;
       float blinn_phong = pow(max(dot(N, H), 0.0), shininess);
       vec3 specular = blinn_phong * vec3(0.8);
       //vec3 specular = vec3(blinn_phong);
       
       float lambert = max(dot(N, L), 0.0);

       vec3 ambient = vec3(0, 0, 0);

       vec3 diffuse = albedo.rgb * lambert;

       vec3 linear_color = diffuse + emissive + specular + ambient;
       out_color = vec4(linear_color, albedo.a);

       if(globals.draw_normals)
       {
          // normal debugging
          vec3 linear_color = world_normal;
          out_color = vec4(linear_color * 0.5 + 0.5, 1);
       }

       out_color.xyz = color_to_srgb(out_color.xyz);
}
