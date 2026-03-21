#version 460

#include "common.glsl"

layout(set = 0, binding = 2) uniform sampler2D scene_texture;

layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 out_color;

const vec2 resolution = vec2(1920.0, 1080.0); // hardcoded for now
const float pixel_size = 4.0; // hardcoded for now

void main()
{
    vec3 color = texture(scene_texture, in_uv).rgb; // original

    if(globals.do_postprocess == 1)
    {
      vec2 pixel_count = resolution / pixel_size;
      vec2 pixel_uv = (floor(in_uv * pixel_count) + 0.5) / pixel_count;
      color = texture(scene_texture, pixel_uv).rgb;
    }

    out_color = vec4(color, 1.0);
}
