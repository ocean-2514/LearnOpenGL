#version 330 core
layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 TexCoords;
layout (location = 3) in vec2 Offset;

out vec2 outTexCoord;
out vec3 outNormal;
out vec3 outFragPos;
// out vec3 outFragViewPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

void main() {
  vec3 newPosition = Position * 1.5 / (1 + gl_InstanceID) + vec3(Offset, 0.0);
  gl_Position = projection * view * model * vec4(newPosition, 1.0f);
  gl_PointSize = gl_Position.z * 5.0f;
  outTexCoord = TexCoords;
  outNormal = normalMatrix * Normal;
  outFragPos = vec3(model * vec4(newPosition, 1.0f));
  // outFragViewPos = vec3(view * vec4(vFragPos, 1.0));
}