#version 330 core
layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 TexCoords;

out vec2 vTexCoord;
out vec3 vNormal;
out vec3 vFragPos;
// out vec3 vFragViewPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

void main() {
  gl_Position = projection * view * model * vec4(Position, 1.0f);
  gl_PointSize = gl_Position.z * 5.0f;
  vTexCoord = TexCoords;
  vNormal = normalMatrix * Normal;
  vFragPos = vec3(model * vec4(Position, 1.0f));
  // vFragViewPos = vec3(view * vec4(vFragPos, 1.0));
}