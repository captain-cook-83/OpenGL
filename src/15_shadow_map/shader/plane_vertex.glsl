#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

layout (std140) uniform Matrices
{
    mat4 view;
    mat4 projection;
    mat4 lightSpaceMatrix;
};

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec4 FragPosLightSpace;

uniform mat4 model;

void main()
{
    mat4 mv = view * model;                                     // 观察空间内计算光照
    vec4 fragPos = mv * vec4(aPos, 1.0f);
    Normal = normalize((transpose(inverse(mv)) * vec4(aNormal, 0.0f)).xyz);       // 适用于任何情况，包括不等比缩放的 model 矩阵。矩阵求逆是一项对于着色器开销很大的运算，因为它必须在场景中的每一个顶点上进行，所以应该尽可能地避免在着色器中进行求逆运算。以学习为目的的话这样做还好，但是对于一个高效的应用来说，你最好先在CPU上计算出法线矩阵，再通过uniform把它传递给着色器（就像模型矩阵一样）。
    TexCoord = aTexCoord;
    FragPos = fragPos.xyz;
    FragPosLightSpace = lightSpaceMatrix * model * vec4(aPos, 1.0f);
    gl_Position = projection * fragPos;
}