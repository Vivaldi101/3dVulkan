#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform Projection
{
    mat4 matrix;
} projection;

const vec3 verts[] = 
{
   vec3(0.0, 1.0f, 1.0f),
   vec3(-1.0f, -1.0f, 1.0f),
   vec3(1.0f, -1.0f, 1.0f),
};

void main()
{
   gl_Position = projection.matrix*vec4(verts[gl_VertexIndex], 1.0f);
}