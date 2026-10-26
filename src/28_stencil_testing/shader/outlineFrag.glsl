#version 330 core
in vec2 outTexCoord;
out vec4 FragColor;

uniform vec3 lightColor;

void main()
{
    FragColor = vec4(0.04, 0.89, 0.89, 1.0);
}