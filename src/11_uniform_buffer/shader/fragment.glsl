#version 330 core

layout (std140) uniform Screen
{
    int scrWidth;
    int scrHeight;
};

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform samplerCube skybox;

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
    vec3 reflectDir = reflect(-lightNormalizedDir, Normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = (spec * 2.0f) * texture(texture1, TexCoord).rgb;

    vec3 C = (ambient + diffuse + specular) * attenuation;
    vec4 R = texture(skybox, reflect(FragPos, Normal));

    int halfWidth = int(scrWidth * 0.5f);
    int halfHeight = int(scrHeight * 0.5f);
    if (gl_FragCoord.x > halfWidth)
        if (gl_FragCoord.y > halfHeight)
            FragColor = vec4(C * 0.6f + R.rgb * 0.4f, 1.0f);
        else
            FragColor = R;
    else
        if (gl_FragCoord.y > halfHeight)
            FragColor = vec4(C, 1.0f);
        else
            FragColor = texture(skybox, refract(FragPos, Normal, 1.00 / 1.52));
}