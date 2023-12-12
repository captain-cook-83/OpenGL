#version 330 core

out vec4 FragColor;
in vec3 vertexColor;

uniform float timeIntensity;

void main()
{
    FragColor = vec4(vertexColor * timeIntensity, 1.0f);
}