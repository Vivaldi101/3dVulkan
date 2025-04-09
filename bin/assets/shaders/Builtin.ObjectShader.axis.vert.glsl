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

layout(location = 0) out vec3 axis_color;

mat4 translate(vec3 t)
{
    return mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        t.x, t.y, t.z, 1.0
    );
}

void main() {
    float s = 1.0;
    vec3 positions[6] = vec3[](
        vec3(0.0, 0.0, 0.0), vec3(s, 0.0, 0.0), // X axis
        vec3(0.0, 0.0, 0.0), vec3(0.0, s, 0.0), // Y axis
        vec3(0.0, 0.0, 0.0), vec3(0.0, 0.0, s)  // Z axis
    );

    vec3 colors[6] = vec3[](
        vec3(1, 0, 0), vec3(1, 0, 0), // X = red
        vec3(0, 1, 0), vec3(0, 1, 0), // Y = green
        vec3(0, 0, 1), vec3(0, 0, 1)  // Z = blue
    );

    mat4 t = translate(vec3(0.0, 0.0, -1.0));
    gl_Position = transform.projection * transform.view * t * vec4(positions[gl_VertexIndex], 1.0);

    axis_color = colors[gl_VertexIndex];
}
