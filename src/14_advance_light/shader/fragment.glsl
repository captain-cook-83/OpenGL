#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D texture0;
uniform sampler2D texture1;

uniform int shininess;
struct Light {
    vec3 position; 
    vec3 color;

    vec3 ambient;

    float constant;
    float linear;
    float quadratic;
};
uniform Light light;

float gamma = 2.2;

void main()
{
    vec3 texColor = texture(texture0, TexCoord).rgb;
    vec3 lightDir = light.position - FragPos;
    float lightDistance = length(lightDir);
    float attenuation = 1.0 / (light.constant + light.linear * lightDistance + light.quadratic * (lightDistance * lightDistance));

    vec3 lightNormalizedDir = normalize(lightDir);
    vec3 ambient = light.ambient * texColor;

    float diff = max(dot(Normal, lightNormalizedDir), 0.0f);
    vec3 diffuse = light.color * texColor * diff;

    vec3 viewDir = normalize(-FragPos);

    // Phong
    // vec3 reflectDir = reflect(-lightNormalizedDir, Normal);
    // float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    
    // Blinn-Phong
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), shininess);

    vec3 specular = spec * texture(texture1, TexCoord).rgb;

    FragColor = vec4((ambient + diffuse + specular) * attenuation, 1.0f);
}