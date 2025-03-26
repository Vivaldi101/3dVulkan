#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;

layout(set = 0, binding = 0) uniform global_uniform
{
    mat4 proj;
    mat4 view;
} global_ubo;

mat4 scale = mat4(
    2.0, 0.0, 0.0, 0.0,
    0.0, 2.0, 0.0, 0.0,
    0.0, 0.0, 2.0, 0.0,
    0.0, 0.0, 0.0, 1.0   // Keep w=1 to avoid unintended perspective effects
);

void main()
{
   gl_Position = global_ubo.proj * global_ubo.view * vec4(in_position, 1.0);
}