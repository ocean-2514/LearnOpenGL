#version 330 core
out vec4 FragColor;

in vec2 outTexCoord;
in vec3 outNormal;
in vec3 outFragPos;
in vec3 outFragViewPos;

float near = 0.1;
float far = 100.0;

struct Material{
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct SpotLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    sampler2D spotLightMap;
};

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

vec3 getSpotLightColor(vec3 norm, vec3 viewDir, SpotLight light);
vec3 getPointLightColor(vec3 norm, vec3 viewDir, PointLight light);
vec3 getDirLightColor(vec3 norm, vec3 viewDir, DirLight light);
float LinearizeDepth(float depth);

uniform vec3 viewPos;
uniform Material material;
uniform SpotLight spotLight;
uniform PointLight pointLights[4];
uniform DirLight dirLight;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

void main()
{
    vec3 norm = normalize(outNormal);
    vec3 viewDir = normalize(viewPos - outFragPos);
    
    // vec3 result = getSpotLightColor(norm, viewDir, spotLight);
    // result += getDirLightColor(norm, viewDir, dirLight);
    // result += getPointLightColor(norm, viewDir, pointLights[0]);

    FragColor = vec4(vec3(LinearizeDepth(gl_FragCoord.z) / far), 1.0);
}

vec3 getSpotLightColor(vec3 norm, vec3 viewDir, SpotLight light)
{
    float distance = length(light.position - outFragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    vec3 ambient = light.ambient * vec3(texture(texture_diffuse1, outTexCoord));

    vec3 lightDir = normalize(light.position - outFragPos);
    float diff = max(0.0, dot(lightDir, norm));
    vec3 diffuse = diff * light.diffuse * vec3(texture(texture_diffuse1, outTexCoord));

    vec3 reflection = reflect(-lightDir, norm);
    float spec = pow(max(0.0, dot(reflection, viewDir)), 32);
    vec3 specular = spec * light.specular * vec3(texture(texture_specular1, outTexCoord));

    float epsilon = light.cutOff - light.outerCutOff;
    float theta = dot(-lightDir, normalize(light.direction));
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    // vec2 patternTexCoord = normalize(outFragViewPos).xy * 3.5;
    // patternTexCoord.x = patternTexCoord.x * 0.6 + 0.5;
    // patternTexCoord.y = patternTexCoord.y * 0.5 + 0.5;
    // vec3 spotDiffuse = texture(light.spotLightMap, patternTexCoord).rgb * diff;

    return intensity * attenuation * (ambient + diffuse + specular);
}

vec3 getPointLightColor(vec3 norm, vec3 viewDir, PointLight light)
{
    float distance = length(light.position - outFragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    vec3 ambient = light.ambient * vec3(texture(texture_diffuse1, outTexCoord));

    vec3 lightDir = normalize(light.position - outFragPos);
    float diff = max(0.0, dot(lightDir, norm));
    vec3 diffuse = diff * light.diffuse * vec3(texture(texture_diffuse1, outTexCoord));

    vec3 reflection = reflect(-lightDir, norm);
    float spec = pow(max(0.0, dot(reflection, viewDir)), 32);
    vec3 specular = spec * light.specular * vec3(texture(texture_specular1, outTexCoord));

    return attenuation * (ambient + diffuse + specular);
}

vec3 getDirLightColor(vec3 norm, vec3 viewDir, DirLight light)
{
    vec3 ambient = light.ambient * vec3(texture(texture_diffuse1, outTexCoord));

    vec3 lightDir = normalize(-light.direction);
    float diff = max(0.0, dot(lightDir, norm));
    vec3 diffuse = diff * light.diffuse * vec3(texture(texture_diffuse1, outTexCoord));

    vec3 reflection = reflect(-lightDir, norm);
    float spec = pow(max(0.0, dot(reflection, viewDir)), 32);
    vec3 specular = spec * light.specular * vec3(texture(texture_specular1, outTexCoord));

    return ambient + diffuse + specular;
}

float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // 转换为 NDC
    return (2.0 * near * far) / (far + near - z * (far - near));    
}