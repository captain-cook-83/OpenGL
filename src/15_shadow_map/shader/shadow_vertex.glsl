#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 3) in mat4 instanceMatrix;

layout (std140) uniform Matrices
{
    mat4 view;
    mat4 projection;
};

void main()
{
    gl_Position = projection * view * instanceMatrix * vec4(aPos, 1.0);
}  