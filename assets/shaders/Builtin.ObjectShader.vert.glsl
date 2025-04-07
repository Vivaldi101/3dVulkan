#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform Transform
{
    mat4 projection;
    mat4 view;
    mat4 model;
} transform;

const vec3 verts[] = 
{
   vec3(0.0, -1.0f, 1.0f),
   vec3(1.0f, 1.0f, 1.0f),
   vec3(-1.0f, 1.0f, 1.0f),
};

const vec3 small_verts[] = 
{
   vec3(0.0, 0.7f, 1.0f),
   vec3(-0.7f, -0.7f, 1.0f),
   vec3(0.7f, -0.7f, 1.0f),
};

vec3 cube_strip[] = {
    vec3(-1.0f, -1.0f, +1.0f),
    vec3(+1.0f, -1.0f, +1.0f),
    vec3(-1.0f, +1.0f, +1.0f),
    vec3(+1.0f, +1.0f, +1.0f),
    vec3(+1.0f, -1.0f, -3.0f),
    vec3(-1.0f, -1.0f, -3.0f),
    vec3(+1.0f, +1.0f, -3.0f),
    vec3(-1.0f, +1.0f, -3.0f),
    vec3(+1.0f, -1.0f, +1.0f),
    vec3(+1.0f, -1.0f, -3.0f),
    vec3(+1.0f, +1.0f, +1.0f),
    vec3(+1.0f, +1.0f, -3.0f),
    vec3(-1.0f, -1.0f, -3.0f),
    vec3(-1.0f, -1.0f, +1.0f),
    vec3(-1.0f, +1.0f, -3.0f),
    vec3(-1.0f, +1.0f, +1.0f),
    vec3(-1.0f, +1.0f, +1.0f),
    vec3(+1.0f, +1.0f, +1.0f),
    vec3(-1.0f, +1.0f, -3.0f),
    vec3(+1.0f, +1.0f, -3.0f),
    vec3(-1.0f, -1.0f, -3.0f),
    vec3(+1.0f, -1.0f, -3.0f),
    vec3(-1.0f, -1.0f, +1.0f),
    vec3(+1.0f, -1.0f, +1.0f) 
};

layout(location = 0) out vec4 out_pos;

void main()
{
   // TODO: View matrix
   vec3 pos = cube_strip[gl_VertexIndex];
   // right-to-left transorm order
   gl_Position = transform.projection * transform.view * transform.model * vec4(pos, 1.0f);
   out_pos = gl_Position;
}