#version 330 core

out vec4 FragColor;

in vec2 outTexCoord;

uniform sampler2D imageTexture;

void main() {
    FragColor = texture2D(imageTexture, outTexCoord);
}