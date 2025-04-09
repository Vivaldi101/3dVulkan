#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform Transform
{
    mat4 projection;
    mat4 view;
    mat4 model;
   float near;
   float far;
} transform;

const vec3 verts[] = 
{
   vec3(0.0, -1.0f, 1.0f),
   vec3(1.0f, 1.0f, 1.0f),
   vec3(-1.0f, 1.0f, 1.0f),
};

vec3 cube_strip[] = {
    vec3(-1.0f, -1.0f, +1.0f),
    vec3(+1.0f, -1.0f, +1.0f),
    vec3(-1.0f, +1.0f, +1.0f),
    vec3(+1.0f, +1.0f, +1.0f),
    vec3(+1.0f, -1.0f, -1.0f),
    vec3(-1.0f, -1.0f, -1.0f),
    vec3(+1.0f, +1.0f, -1.0f),
    vec3(-1.0f, +1.0f, -1.0f),
    vec3(+1.0f, -1.0f, +1.0f),
    vec3(+1.0f, -1.0f, -1.0f),
    vec3(+1.0f, +1.0f, +1.0f),
    vec3(+1.0f, +1.0f, -1.0f),
    vec3(-1.0f, -1.0f, -1.0f),
    vec3(-1.0f, -1.0f, +1.0f),
    vec3(-1.0f, +1.0f, -1.0f),
    vec3(-1.0f, +1.0f, +1.0f),
    vec3(-1.0f, +1.0f, +1.0f),
    vec3(+1.0f, +1.0f, +1.0f),
    vec3(-1.0f, +1.0f, -1.0f),
    vec3(+1.0f, +1.0f, -1.0f),
    vec3(-1.0f, -1.0f, -1.0f),
    vec3(+1.0f, -1.0f, -1.0f),
    vec3(-1.0f, -1.0f, +1.0f),
    vec3(+1.0f, -1.0f, +1.0f) 
};

layout(location = 0) out vec4 out_pos;

void main()
{
   vec3 pos = cube_strip[gl_VertexIndex];
   gl_Position = transform.projection * transform.view * transform.model * vec4(pos, 1.0f);
   out_pos = gl_Position;
}