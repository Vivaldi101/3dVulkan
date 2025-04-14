#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform Transform
{
    mat4 projection;
    mat4 view;
    mat4 model;
   float near;
   float far;
   float ar;
} transform;

float ndc_to_linear_z(float ndc_z, float near, float far)
{
   float u = far * near;
   float l = (near - far)*ndc_z + far;

   float linear_z = u / l;

   return linear_z;
}

void main()
{
    float linear_z = ndc_to_linear_z(gl_FragCoord.z, transform.near, transform.far);

    //out_color = vec4(1.0);
    out_color = vec4(vec3(linear_z*0.125*0.5, 0.0, 0.0), 1.0);
}
