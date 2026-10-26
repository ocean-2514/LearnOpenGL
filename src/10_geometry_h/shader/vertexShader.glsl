#version 330 core
layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 TexCoords;

out vec2 outTexCoord;

uniform mat4 trans;

void main() {
  // gl_Position = rotateXYZ(factor) * vec4(Position, 1.0f);
  gl_Position = trans * vec4(Position, 1.0f);
  gl_PointSize = 10.0f;
  outTexCoord = TexCoords;
}