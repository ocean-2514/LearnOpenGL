#version 330 core
layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 TexCoords;

out vec3 outFragPos;
out vec3 outNormal;
out vec2 outTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

void main()
{
  gl_Position = projection * view * model * vec4(Position, 1.0f);
  gl_PointSize = 10.0f;
  outFragPos = (view * model * vec4(Position, 1.0f)).xyz;
  outNormal = normalMatrix * Normal;
  outTexCoords = TexCoords;
}
