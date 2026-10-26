#version 330 core
out vec4 FragColor;

in vec3 outTexCoord;

uniform samplerCube cubeMap;

void main()
{
    FragColor = texture(cubeMap, outTexCoord);
}

