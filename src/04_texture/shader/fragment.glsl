#version 330 core

out vec4 FragColor;

in vec3 vertexColor;
in vec2 TexCoord;

uniform float timeIntensity;
uniform sampler2D texture0;
uniform sampler2D texture1;

void main()
{
    FragColor = mix(texture(texture0, TexCoord), texture(texture1, TexCoord), timeIntensity) * vec4(vertexColor, 1.0f);
    // FragColor = texture(texture0, TexCoord) * vec4(vertexColor, 1.0f) * timeIntensity;
}