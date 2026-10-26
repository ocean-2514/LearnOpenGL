#version 330 core

layout (points) in;
layout (triangle_strip, max_vertices = 5) out;

in vec2 vTexCoord[];
in vec3 vNormal[];
in vec3 vFragPos[];

out vec2 outTexCoord;
out vec3 outNormal;
out vec3 outFragPos;

float offsets[6] = {-0.2, 0.2, 0.2, -0.2, -0.2, 0.2};

void main()
{
    vec4 position = gl_in[0].gl_Position;

    for (int i = 0; i < 5; i++)
    {
        gl_Position = position + vec4(offsets[i], offsets[i + 1], 0.0, 1.0);
        outFragPos = vFragPos[0] + vec3(offsets[i], offsets[i + 1], 0.0);
        outTexCoord = vTexCoord[0];
        outNormal = vNormal[0];

        EmitVertex();
    }
    EndPrimitive();
}