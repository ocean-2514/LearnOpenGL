#version 330 core
out vec4 FragColor;

in vec2 outTexCoord;
in vec3 outNormal;
in vec3 outFragPos;
in vec3 outFragViewPos;
in vec4 outFragLightPos;

float near = 0.1;
float far = 100.0;

struct Material{
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 color;

    float constant;
    float linear;
    float quadratic;
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


vec3 getSpotLightColor(vec3 norm, vec3 viewDir, SpotLight light);
vec3 getPointLightColor(vec3 norm, vec3 viewDir, PointLight light);
vec3 getDirLightColor(vec3 norm, vec3 viewDir, DirLight light);


uniform vec3 viewPos;
uniform Material material;
uniform PointLight pointLights[4];
uniform SpotLight spotLight;
uniform DirLight dirLight;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D shadowMap;

float ShadowCalculation(vec4 outFragLightPos) 
{
    // 执行透视除法
    vec3 projCoords = outFragLightPos.xyz / outFragLightPos.w;
    // 变换到[0,1]的范围
    projCoords = projCoords * 0.5 + 0.5;
    // 取得最近点的深度(使用[0,1]范围下的fragPosLight当坐标)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // 取得当前片段在光源视角下的深度
    float currentDepth = projCoords.z;
    //偏移，解决阴影失真
    vec3 norm = normalize(outNormal);
    vec3 lightDir = normalize(dirLight.direction);
    float bias = max(0.005, 0.05 * (1.0 - dot(norm, lightDir)));
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;

    return shadow;
}

void main()
{
    vec3 norm = normalize(outNormal);
    vec3 viewDir = normalize(viewPos - outFragPos);
    
    // vec3 result = getPointLightColor(norm, viewDir, pointLights[0]);
    vec3 result = getDirLightColor(norm, viewDir, dirLight);

    FragColor = vec4(result, 1.0);

    
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

    vec3 ambient = light.ambient * vec3(texture(texture_diffuse1, outTexCoord));

    vec3 lightDir = normalize(light.position - outFragPos);
    float diff = max(0.0, dot(lightDir, norm));
    vec3 diffuse = diff * light.color * vec3(texture(texture_diffuse1, outTexCoord));

    vec3 halfwayDir = normalize(viewDir + lightDir);
    float spec = pow(max(0.0, dot(halfwayDir, norm)), 32.0);
    vec3 specular = spec * light.color * vec3(0.3);
    // vec3 reflection = reflect(-lightDir, norm);
    // float spec = pow(max(0.0, dot(reflection, viewDir)), 32);
    // vec3 specular = spec * light.specular * vec3(texture(texture_specular1, outTexCoord));

    return attenuation * (ambient + diffuse + specular);
}

vec3 getDirLightColor(vec3 norm, vec3 viewDir, DirLight light)
{
    vec3 ambient = light.ambient;

    vec3 lightDir = normalize(-light.direction);
    float diff = max(0.0, dot(lightDir, norm));
    vec3 diffuse = diff * light.diffuse;

    vec3 reflection = reflect(-lightDir, norm);
    float spec = pow(max(0.0, dot(reflection, viewDir)), material.shininess);
    vec3 specular = spec * light.specular;

    float shadow = ShadowCalculation(outFragLightPos);
    vec3 result = (ambient + (1 - shadow) * (diffuse + specular)) * 
        vec3(texture(texture_diffuse1, outTexCoord));

    return result;
}
