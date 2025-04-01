#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform Projection
{
    mat4 matrix;
} projection;

const vec4 verts[] = 
{
   vec4(0.0, 1.0f, 1.0f, 1.0f),
   vec4(-1.0f, -1.0f, 1.0f, 1.0f),
   vec4(1.0f, -1.0f, 1.0f, 1.0f),
};

void main()
{
   gl_Position = projection.matrix*verts[gl_VertexIndex];
}