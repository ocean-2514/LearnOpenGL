#version 330 core

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

in vec2 outTexCoord[];
in vec3 outNormal[];
in vec3 outFragPos[];

uniform float time;
uniform mat4 view;
uniform mat4 projection;

void generateLine(int i)
{
    float magnitude = 0.3;
    gl_Position = gl_in[i].gl_Position;
    EmitVertex();
    gl_Position = projection * view * vec4(outFragPos[i] + outNormal[i] * magnitude, 1.0);
    EmitVertex();
    EndPrimitive();
    
}

void main()
{
   generateLine(0);
   generateLine(1);
   generateLine(2);
    
}