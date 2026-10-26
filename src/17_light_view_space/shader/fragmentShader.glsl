#version 330 core
out vec4 FragColor;

in vec2 outTexCoords;
in vec3 outNormal;
in vec3 outFragPos;

uniform sampler2D ourTexture;
uniform vec3 lightColor;
uniform vec3 lightPos;
// uniform vec3 objectColor;

void main()
{
    vec3 objectColor = texture2D(ourTexture, outTexCoords).xyz;

    float ambientStrength = 0.3f;
    vec3 ambient = ambientStrength * lightColor;

    vec3 lightDir = normalize(lightPos - outFragPos);
    vec3 norm = normalize(outNormal);
    float diff = max(0.0, dot(norm, lightDir));
    vec3 diffuse = diff * lightColor;

    float specularStrength = 0.9f;
    vec3 reflection = reflect(-lightDir, norm);
    float spec = pow(max(0.0, dot(reflection, normalize(-outFragPos))), 32);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result = objectColor * (ambient + diffuse + specular);
    FragColor = vec4(result, 1.0f);
}
