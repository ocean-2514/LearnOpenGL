#version 330 core
layout (location = 0) in vec2 aPosition;
layout (location = 1) in vec3 aColor;
out vec3 vertexColor;
uniform vec2 uViewport;
void main()
{
    vec2 ndc = vec2(aPosition.x / uViewport.x * 2.0 - 1.0,
                    1.0 - aPosition.y / uViewport.y * 2.0);
    gl_Position = vec4(ndc, 0.0, 1.0);
    vertexColor = aColor;
}