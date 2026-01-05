#version 460

#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int32 : require
#extension GL_ARB_shader_draw_parameters : require
#extension GL_EXT_nonuniform_qualifier : require

#include "mesh.h"
#include "common.glsl"

#if RAYTRACE
#extension GL_EXT_ray_query : require
layout(set = 0, binding = 3) uniform accelerationStructureEXT tlas;
#endif

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec4 in_color;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_wp; // world position
layout(location = 3) in vec3 in_normal;
layout(location = 4) flat in uint in_draw_ID;
layout(location = 5) in vec4 in_tangent;
layout(location = 6) in vec3 in_camera_pos;

layout(set = 1, binding = 0)
uniform sampler2D textures[];

layout(set = 0, binding = 2) readonly buffer mesh_draw_block
{
   mesh_draw draws[];
};

void main()
{
#if DEBUG
   out_color = in_color;
#else
    mesh_draw draw = draws[in_draw_ID];
   vec3 light_color = vec3(1.f);

   //float t = (sin(float(globals.time)) + 1) * 0.5f; //  map from [-1, 1] to [0, 2] and scale down by 2 to [0, 1]
   float t = sin(float(globals.time));
   float p = 0.0f;   // dont go below this
   float q = 0.5f;   // dont go above this
   float shadow = mix(p, q, t);

   vec4 albedo = vec4(1.0, 1.0, 1.0, 1);
   vec3 emissive = vec3(0, 0, 0);
   vec3 world_normal = vec3(0.0, 0.0, 1.0);
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
      metal_roughness = texture(textures[draw.metal], in_uv).g;

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
   else
      world_normal = normalize(in_normal);

   vec3 N = world_normal;
   float epsilon = 0.05;
   vec3 ray_origin = in_wp + N * epsilon;
   vec3 sun_dir = normalize(vec3(shadow,1,shadow));
   
   rayQueryEXT rq;
   rayQueryInitializeEXT(
       rq,
       tlas,
       gl_RayFlagsTerminateOnFirstHitEXT,
       0xff,                            
       ray_origin,                     
       epsilon,                       
       sun_dir,                      
       1000.0                       
   );
   rayQueryProceedEXT(rq);
   
   bool hit = (rayQueryGetIntersectionTypeEXT(rq, true) != gl_RayQueryCommittedIntersectionNoneEXT);

   float visibility = hit ? 0.15 : 1.0;
   vec3 L = sun_dir;
   float lambert = max(dot(N, L), 0.0);

       lambert *= visibility;

       float shininess = mix(128.0, 4.0, metal_roughness);

       vec3 V = normalize(in_camera_pos - ray_origin);
       vec3 H = normalize(L + V);

       float blinn_phong = pow(max(dot(N, H), 0.0), shininess);
       vec3 specular = blinn_phong * vec3(0.25);
       
       vec3 ambient = vec3(1, 1, 1) * 0;

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
#endif
}
