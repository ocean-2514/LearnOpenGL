#version 330 core
out vec4 FragColor;

in vec2 outTexCoord;

uniform sampler2D screenTexture;
uniform vec2 screenSize;

const float offset = 1.0 / 300.0; 

void main() 
{
    // FragColor = texture(screenTexture, outTexCoord);
    // FragColor = vec4(vec3(1.0 - texture(screenTexture, outTexCoord)), 1.0); //反相

    // FragColor = texture(screenTexture, outTexCoord);
    // float average = 0.2126 * FragColor.r + 0.7152 * FragColor.g + 0.0722 * FragColor.b;
    // FragColor = vec4(average, average, average, 1.0);

    //核效果
    vec2 offsets[9] = vec2[](
        vec2(-offset,  offset), // 左上
        vec2( 0.0f,    offset), // 正上
        vec2( offset,  offset), // 右上
        vec2(-offset,  0.0f),   // 左
        vec2( 0.0f,    0.0f),   // 中
        vec2( offset,  0.0f),   // 右
        vec2(-offset, -offset), // 左下
        vec2( 0.0f,   -offset), // 正下
        vec2( offset, -offset)  // 右下
    );

    // float kernel[9] = float[](
    //     -1, -1, -1,
    //     -1,  9, -1,
    //     -1, -1, -1
    // );
    
    // float kernel[9] = float[](
    //     1.0 / 16, 2.0 / 16, 1.0 / 16,
    //     2.0 / 16, 4.0 / 16, 2.0 / 16,
    //     1.0 / 16, 2.0 / 16, 1.0 / 16 
    // );  //模糊
    
    // float kernel[9] = float[](
    //     1, 1, 1,
    //     1, -8, 1,
    //     1, 1, 1
    // );  //边缘检测

    vec3 sampleTex[9];
    for (int i = 0; i < 9; i++)
    {
        sampleTex[i] = vec3(texture2D(screenTexture, outTexCoord + offsets[i]));
    }
    vec3 col = vec3(0.0);
    for (int i = 0; i < 9; i++)
    {
        col += sampleTex[i] * kernel[i];
    }
    FragColor = vec4(col, 1.0);

    //  // 像素化参数
    // float pixelSize = 10.0; // 像素块大小
    
    // // 计算像素化后的UV坐标
    // vec2 pixelatedUV = floor(outTexCoord * screenSize / pixelSize) * pixelSize / screenSize;
    
    // // 采样纹理
    // FragColor = texture(screenTexture, pixelatedUV);

}