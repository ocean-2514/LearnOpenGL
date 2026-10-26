#version 330 core

in vec2 outTexCoord;

out vec4 FragColor;

uniform sampler2D texture1;

void main() {
    FragColor = texture2D(texture1, outTexCoord);
}