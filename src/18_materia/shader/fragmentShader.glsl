#version 330 core
out vec4 FragColor;

in vec2 outTexCoord;
in vec3 outNormal;
in vec3 outFragPos;

struct Material{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};


uniform sampler2D ourTexture;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 objectColor;
uniform vec3 viewPos;
uniform Material material;
uniform Light light;

void main()
{
    // vec3 objectColor = texture2D(ourTexture, outTexCoord).xyz;

    vec3 ambient = material.ambient * light.ambient;

    vec3 norm = normalize(outNormal);
    vec3 lightDir = normalize(lightPos - outFragPos);
    float diff = max(0.0, dot(norm, lightDir));
    vec3 diffuse = diff * material.diffuse * light.diffuse;

    vec3 viewDir = normalize(viewPos - outFragPos);
    vec3 reflection = reflect(-lightDir, norm);
    float spec = pow(max(0.0, dot(viewDir, reflection)), material.shininess);
    vec3 specular = material.specular * spec * light.specular;


    vec3 result = objectColor * (ambient + diffuse + specular);

    FragColor = vec4(result, 1.0f);
}
