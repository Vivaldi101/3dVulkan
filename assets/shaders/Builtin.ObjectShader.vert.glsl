#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform Transform
{
    mat4 projection;
    mat4 scale;
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

const vec3 cube_verts_strip[] = 
{
    // Front face (triangle strip)
    vec3(-1.0f, -1.0f,  1.0f), // v0
    vec3( 1.0f, -1.0f,  1.0f), // v1
    vec3(-1.0f,  1.0f,  1.0f), // v2
    vec3( 1.0f,  1.0f,  1.0f), // v3

    // Back face (triangle strip)
    vec3( 1.0f, -1.0f, -1.0f), // v4
    vec3(-1.0f, -1.0f, -1.0f), // v5
    vec3( 1.0f,  1.0f, -1.0f), // v6
    vec3(-1.0f,  1.0f, -1.0f), // v7

    // Left face (triangle strip)
    vec3(-1.0f, -1.0f, -1.0f), // v8
    vec3(-1.0f, -1.0f,  1.0f), // v9
    vec3(-1.0f,  1.0f, -1.0f), // v10
    vec3(-1.0f,  1.0f,  1.0f), // v11

    // Right face (triangle strip)
    vec3( 1.0f, -1.0f,  1.0f), // v12
    vec3( 1.0f, -1.0f, -1.0f), // v13
    vec3( 1.0f,  1.0f,  1.0f), // v14
    vec3( 1.0f,  1.0f, -1.0f), // v15

    // Top face (triangle strip)
    vec3(-1.0f,  1.0f,  1.0f), // v16
    vec3( 1.0f,  1.0f,  1.0f), // v17
    vec3(-1.0f,  1.0f, -1.0f), // v18
    vec3( 1.0f,  1.0f, -1.0f), // v19

    // Bottom face (triangle strip)
    vec3(-1.0f, -1.0f,  1.0f), // v20
    vec3( 1.0f, -1.0f,  1.0f), // v21
    vec3(-1.0f, -1.0f, -1.0f), // v22
    vec3( 1.0f, -1.0f, -1.0f), // v23
};

const vec3 cube_verts[] = 
{
    // Front face (2 triangles)
    vec3(-1.0f, -1.0f,  1.0f), // v0
    vec3( 1.0f, -1.0f,  1.0f), // v1
    vec3( 1.0f,  1.0f,  1.0f), // v2
    vec3(-1.0f, -1.0f,  1.0f), // v3
    vec3( 1.0f,  1.0f,  1.0f), // v4
    vec3(-1.0f,  1.0f,  1.0f), // v5

    // Back face (2 triangles)
    vec3(-1.0f, -1.0f, -1.0f), // v6
    vec3( 1.0f, -1.0f, -1.0f), // v7
    vec3( 1.0f,  1.0f, -1.0f), // v8
    vec3(-1.0f, -1.0f, -1.0f), // v9
    vec3( 1.0f,  1.0f, -1.0f), // v10
    vec3(-1.0f,  1.0f, -1.0f), // v11

    // Left face (2 triangles)
    vec3(-1.0f, -1.0f, -1.0f), // v12
    vec3(-1.0f, -1.0f,  1.0f), // v13
    vec3(-1.0f,  1.0f,  1.0f), // v14
    vec3(-1.0f, -1.0f, -1.0f), // v15
    vec3(-1.0f,  1.0f,  1.0f), // v16
    vec3(-1.0f,  1.0f, -1.0f), // v17

    // Right face (2 triangles)
    vec3( 1.0f, -1.0f,  1.0f), // v18
    vec3( 1.0f, -1.0f, -1.0f), // v19
    vec3( 1.0f,  1.0f,  1.0f), // v20
    vec3( 1.0f, -1.0f, -1.0f), // v21
    vec3( 1.0f,  1.0f,  1.0f), // v22
    vec3( 1.0f,  1.0f, -1.0f), // v23

    // Top face (2 triangles)
    vec3(-1.0f,  1.0f,  1.0f), // v24
    vec3( 1.0f,  1.0f,  1.0f), // v25
    vec3(-1.0f,  1.0f, -1.0f), // v26
    vec3(-1.0f,  1.0f,  1.0f), // v27
    vec3( 1.0f,  1.0f, -1.0f), // v28
    vec3( 1.0f,  1.0f,  1.0f), // v29

    // Bottom face (2 triangles)
    vec3(-1.0f, -1.0f,  1.0f), // v30
    vec3( 1.0f, -1.0f,  1.0f), // v31
    vec3(-1.0f, -1.0f, -1.0f), // v32
    vec3(-1.0f, -1.0f, -1.0f), // v33
    vec3( 1.0f, -1.0f,  1.0f), // v34
    vec3( 1.0f, -1.0f, -1.0f), // v35
};

vec3 cube_strip[] = 
{
    vec3(-1.0f, -1.0f,  1.0f), //Vertex 0
    vec3( 1.0f, -1.0f,  1.0f), //v1
    vec3(-1.0f,  1.0f,  1.0f), //v2
    vec3( 1.0f,  1.0f,  1.0f), //v3

    vec3( 1.0f, -1.0f,  1.0f), //...
    vec3( 1.0f, -1.0f, -1.0f), 
    vec3( 1.0f,  1.0f,  1.0f),
    vec3( 1.0f,  1.0f, -1.0f),

    vec3( 1.0f, -1.0f, -1.0f),
    vec3(-1.0f, -1.0f, -1.0f), 
    vec3( 1.0f,  1.0f, -1.0f),
    vec3(-1.0f,  1.0f, -1.0f),

    vec3(-1.0f, -1.0f, -1.0f),
    vec3(-1.0f, -1.0f,  1.0f), 
    vec3(-1.0f,  1.0f, -1.0f),
    vec3(-1.0f,  1.0f,  1.0f),

    vec3(-1.0f, -1.0f, -1.0f),
    vec3( 1.0f, -1.0f, -1.0f), 
    vec3(-1.0f, -1.0f,  1.0f),
    vec3( 1.0f, -1.0f,  1.0f),

    vec3(-1.0f,  1.0f,  1.0f),
    vec3( 1.0f,  1.0f,  1.0f), 
    vec3(-1.0f,  1.0f, -1.0f),
    vec3( 1.0f,  1.0f, -1.0f),
};

layout(location = 0) out vec4 out_pos;

void main()
{
   vec3 pos = cube_strip[gl_VertexIndex];
   gl_Position = transform.projection * transform.scale * vec4(pos, 1.0f);
   out_pos = gl_Position;
}