#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 out_color;
layout(location = 0) in vec4 in_pos;

layout(push_constant) uniform Transform
{
    mat4 projection;
    mat4 view;
    mat4 model;
   float near;
   float far;
} transform;

void main() 
{
    float n = transform.near;
    //out_color = vec4(0, n, 0, 1.0f);
    out_color = vec4(0, 1.0f, 0, 1.0f);
}
