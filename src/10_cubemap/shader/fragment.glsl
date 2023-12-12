#version 330 core

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

    // 正常渲染
    // FragColor = vec4(C, 1.0f);

    // // 镜面反射
    vec4 R = texture(skybox, reflect(FragPos, Normal));
    FragColor = vec4(C * 0.8f + R.rgb * 0.2f, 1.0f);
    // FragColor = R;

    // 玻璃折射
    // float ratio = 1.00 / 1.52;          // https://learnopengl-cn.github.io/04%20Advanced%20OpenGL/06%20Cubemaps/
    // vec4 R = texture(skybox, refract(FragPos, Normal, ratio));
    // FragColor = vec4(C * 0.2f + R.rgb * 0.8f, 1.0f);
    // FragColor = R;

    // 深度图
    // float z = gl_FragCoord.z * 2.0 - 1.0;
    // float linearDepth = (2.0 * near * far) / (far + near - z * (far - near));
    // FragColor = vec4(vec3(linearDepth / far), 1.0);
}