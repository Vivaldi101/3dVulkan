#version 450

layout(location = 0) in vec3 in_position;
layout(location = 0) out vec4 out_color;

void main() 
{
    out_color = vec4(in_position.r*0.45f+0.5f, in_position.g*0.75f+0.5f, in_position.b*0.65f+0.5f, 1.0);
}
