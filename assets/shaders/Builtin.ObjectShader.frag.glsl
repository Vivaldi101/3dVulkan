#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 out_color;
layout(location = 0) in vec4 in_pos;

void main() 
{
    //out_color = vec4(in_pos.x, in_pos.y, in_pos.z, 1.0);
    out_color = vec4(0.0f, 0.0f, 1.0f, 1.0f);
}
