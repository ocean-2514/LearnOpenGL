# 光线追踪学习与实现路线

> 本章的目标不是一次写出完整渲染器，而是从“一条射线”开始，逐步得到一个支持球体、材质、阴影、反射、折射、抗锯齿和景深的路径追踪器，最后再迁移到 OpenGL/GLSL 实时运行。

## 1. 光线追踪在做什么

光栅化从三角形出发，寻找它覆盖了哪些像素；光线追踪从像素出发，寻找这个像素能看见什么。

对每个像素，从相机穿过成像平面发出一条射线：

$$
P(t)=O+tD,\quad t>0
$$

- $O$：射线起点（origin）
- $D$：射线方向（direction），通常归一化
- $t$：沿射线前进的距离参数
- $P(t)$：射线上的点

射线与场景中所有物体求交，选择最近的有效交点，再根据材质和光照计算颜色。如果材质会反射或折射，就从交点继续产生新射线。

经典递归光线追踪通常显式计算直接光照、阴影、镜面反射和折射；路径追踪则把渲染方程写成随机采样过程，可以统一模拟直接光照和间接光照。建议先完成前者的基本几何结构，再自然过渡到路径追踪。

## 2. 需要掌握的知识地图

### 2.1 数学基础

- 向量加减、点积、叉积、长度与归一化
- 射线参数方程
- 二次方程（射线与球体求交）
- 坐标系与相机基向量
- 反射、折射和全反射
- 概率、随机采样、蒙特卡洛积分
- Gamma 校正、HDR 和色调映射

最常用的三个公式是：

反射方向：

$$
R=V-2(V\cdot N)N
$$

其中 $V$ 是入射方向，$N$ 是单位法线。

Snell 折射定律：

$$
\eta_i\sin\theta_i=\eta_t\sin\theta_t
$$

渲染方程：

$$
L_o(x,\omega_o)=L_e(x,\omega_o)+
\int_\Omega f_r(x,\omega_i,\omega_o)
L_i(x,\omega_i)(n\cdot\omega_i)\,d\omega_i
$$


它表示：离开表面的光等于表面自己发出的光，加上来自半球各方向的入射光经过材质反射后的总和。路径追踪使用有限个随机样本估计这个积分。

### 2.2 几何与求交

- 球、平面、三角形、轴对齐包围盒（AABB）
- 世界空间、观察空间、局部空间
- 最近交点、法线朝向、纹理坐标
- 浮点误差和自相交（shadow acne）

### 2.3 光照与材质

- Lambert 漫反射
- 金属镜面反射与粗糙度
- 电介质折射、Fresnel 效应、全反射
- BRDF/BSDF、能量守恒、PDF
- 点光源、方向光、面光源、环境光

### 2.4 图像质量与性能

- 抗锯齿、软阴影、景深、运动模糊
- 每像素采样数（SPP）与最大反弹次数
- 噪声、方差、重要性采样、俄罗斯轮盘赌
- BVH、空间划分、并行与 GPU 计算

## 3. 推荐的工程结构

先写 CPU 版本最容易调试：结果保存为 PPM 图片，确认算法正确后再上传为 OpenGL 纹理显示。类可以逐步拆成：

```text
ray_tracing/
├── ray.h          // Ray
├── hittable.h     // HitRecord、Hittable 接口
├── sphere.h       // Sphere
├── material.h     // Lambertian、Metal、Dielectric
├── camera.h       // 生成主射线
├── scene.h        // 场景构建
├── renderer.h     // 采样循环、rayColor
└── image.h        // 像素缓存、保存图片/创建纹理
```

当前工程已有 `include/ray_tracing/Image.h`，可以先让它负责保存 `width * height` 个线性 RGB 浮点颜色，之后增加 `savePPM()` 和 OpenGL 纹理上传功能。

## 4. 分阶段实现

每完成一步都先保存一张图。若图像不符合“预期结果”，不要急着进入下一步。

### 第 0 步：建立最小输出通路

先生成一张二维渐变图，验证像素坐标、颜色范围和图片上下方向。

```cpp
for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
        float r = static_cast<float>(x) / (width - 1);
        float g = static_cast<float>(y) / (height - 1);
        image.setPixel(x, y, glm::vec3(r, g, 0.2f));
    }
}
image.savePPM("output/gradient.ppm");
```

PPM 的 ASCII 格式非常简单：

```text
P3
width height
255
r g b
...
```

预期结果：从一个角到另一个角平滑变色，没有颠倒或错行。

### 第 1 步：实现射线与相机

