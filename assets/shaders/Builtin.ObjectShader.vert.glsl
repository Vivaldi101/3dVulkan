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

const vec3 small_verts[] = 
{
   vec3(0.0, 0.5f, 1.0f),
   vec3(-0.5f, -0.5f, 1.0f),
   vec3(0.5f, -0.5f, 1.0f),
};

void main()
{
   vec3 pos = small_verts[gl_VertexIndex];
   gl_Position = projection.matrix*vec4(pos, 1.0f);
}