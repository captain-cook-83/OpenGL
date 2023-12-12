#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

layout (std140) uniform Matrices
{
    mat4 view;
    mat4 projection;
};

void main()
{
    TexCoords = aPos;
    gl_Position = (projection * mat4(mat3(view)) * vec4(aPos, 1.0)).xyww;           // xyww 将深度设置为最大 z = w
}