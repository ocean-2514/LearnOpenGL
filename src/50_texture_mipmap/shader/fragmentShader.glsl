#version 430 core

in vec2 outTexCoord;

out vec4 FragColor;

uniform sampler2D texture1;

float calculateLod();
vec4 calculateColor();

void main() {
    int lod = int(textureQueryLod(texture1, outTexCoord).x);
    // float lod = calculateLod();
    int maxLevel = textureQueryLevels(texture1);

    float normalizedLod = float(lod) / maxLevel;

    FragColor = vec4(vec3(normalizedLod), 1.0f);

    // FragColor = calculateColor();
    // FragColor = texture2D(texture1, outTexCoord);
}

vec4 calculateColor() {
    float lod = calculateLod();
    float ceil = ceil(lod);
    float floor = floor(lod);
    float mixFactor = fract(lod);

    vec4 texColor1 = texture2DLod(texture1, outTexCoord, floor);
    vec4 texColor2 = texture2DLod(texture1, outTexCoord, ceil);
    return vec4(mix(texColor1, texColor2, mixFactor));
}

float calculateLod() {
    vec2 dx = dFdx(outTexCoord);
    vec2 dy = dFdy(outTexCoord);
    
    // 计算投影后的纹理坐标变化（考虑纹理大小）
    vec2 texSize = vec2(textureSize(texture1, 0));
    float maxDeriv = max(length(dx * texSize), length(dy * texSize));
    
    // 估算Mipmap层级
    float lod = log2(maxDeriv);
    lod = clamp(lod, 0.0, textureQueryLevels(texture1) - 1.0);
    return lod;
}