```cpp
struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;

    glm::vec3 at(float t) const {
        return origin + t * direction;
    }
};
```

最简单的相机位于原点，朝 $-Z$ 方向看。成像平面由左下角、水平轴和竖直轴确定。像素的归一化坐标为 $u,v\in[0,1]$：

```cpp
Ray getRay(float u, float v) {
    glm::vec3 target = lowerLeft + u * horizontal + v * vertical;
    return {origin, glm::normalize(target - origin)};
}
```

暂时不放物体，按射线方向在白色与天蓝色之间插值作为背景。

预期结果：上方蓝、下方白的天空渐变。

### 第 2 步：射线与球求交

球心为 $C$、半径为 $r$。交点满足：

$$
\lVert O+tD-C\rVert^2=r^2
$$

展开为：

$$
(D\cdot D)t^2+2D\cdot(O-C)t+
(O-C)\cdot(O-C)-r^2=0
$$

判别式小于 0 表示没有交点；否则检查两个根，并取位于合法区间内的最小根。使用 `halfB` 可以减少一次乘法：

```cpp
glm::vec3 oc = ray.origin - center;
float a = glm::dot(ray.direction, ray.direction);
float halfB = glm::dot(oc, ray.direction);
float c = glm::dot(oc, oc) - radius * radius;
float discriminant = halfB * halfB - a * c;
```

不要只返回 `bool`，要保存完整交点信息：

```cpp
struct HitRecord {
    glm::vec3 point;
    glm::vec3 normal;
    float t;
    bool frontFace;
    const Material* material;

    void setFaceNormal(const Ray& ray, const glm::vec3& outwardNormal) {
        frontFace = glm::dot(ray.direction, outwardNormal) < 0.0f;
        normal = frontFace ? outwardNormal : -outwardNormal;
    }
};
```

遍历物体时，将当前最近距离作为下一个物体的 `tMax`，就能得到最前面的表面。

预期结果：用法线映射为颜色时，画面中出现彩色球体。

### 第 3 步：抽象场景物体

所有可求交物体实现统一接口：

```cpp
class Hittable {
public:
    virtual ~Hittable() = default;
    virtual bool hit(const Ray& ray, float tMin, float tMax,
                     HitRecord& record) const = 0;
};
```

`HittableList` 保存物体指针并寻找最近交点。随后加入一个半径很大的球作为地面。这个抽象非常关键：材质系统、三角形和 BVH 都依赖它。

预期结果：一个小球落在大球形成的弧形地面上，遮挡关系正确。

### 第 4 步：直接光照与硬阴影

在进入随机路径追踪前，可先实现一个点光源，便于理解局部光照：

```cpp
glm::vec3 L = glm::normalize(lightPos - hit.point);
float diffuse = glm::max(glm::dot(hit.normal, L), 0.0f);
```

从交点向光源发出阴影射线。如果在到达光源前击中物体，该点就在阴影中。射线起点要略微离开表面，或令 `tMin = 0.001f`，防止浮点误差让表面击中自身。

预期结果：球有明暗面，地面上出现轮廓清晰的投影。

### 第 5 步：材质与递归散射

材质只负责回答：射线击中后是否继续、下一条射线是什么、能量衰减是多少。

```cpp
class Material {
public:
    virtual ~Material() = default;
    virtual bool scatter(const Ray& incoming,
                         const HitRecord& hit,
                         glm::vec3& attenuation,
                         Ray& scattered) const = 0;
};
```

递归颜色函数的核心形式：

```cpp
glm::vec3 rayColor(const Ray& ray, const Hittable& world, int depth) {
    if (depth <= 0) return glm::vec3(0.0f);

    HitRecord hit;
    if (world.hit(ray, 0.001f, infinity, hit)) {
        Ray scattered;
        glm::vec3 attenuation;
        if (hit.material->scatter(ray, hit, attenuation, scattered))
            return attenuation * rayColor(scattered, world, depth - 1);
        return glm::vec3(0.0f);
    }
    return background(ray);
}
```

先实现三类材质：

1. **Lambert 漫反射**：在法线半球随机选方向。入门时可用 `normal + randomUnitVector()`；进阶时应使用余弦加权半球采样。
2. **金属**：使用反射方向，再叠加 `roughness * randomInUnitSphere()`；粗糙度限制在 $[0,1]$。
3. **玻璃/电介质**：根据折射率决定折射或反射；发生全反射时只能反射，再使用 Schlick 近似随机选择 Fresnel 反射。

Schlick 近似：

