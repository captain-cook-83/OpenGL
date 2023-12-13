#version 330 core

layout (std140) uniform LightCamera
{
    bool perspective;
    float near;
    float far;
};

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightSpace;

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform sampler2D shadowMap;

uniform int shininess;
struct Light {
    vec3 dir; 
    vec3 color;
    vec3 ambient;
};
uniform Light light;

float PCFShadowCalculation(vec4 fragPosLightSpace, float bias)
{
    // perform perspective divide
    float distance = fragPosLightSpace.z;
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    if (projCoords.z > 1.0) return 0.0;    // 超出视锥剪裁范围时，深度图采样的 GL_CLAMP_TO_BORDER 不起作用，所以需要在这里检查

    projCoords = projCoords * 0.5 + 0.5;        // [-1, 1] -> [0, 1]

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            float depth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;         // [0, 1]
            if (perspective)
            {
                // 转换为线性深度值
                depth = depth * 2.0 - 1.0;  // [0, 1] -> [-1, 1]
                depth = (2.0 * near * far) / (far + near - depth * (far - near));
                shadow += distance > depth - near ? 1.0 : 0.0;
            } else {
                shadow += projCoords.z - bias > depth ? 1.0 : 0.0;
            }
        }
    }
    return shadow /= 9.0;
}

float ShadowCalculation(vec4 fragPosLightSpace, float bias)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    if (projCoords.z > 1.0) return 0.0;    // 超出视锥剪裁范围时，深度图采样的 GL_CLAMP_TO_BORDER 不起作用，所以需要在这里检查

    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float depth = texture(shadowMap, projCoords.xy).r;
    if (perspective)
    {
        depth = depth * 2.0 - 1.0;
        depth = (2.0 * near * far) / (far + near - depth * (far - near));
        depth = depth / far;
    }

    float shadow = currentDepth - bias > depth  ? 1.0 : 0.0;
    return shadow;
}

void main()
{
    vec3 lightDir = normalize(light.dir);
    vec3 texColor = texture(texture0, TexCoord).rgb;
    
    vec3 ambient = light.ambient;

    float diff = max(dot(Normal, lightDir), 0.0f);
    vec3 diffuse = diff * light.color;

    vec3 viewDir = normalize(-FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), shininess);
    vec3 specular = (spec * 3.0f) * texture(texture1, TexCoord).rgb * light.color;

    float bias = max(0.05 * (1.0 - dot(Normal, lightDir)), 0.005);
    float shadow = PCFShadowCalculation(FragPosLightSpace, bias);
    FragColor = vec4((ambient + (1.0 - shadow) * (diffuse + specular)) * texColor, 1.0f);
}