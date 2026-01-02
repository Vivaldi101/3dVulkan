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

   vec4 albedo = vec4(1.0, 1.0, 1.0, 1);
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

   vec3 sun_dir = normalize(vec3(0, 1, 0));
   vec3 N = world_normal;
   
   // Small offset to avoid self-intersection
   float epsilon = 0.0005;
   vec3 ray_origin = in_wp + N * epsilon;
   
   // Lambertian term
   float lambert_term = max(dot(N, sun_dir), 0.0);
   
   // Initialize and cast the shadow ray
   rayQueryEXT rq;
   rayQueryInitializeEXT(
       rq,
       tlas,
       gl_RayFlagsTerminateOnFirstHitEXT, // stop at first hit
       0xff,                               // instance mask (all)
       ray_origin,                         // ray origin with offset
       epsilon,                             // tMin matching offset
       sun_dir,                             // ray direction
       100.0                               // tMax
   );
   rayQueryProceedEXT(rq);
   
   // Visibility factor: 1 = fully lit, 0.1 = in shadow
   bool hit = (rayQueryGetIntersectionTypeEXT(rq, true) != gl_RayQueryCommittedIntersectionNoneEXT);
   float visibility = hit ? 0.1 : 1.0;

       float shininess = mix(128.0, 4.0, metal_roughness); // rough = wide lobe

       //vec3 light_pos = vec3(0.0, 4.0, 0.0);
       vec3 light_pos = in_camera_pos;
       //vec3 light_dir = -in_camera_pos;
       //vec3 light_dir = normalize(vec3(0.0, -1.0, 0.0));

       vec3 V = normalize(in_camera_pos - in_wp);
       vec3 L = normalize(light_pos - in_wp);
       //vec3 L = -light_dir;
       vec3 H = normalize(L + V);                        // half vector for GGX / Blinn

       float blinn_phong = pow(max(dot(N, H), 0.0), shininess);
       vec3 specular = blinn_phong * vec3(0.35);
       //vec3 specular = vec3(blinn_phong);
       
       float lambert = max(dot(N, L), 0.0);

       vec3 ambient = vec3(0, 0, 0);

       vec3 diffuse = albedo.rgb * lambert * visibility;

       vec3 linear_color = diffuse + emissive + specular + ambient;
       out_color = vec4(linear_color, albedo.a);
#endif
}