$$
R(\theta)=R_0+(1-R_0)(1-\cos\theta)^5,
\quad R_0=\left(\frac{1-\eta}{1+\eta}\right)^2
$$

预期结果：漫反射球表面柔和、金属球反射环境、玻璃球能看到折射和边缘反射。

### 第 6 步：多重采样抗锯齿

每个像素不要只采样中心，而是在像素范围内加入随机偏移，发出多条射线后取平均：

```cpp
glm::vec3 color(0.0f);
for (int s = 0; s < samplesPerPixel; ++s) {
    float u = (x + randomFloat()) / (width - 1);
    float v = (y + randomFloat()) / (height - 1);
    color += rayColor(camera.getRay(u, v), world, maxDepth);
}
color /= static_cast<float>(samplesPerPixel);
```

采样数越高，噪声通常越小，但时间近似线性增长。调试时可使用 `SPP=1~4`、`maxDepth=5`，最终图片再使用 `SPP=100~500`、`maxDepth=20~50`。

预期结果：球的边缘不再有明显锯齿，漫反射阴影逐渐平滑。

### 第 7 步：线性颜色、Gamma 与色调映射

光照计算必须在线性空间完成。输出到普通显示器前进行 Gamma 校正。Gamma 约为 2 时：

```cpp
color = glm::sqrt(glm::max(color, glm::vec3(0.0f)));
color = glm::clamp(color, 0.0f, 0.999f);
```

如果颜色可能超过 1，可先做简单的 Reinhard 色调映射：

```cpp
color = color / (color + glm::vec3(1.0f));
```

注意顺序：采样平均 → 色调映射 → Gamma 校正 → 转换为 8 位整数。不要在每个样本上提前 Gamma 校正。

### 第 8 步：可移动相机与景深

用 `lookFrom`、`lookAt` 和世界上方向构造相机坐标基：

```cpp
w = normalize(lookFrom - lookAt);  // 相机后方
u = normalize(cross(vUp, w));      // 相机右方
v = cross(w, u);                   // 相机上方
```

景深模拟薄透镜：不再让所有射线从同一点发出，而是在镜头圆盘内随机选择起点，并让它们汇聚到焦平面。需要光圈大小 `aperture` 和焦距 `focusDistance`。

预期结果：焦点附近清晰，前景和背景逐渐模糊。

### 第 9 步：面光源与更完整的路径追踪

让材质增加 `emitted()`，发光材质直接返回辐射亮度。把一个矩形或球体设为面光源，通过随机路径自然产生软阴影。

只靠随机反弹命中小光源会产生大量噪声。进阶实现应显式采样光源，并用多重重要性采样（MIS）结合“采样光源”和“采样 BSDF”两种策略。此时材质最好返回 BSDF 值及采样 PDF，而不只是 `attenuation`。

### 第 10 步：三角形、模型与纹理

射线与三角形求交常用 Möller–Trumbore 算法。交点重心坐标 $(1-u-v,u,v)$ 可插值顶点法线、UV 和其他属性：

$$
A=(1-u-v)A_0+uA_1+vA_2
$$

加入：

- 纹理采样和球面 UV
- 三角网格和 OBJ/glTF 模型
- 法线插值；之后再实现切线空间法线贴图
- 物体变换：把射线变换到物体局部空间求交

注意：变换法线要使用模型矩阵线性部分的逆转置矩阵。

### 第 11 步：BVH 加速

暴力求交复杂度约为“射线数 × 物体数”。BVH 用 AABB 包围一组物体；射线未击中父包围盒时，整个子树都可以跳过。

实现顺序：

1. slab 方法实现射线与 AABB 求交。
2. 为每个物体提供 `boundingBox()`。
3. 沿某个轴按包围盒中心排序并二分构建 BVH。
4. 遍历时先测包围盒，再递归测试左右孩子，并不断缩短最近距离。

预期结果：物体数量增加到数千或数万时，渲染时间不再近似线性爆炸。进一步可研究 SAH（表面积启发式）划分和扁平数组 BVH。

## 5. 随机采样必须注意什么

### 5.1 分布要正确

“随机选三个分量再归一化”通常不会得到球面均匀分布。可以使用拒绝采样生成单位球内随机点，或直接用正确的球坐标分布。Lambert 材质优先使用余弦加权半球采样，因为它与 $n\cdot\omega$ 项匹配，噪声更低。

### 5.2 随机数要可复现

调试时使用固定种子，并为像素/样本构造独立随机状态。否则一次运行一个结果，很难定位错误。并行渲染时不要让所有线程竞争同一个全局随机数生成器。

