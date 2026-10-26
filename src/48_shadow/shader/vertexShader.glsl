#version 330 core
layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 TexCoords;

out vec2 outTexCoord;
out vec3 outNormal;
out vec3 outFragPos;
out vec3 outFragViewPos;
out vec4 outFragLightPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;
uniform mat3 normalMatrix;
// uniform vec3 floorNormal;

void main() {
  gl_Position = projection * view * model * vec4(Position, 1.0f);
  gl_PointSize = 10.0f;
  outTexCoord = TexCoords;
  outNormal = normalMatrix * Normal;
  outFragPos = vec3(model * vec4(Position, 1.0f));
  outFragViewPos = vec3(view * vec4(outFragPos, 1.0));
  outFragLightPos = lightSpaceMatrix * vec4(outFragPos, 1.0f);
}