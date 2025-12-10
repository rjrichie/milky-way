#version 330 core

/*default camera matrices. do not modify.*/
layout(std140) uniform camera {
    mat4 projection;
    mat4 view;
    mat4 pvm;
    mat4 ortho;
    vec4 position;
};

layout(location = 0) in vec4 pos; // xyz = position, w = placeholder
layout(location = 1) in vec4 v_color; // rgba

uniform float point_size;

out vec4 vtx_color;

void main()
{
    vec4 worldPos = vec4(pos.xyz, 1.0);
    vtx_color = v_color;
    gl_Position = projection * view * worldPos;
    gl_PointSize = point_size; // set by OpenGLPoints
}