### 5.3 路径终止

固定最大深度简单但可能浪费计算。进阶时在若干次反弹后使用俄罗斯轮盘赌：根据当前通量选择继续概率，继续时除以该概率以保持估计无偏。

## 6. 迁移到当前 OpenGL 工程

当前 `main.cpp` 绘制的是带纹理的 `BoxGeometry`，顶点/片段着色器仍属于光栅化管线。迁移建议分两阶段。

### 6.1 CPU 计算，OpenGL 只显示

这是最稳妥的第一阶段：

1. CPU 路径追踪器生成 `std::vector<float>` 或 `std::vector<unsigned char>`。
2. 创建 `GL_TEXTURE_2D`，用 `glTexImage2D` 上传结果。
3. 绘制一个覆盖屏幕的三角形或四边形。
4. 片段着色器只执行 `texture(screenTexture, uv)`。

这样可以复用 C++、GLM、窗口和相机输入，并把“渲染算法是否正确”与“GPU 并行是否正确”分开验证。

### 6.2 GLSL 片段着色器实时追踪

OpenGL 3.3 没有计算着色器，可用全屏三角形触发一次片段着色，每个片段追踪一个像素。需要做这些调整：

- 场景物体先放在 uniform 数组中，数量受 uniform 限制。
- GLSL 不支持 C++ 式递归，使用固定上限的 `for` 循环追踪反弹。
- 用像素坐标、帧号构造伪随机种子。
- 使用浮点纹理/FBO 保存历史累积结果。
- 相机或场景变化时把累计帧数清零。

逐帧累积可写成运行平均：

$$
C_n=\frac{(n-1)C_{n-1}+S_n}{n}
$$

其中 $S_n$ 是当前帧的新样本。显示前再做色调映射和 Gamma 校正。

若希望实现 SSBO、计算着色器或硬件光追，应把上下文升级到 OpenGL 4.3+，或进一步学习 Vulkan/DXR/OptiX。学习阶段不必一开始就追求硬件光追 API，它们主要解决工程和性能问题，核心数学仍然相同。

## 7. 一条可执行的里程碑清单

- [ ] 输出一张正确方向的渐变 PPM
- [ ] 实现 `Ray`，显示天空背景
- [ ] 实现球体求交，用法线着色
- [ ] 实现 `HitRecord`、`Hittable` 和最近交点
- [ ] 加入点光源和硬阴影射线
- [ ] 实现 Lambert、Metal、Dielectric
- [ ] 加入多次随机采样和 Gamma 校正
- [ ] 实现可移动相机和景深
- [ ] 加入发光材质与面光源
- [ ] 实现三角形、UV、纹理和模型
- [ ] 实现 AABB 和 BVH
- [ ] 把 CPU 图片作为 OpenGL 纹理显示
- [ ] 改成 GLSL 全屏路径追踪和逐帧累积
- [ ] 学习重要性采样、PDF、NEE 和 MIS

## 8. 常见错误排查

| 现象 | 常见原因 |
| --- | --- |
| 整张图黑色 | 射线方向或相机坐标系反了；递归立即达到深度；颜色转整数过早 |
| 球体内外明暗颠倒 | 法线方向没有用 `frontFace` 统一 |
| 表面有黑点/条纹 | 次级射线击中自身；提高 `tMin` 或沿法线偏移起点 |
| 玻璃球全黑 | 折射率比例用反；未处理全反射；法线朝向错误 |
| 图像特别暗 | 漏做 Gamma 校正；能量衰减过强；采样分布与权重不匹配 |
| 噪声不随采样数下降 | 随机种子重复；累积公式错误；每帧覆盖了历史纹理 |
| 边缘拉伸或画面变形 | 宽高比、FOV 或 `u/v` 计算错误 |
| 移动相机后出现重影 | GPU 累积缓存和帧计数没有清零 |
| BVH 偶尔漏物体 | AABB 边界、负方向倒数或最近距离更新错误 |

## 9. 建议的学习顺序

第一阶段只关注“正确”：CPU、球体、少量材质、低分辨率。第二阶段关注“真实”：渲染方程、PDF、重要性采样、面光源和 MIS。第三阶段关注“快”：多线程、BVH、数据布局、GPU 和渐进累积。最后再学习 PBR 材质、体积散射、去噪以及硬件光追 API。

最重要的调试原则是一次只增加一个效果，并为求交、反射、折射、AABB 等纯数学函数编写小测试。光线追踪器的大多数严重错误最终都来自方向、坐标空间、概率权重或浮点边界，而不是代码量不足。
