#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 out_color;
layout(location = 0) in vec4 in_pos;

void main() 
{
    float depth = gl_FragCoord.z; // This comes from NDC depth, mapped to [0,1]
    if(depth > 0.5f)
      out_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    else
      out_color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}
