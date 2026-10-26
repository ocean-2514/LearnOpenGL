#version 330 core
out vec4 FragColor;

in vec3 outNormal;
flat in ivec4 outBoneIds;
in vec4 outWeights;

uniform vec3 uDiffuse = vec3(1.0f, 0.0f, 0.0f);
uniform int displayBoneIndex;

void main()
{
    bool found = false;
    for (int i = 0; i < 4; ++i) {
        if (outBoneIds[i] < 0) break;
        if (outBoneIds[i] == displayBoneIndex) {
            found = true;
            if (outWeights[i] >= 0.7) {
                FragColor = vec4(1.0, 0.0, 0.0, 1.0) * outWeights[i];
            } else if (outWeights[i] >= 0.4 && outWeights[i] < 0.7) {
                FragColor = vec4(0.0, 1.0, 0.0, 1.0) * outWeights[i];
            } else {
                FragColor = vec4(1.0, 1.0, 0.0, 1.0) * outWeights[i];
            }
            break;
        }
    }

    if (found) return;

    vec3 norm = normalize(outNormal);
    float diff = max(0.4, dot(norm, vec3(0.0, 1.0, 0.0)));
    FragColor = vec4(diff * uDiffuse, 1.0f);
}
