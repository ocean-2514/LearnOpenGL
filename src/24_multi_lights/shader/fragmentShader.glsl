#version 330 core
out vec4 FragColor;

in vec2 outTexCoord;
in vec3 outNormal;
in vec3 outFragPos;

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


uniform vec3 viewPos;
uniform Material material;
uniform SpotLight spotLight;
uniform PointLight pointLights[4];
uniform DirLight dirLight;

void main()
{
    vec3 norm = normalize(outNormal);
    vec3 viewDir = normalize(viewPos - outFragPos);

    vec3 result = getSpotLightColor(norm, viewDir, spotLight);

    for(int i = 0; i < 4; i++)
    {
        result += getPointLightColor(norm, viewDir, pointLights[i]);
    }

    result += getDirLightColor(norm, viewDir, dirLight);

    FragColor = vec4(result, 1.0f);
}

vec3 getSpotLightColor(vec3 norm, vec3 viewDir, SpotLight light)
{
    float distance = length(light.position - outFragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    vec3 ambient = light.ambient * vec3(texture(material.diffuse, outTexCoord));

    vec3 lightDir = normalize(light.position - outFragPos);
    float diff = max(0.0, dot(lightDir, norm));
    vec3 diffuse = diff * light.diffuse * vec3(texture(material.diffuse, outTexCoord));

    vec3 reflection = reflect(-lightDir, norm);
    float spec = pow(max(0.0, dot(reflection, viewDir)), material.shininess);
    vec3 specular = spec * light.specular * vec3(texture(material.specular, outTexCoord));

    float epsilon = light.cutOff - light.outerCutOff;
    float theta = dot(-lightDir, normalize(light.direction));
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    return intensity * attenuation * (ambient + diffuse + specular);
}

vec3 getPointLightColor(vec3 norm, vec3 viewDir, PointLight light)
{
    float distance = length(light.position - outFragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    vec3 ambient = light.ambient * vec3(texture(material.diffuse, outTexCoord));

    vec3 lightDir = normalize(light.position - outFragPos);
    float diff = max(0.0, dot(lightDir, norm));
    vec3 diffuse = diff * light.diffuse * vec3(texture(material.diffuse, outTexCoord));

    vec3 reflection = reflect(-lightDir, norm);
    float spec = pow(max(0.0, dot(reflection, viewDir)), material.shininess);
    vec3 specular = spec * light.specular * vec3(texture(material.specular, outTexCoord));

    return attenuation * (ambient + diffuse + specular);
}

vec3 getDirLightColor(vec3 norm, vec3 viewDir, DirLight light)
{
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, outTexCoord));

    vec3 lightDir = normalize(-light.direction);
    float diff = max(0.0, dot(lightDir, norm));
    vec3 diffuse = diff * light.diffuse * vec3(texture(material.diffuse, outTexCoord));

    vec3 reflection = reflect(-lightDir, norm);
    float spec = pow(max(0.0, dot(reflection, viewDir)), material.shininess);
    vec3 specular = spec * light.specular * vec3(texture(material.specular, outTexCoord));

    return ambient + diffuse + specular;
}
