#version 330 core
out vec4 FragColor;

in vec2 outTexCoord;

uniform sampler2D ourTexture;


void main()
{
    FragColor = texture2D(ourTexture, outTexCoord);
    // FragColor = vec4(ourColor, 1.0f);
}
