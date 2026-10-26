#version 330 core

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 5) in ivec4 BoneIds;
layout(location = 6) in vec4 Weights;

out vec3 outNormal;
flat out ivec4 outBoneIds;
out vec4 outWeights;

uniform mat4 model = mat4(1.0f);
uniform mat4 view;
uniform mat4 projection;

const int maxBones = 150;
uniform bool hasAnim = false;
uniform mat4 gBones[maxBones];

void main() {
    mat4 finalModel;

    if (hasAnim) {
        mat4 skinTransform = mat4(0.0f);
        bool hasSkin = false;
        for (int i = 0; i < 4; ++i) {
            if (BoneIds[i] == -1) break;
            hasSkin = true;
            skinTransform += gBones[BoneIds[i]] * Weights[i];
        }

        if (!hasSkin) {
            skinTransform = mat4(1.0f);
        }

        finalModel = model * skinTransform;
    } else {
        finalModel = model;
    }

    gl_Position = projection * view * finalModel * vec4(Position, 1.0f);

    mat3 normalMatrix = mat3(transpose(inverse(finalModel)));
    outNormal = normalMatrix * Normal;
}