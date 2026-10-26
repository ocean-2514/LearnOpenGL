#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

// in VS_OUT{
//     vec2 vTexCoord;
//     vec3 vNormal;
//     vec3 vFragPos;
// } gs_in[];

in vec2 vTexCoord[];
in vec3 vNormal[];
in vec3 vFragPos[];

out vec2 outTexCoord;
out vec3 outNormal;
out vec3 outFragPos;

uniform float time;

vec3 getNormal() 
{
    vec3 a = vec3(gl_in[1].gl_Position) - vec3(gl_in[0].gl_Position);
    vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[0].gl_Position);
    vec3 normal = normalize(cross(a, b));
    return normal;
}

vec4 explode(vec4 position, vec3 normal)
{
    float magnitude = 0.5;
    return position + vec4(normal * (sin(time) - 1.0) * magnitude, 1.0);
}

void main()
{
    vec3 normal = getNormal();
    for (int i = 0; i < 3; i++)
    {
        gl_Position = vec4(explode(gl_in[i].gl_Position, normal));
        // gl_Position = gl_in[i].gl_Position;
        outTexCoord = vTexCoord[i];
        outFragPos = vFragPos[i];
        outNormal = vNormal[i];
        EmitVertex();
    }
    
    EndPrimitive();
}