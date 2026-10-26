#version 330 core
layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 TexCoords;

out vec2 outTexCoord;
out vec3 outNormal;
out vec3 outFragPos;
// out vec3 outFragViewPos;

// out VS_OUT{
//   vec2 vTexCoord;
//   vec3 vNormal;
//   vec3 vFragPos;
// } vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

void main() {
  gl_Position = projection * view * model * vec4(Position, 1.0f);
  gl_PointSize = gl_Position.z * 5.0f;
  outTexCoord = TexCoords;
  outNormal = normalMatrix * Normal;
  outFragPos = vec3(model * vec4(Position, 1.0f));
  // outFragViewPos = vec3(view * vec4(vFragPos, 1.0));
}