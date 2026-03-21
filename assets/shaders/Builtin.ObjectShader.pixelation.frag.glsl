#version 460

#include "common.glsl"

layout(set = 0, binding = 2) uniform sampler2D scene_texture;

layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 out_color;

const vec2 resolution = vec2(1920.0, 1080.0); // hardcoded for now
const float pixel_size = 4.0; // hardcoded for now

void main()
{
vec3 color = texture(scene_texture, in_uv).rgb;

if (globals.do_postprocess == 1)
{
    vec2 pixel_count = floor(resolution / pixel_size);
    vec2 pixel_uv = (floor(in_uv * pixel_count) + 0.5) / pixel_count;

    color = texture(scene_texture, pixel_uv).rgb;

    // --- DITHER ---
    int x = int(mod(gl_FragCoord.x, 4.0));
    int y = int(mod(gl_FragCoord.y, 4.0));
    int index = x + y * 4;

    float[16] matrix = float[](
         0.0,  8.0,  2.0, 10.0,
        12.0,  4.0, 14.0,  6.0,
         3.0, 11.0,  1.0,  9.0,
        15.0,  7.0, 13.0,  5.0
    );

    float dither = matrix[index] / 16.0;

    // --- COLOR QUANT ---
    float levels = 6.0;
    color += (dither - 0.5) / levels * 0.6;
    color = floor(color * levels) / levels;

    // --- SCANLINES ---
    float scanline = sin(gl_FragCoord.y * 3.14159);
    color *= 0.9 + 0.1 * scanline;

    // --- VIGNETTE ---
    vec2 uv = in_uv * 2.0 - 1.0;
    float vignette = 1.0 - dot(uv, uv) * 0.3;
    color *= vignette;

    color = clamp(color, 0.0, 1.0);
}

out_color = vec4(color, 1.0);
}
