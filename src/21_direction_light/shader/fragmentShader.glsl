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

struct Light {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

// uniform vec3 lightPos;
// uniform vec3 objectColor;
uniform vec3 viewPos;
uniform Material material;
uniform Light light;

void main()
{
    vec3 ambient = light.ambient * vec3(texture2D(material.diffuse, outTexCoord));

    vec3 norm = normalize(outNormal);
    vec3 lightDir = -light.direction;
    float diff = max(0.0, dot(norm, lightDir));
    vec3 diffuse = diff * light.diffuse * vec3(texture2D(material.diffuse, outTexCoord)) ;

    vec3 viewDir = normalize(viewPos - outFragPos);
    vec3 reflection = reflect(-lightDir, norm);
    float spec = pow(max(0.0, dot(viewDir, reflection)), material.shininess);
    vec3 specular = spec * light.specular * vec3(texture2D(material.specular, outTexCoord));

    vec3 result = ambient + diffuse + specular;

    FragColor = vec4(result, 1.0f);
}
