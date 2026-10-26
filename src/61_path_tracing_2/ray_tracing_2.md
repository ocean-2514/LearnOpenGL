# 路径追踪进阶：从发光材质到 MIS

> 第 60 课已经完成了基础路径追踪、Lambert 余弦重要性采样、导体镜面、介质反射与折射、多线程分块渲染，以及 `Film / Renderer / PathIntegrator` 的初步拆分。第 61 课的核心目标是让光源真正成为场景中的几何体，并正确、高效地估计它对场景的贡献。

## 1. 本阶段的目标

建议把第 61 课分成四条主线，按顺序完成：

1. **发光材质**：射线直接看到灯时能获得辐射亮度。
2. **面光源和光源采样**：用几何体发光，产生软阴影。
3. **下一事件估计（NEE）与 MIS**：降低间接命中小光源产生的噪声，同时避免重复计算。
4. **渐进式累积**：每帧增加样本，相机移动时安全地重新开始渲染。

完成后，康奈尔盒应当具备：

- 顶棚上的矩形发光面，而不是盒内点光源；
- 能直接看到发光面；
- 面光源产生的软阴影；
- 低采样数下仍相对稳定的直接光照；
- 逐帧收敛的显示效果；
- 相机或场景变化后清空旧样本。

本阶段暂时不急着实现 BVH、模型加载、纹理、参与介质或完整的 Disney BSDF。这些内容应建立在光照估计器正确之后。

## 2. 开始前的基线检查

增加新功能前，先保留一个第 60 课的基准场景和输出，确保后续改动没有破坏已有行为。

建议记录：

- 图片尺寸、SPP、最大深度和随机种子；
- Lambert 球、金属球和玻璃球的参考图；
- 单位测试中的反射、折射和全反射结果；
- `BRDF * cos(theta) / pdf == albedo` 的 Lambert 检查；
- 相同参数下的大致渲染时间。

渲染器加入光源采样后，图片会因估计方法变化而存在随机差异，不能逐像素比较，但平均亮度、材质颜色和遮挡关系不应发生无理由的变化。

## 3. 第一阶段：发光材质

### 3.1 为什么需要发光材质

当前 `PointLight` 是场景之外的一种特殊光源。相机射线或反弹射线无法“击中”它，因此：

- 画面中看不到灯本身；
- 无法自然表达任意形状的灯；
- 点光源只能产生硬阴影；
- 无法统一处理直接命中光源和间接命中光源。

路径追踪中的表面应该既可以反射光，也可以主动发光。渲染方程为：

$$
L_o(x,\omega_o)=L_e(x,\omega_o)+
\int_\Omega f(x,\omega_o,\omega_i)
L_i(x,\omega_i)|n\cdot\omega_i|\,d\omega_i
$$

其中 $L_e$ 是表面自身发出的辐射亮度。

### 3.2 建议接口

先给 `Material` 增加默认不发光的接口：

```cpp
virtual glm::vec3 Emission(const glm::vec3& wo) const {
    return glm::vec3{0.0f};
}

virtual bool IsEmissive() const {
    return false;
}
```

然后增加 `DiffuseLightMaterial`：

```cpp
class DiffuseLightMaterial : public Material {
public:
    explicit DiffuseLightMaterial(const glm::vec3& radiance,
                                  bool twoSided = false);

    glm::vec3 Emission(const glm::vec3& wo) const override;
    bool IsEmissive() const override { return true; }
};
```

单面发光时，只在 $wo$ 位于几何外法线一侧时返回辐射亮度。双面发光可以用于调试，但康奈尔盒顶灯应明确设置正确的几何朝向。

### 3.3 修改积分器

射线命中表面后，首先累加发光项：

```cpp
color += material->Emission(woLocal);
```

发光材质是否继续反射取决于设计：

- 最简单版本：纯发光、不反射，加入发光项后终止路径；
- 更一般版本：发光与反射可以同时存在，继续计算 BSDF 项。

入门阶段建议先做纯发光材质，逻辑更清楚。

### 3.4 验收标准

- 相机直接面对发光矩形时，矩形可见；
- 删除所有 `PointLight` 后，光源本身仍然可见；
- 如果暂时没有显式光源采样，只有随机反弹命中灯时其他物体才会被照亮，图像会很吵，这是预期现象；
- 单面灯背面不应发光。

## 4. 第二阶段：让几何体可以被采样

面光源既是可求交物体，也是可采样物体。为了从着色点主动连接到光源，需要几何体提供表面采样能力。

### 4.1 表面采样结果

建议定义：

```cpp
struct SurfaceSample {
    glm::vec3 position;
    glm::vec3 normal;
    float pdfArea = 0.0f;
};
```

几何接口逐步增加：

```cpp
virtual bool SampleSurface(SurfaceSample& sample) const;
virtual float Area() const;
```

不要让积分器知道“矩形由两个三角形组成”之类的细节。积分器只依赖抽象的表面样本。

### 4.2 三角形均匀采样

设随机数 $\xi_1,\xi_2\in[0,1)$：

$$
u=1-\sqrt{\xi_1},\qquad
v=\xi_2\sqrt{\xi_1}
$$

采样位置：

$$
p=(1-u-v)p_0+up_1+vp_2
$$

面积 PDF：

$$
p_A=\frac{1}{A}
$$

三角形面积：

$$
A=\frac{1}{2}\lVert(p_1-p_0)\times(p_2-p_0)\rVert
$$

项目的 `Sampling.hpp` 已有 `UniformSampleTriangle()`，可以在此基础上补齐面积与 PDF。

### 4.3 矩形/Plane 采样

当前 `Plane` 由两个三角形组成。若两个三角形面积相等，可以各以 $1/2$ 概率选择；更一般的实现应按面积比例选择：

$$
P(T_i)=\frac{A_i}{\sum_j A_j}
$$

组合后的最终面积 PDF 仍应是整个矩形面积的倒数。

如果 `SceneObject` 有缩放变换，不能直接沿用局部空间面积。非均匀缩放会改变面积，采样位置、法线和 PDF 必须变换到世界空间后保持一致。第 61 课可以先让灯直接使用世界空间顶点，避开非均匀缩放问题，再单独完善通用变换。

### 4.4 验收标准

- 将大量采样点可视化时，它们均匀覆盖发光矩形；
- 两个三角形的对角线附近没有密度突变；
- `pdfArea` 始终为正；
- 采样法线方向一致且为单位向量。

## 5. 第三阶段：面光源抽象

### 5.1 光源与几何的关系

推荐让 `AreaLight` 引用一个带发光材质的场景对象，而不是复制一份几何数据：

```cpp
class AreaLight : public Light {
public:
    AreaLight(const SceneObject& object,
              const DiffuseLightMaterial& material);
};
```

也可以让 `Scene` 在添加发光对象时自动把它注册进可采样光源列表。无论采用哪种方式，都应满足：

- 求交使用的发光几何与采样使用的是同一个对象；
- 物体变换后，求交和采样位置不会分离；
- 一个对象的材质不会被销毁后留下悬空指针。

### 5.2 光源样本

点光源当前只返回位置和辐射亮度，面光源需要更完整的信息：

```cpp
struct LightSample {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 wi;
    glm::vec3 radiance;
    float distance = 0.0f;
    float pdf = 0.0f; // 立体角 PDF
};
```

建议接口：

```cpp
virtual bool SampleLi(const glm::vec3& reference,
                      LightSample& sample) const = 0;

virtual float PdfLi(const glm::vec3& reference,
                    const glm::vec3& wi) const = 0;
```

`SampleLi()` 负责从着色点出发采样一个光源方向；`PdfLi()` 用于之后的 MIS 权重计算。

### 5.3 面积 PDF 转成立体角 PDF

从灯表面均匀采样得到的是面积 PDF，但渲染方程积分变量是方向立体角：

$$
p_\omega=p_A\frac{d^2}{|n_L\cdot(-\omega_i)|}
$$

- $p_A$：灯表面的面积 PDF；
- $d$：着色点到灯采样点的距离；
- $n_L$：灯表面法线；
- $\omega_i$：从着色点指向灯的方向。

若灯面余弦小于等于零，单面灯对当前着色点不可见，应返回无效样本。余弦非常小时 PDF 会变大，要避免除零。

### 5.4 替换康奈尔盒点光源

完成面光源后：

1. 在顶棚稍下方创建一个小矩形；
2. 法线朝向盒内，即 $-Y$；
3. 绑定白色 `DiffuseLightMaterial`；
4. 注册为 `AreaLight`；
5. 删除原来的 `PointLight`。

预期结果：顶棚出现可见的亮矩形，球和墙面产生软阴影。

## 6. 第四阶段：下一事件估计（NEE）

### 6.1 为什么需要 NEE

只使用 BSDF 采样时，一条随机反弹射线必须刚好命中较小的灯才有直接光贡献。灯越小，命中概率越低，噪声越大。

下一事件估计在每个非 Delta 表面主动完成：

1. 选择一个光源；
2. 在光源表面采样一点；
3. 向该点发出阴影射线；
4. 未遮挡时计算直接光贡献。

### 6.2 单个光源的估计量

$$
L_{direct}=
\frac{L_i f_r(\omega_o,\omega_i)
|n\cdot\omega_i|}{p_L(\omega_i)}
$$

其中 $p_L$ 是包含“选择该光源”和“在该光源上采样”两部分概率的完整 PDF。

如果场景有 $N$ 个光源并均匀选择一个：

$$
p_{select}=\frac{1}{N},\qquad
p_L=p_{select}\,p_{\omega}
$$

忘记光源选择概率会使场景亮度随光源数量变化。

### 6.3 阴影射线边界

阴影射线的最大距离应略小于采样点距离：

```cpp
tMax = distance - epsilon;
```

否则射线可能把光源几何本身判断为遮挡物。更稳健的长期方案是提供统一的 `SpawnRay()`，根据场景尺度沿几何法线偏移起点，而不是到处写固定 `0.001f`。

### 6.4 Delta 材质

理想镜面和理想玻璃不能通过普通 BRDF 数值在任意光源方向上估计，因此不要在 Delta 表面执行普通 NEE。继续让镜面和折射路径按照当前确定方向传播即可。

### 6.5 验收标准

- `SPP=1~4` 时，漫反射墙面已经能稳定看到直接照明；
- 光源变小时，直接照明不会完全消失；
- 阴影边缘随面光源尺寸增大而变软；
- 光源背面不贡献能量；
- 灯与物体之间有遮挡时不会漏光。

## 7. 第五阶段：多重重要性采样（MIS）

### 7.1 为什么会重复计算

加入面光源后，某条光路可能通过两种方式生成：

- 光源采样直接连接到灯；
- BSDF 采样后恰好命中灯。

两种无偏估计直接完整相加，会重复计算同一积分并使结果过亮。只保留光源采样又会让强镜面方向附近表现较差。MIS 用权重组合二者。

### 7.2 Power heuristic

常用权重：

$$
w_a=\frac{p_a^2}{p_a^2+p_b^2}
$$

```cpp
float PowerHeuristic(float pdfA, float pdfB) {
    float a2 = pdfA * pdfA;
    float b2 = pdfB * pdfB;
    return a2 / (a2 + b2);
}
```

分母为零时应返回零。

### 7.3 需要补齐的 PDF 接口

当前材质只有 `SampleWi(..., pdf)`。为了给光源采样计算另一策略的 PDF，需要增加：

```cpp
virtual float Pdf(const glm::vec3& wo,
                  const glm::vec3& wi) const;
```

Lambert：

$$
p_B(\omega_i)=\frac{\max(0,\cos\theta)}{\pi}
$$

光源也需要前面提到的 `PdfLi()`，以便 BSDF 路径命中发光面时计算光源策略 PDF。

### 7.4 光源采样分支

光源采样贡献乘以：

$$
w_L=\frac{p_L^2}{p_L^2+p_B^2}
$$

### 7.5 BSDF 采样分支

如果 BSDF 采样射线命中发光表面，其发光贡献乘以：

$$
w_B=\frac{p_B^2}{p_B^2+p_L^2}
$$

Delta 事件没有普通连续 PDF，应作为特殊情况处理，通常令它命中灯时权重为 1。

### 7.6 必须跟踪上一跳信息

为了在路径命中发光面时计算 MIS，需要知道：

- 上一跳是否为 Delta 事件；
- 上一跳的 BSDF PDF；
- 上一个交点位置。

递归函数的参数会因此逐渐增多。这是把积分器改为迭代式路径循环的合适时机，但不要在 NEE 尚未正确时同时重写所有逻辑。

### 7.7 验收标准

- NEE 与 BSDF 采样同时开启后，图片不会变成约两倍亮；
- 大光源、粗糙表面附近光源采样表现稳定；
- 小光源或集中 BSDF 分布下不会出现明显异常高方差；
- 关闭其中一种策略时，另一种策略仍保持无偏，只是噪声不同。

## 8. 第六阶段：渐进式累积

### 8.1 当前一次性渲染的限制

当前 `Renderer` 一次完成固定数量 SPP，并把 Gamma 后的 8 位结果发布到 `Film`。这适合离线图片，但不方便：

- 观察图像逐渐收敛；
- 动态调整采样数；
- 相机移动后重新开始；
- 保存 HDR 中间结果。

### 8.2 Film 保存线性浮点累积

建议让 `Film` 保存：

```cpp
std::vector<glm::vec3> accumulatedRadiance;
std::vector<uint32_t> sampleCount;
```

加入样本：

```cpp
accumulatedRadiance[index] += sample;
sampleCount[index] += 1;
```

显示颜色：

```cpp
linear = accumulatedRadiance[index] / sampleCount[index];
display = GammaCorrect(ToneMap(linear));
```

Gamma 校正只能用于显示，不能写回线性累积数据。

### 8.3 渲染任务生命周期

建议 `Renderer` 逐渐支持：

```cpp
Start(...);
Cancel();
Reset();
IsRunning();
```

相机或场景发生变化时：

1. 通知旧任务取消；
2. 等待工作线程到达安全点；
3. 清空 Film；
4. 复制新的相机和场景状态；
5. 启动新一轮累积。

不要让工作线程直接读取主线程正在修改的相机。

### 8.4 验收标准

- 窗口从低噪声预览逐渐变清晰；
- 显示的累计 SPP 与实际一致；
- 相机不动时亮度不会随帧数变化；
- 相机移动后没有旧画面重影；
- 取消渲染后程序可以快速退出。

## 9. 建议的积分器演进

当前递归形式清晰，适合完成发光材质和最初的 NEE。MIS 需要保存上一跳状态时，可以改为迭代形式：

```cpp
glm::vec3 radiance{0.0f};
glm::vec3 throughput{1.0f};
Ray ray = cameraRay;

for (int depth = 0; depth < maxDepth; ++depth) {
    // hit
    // accumulate emission
    // next-event estimation
    // sample BSDF
    // update throughput
    // Russian roulette
    // spawn next ray
}
```

通量更新的通用形式：

$$
\beta\leftarrow\beta
\frac{f(\omega_o,\omega_i)|n\cdot\omega_i|}{p(\omega_i)}
$$

迭代式积分器的优点：

- 不依赖 C++ 调用栈；
- 更容易记录上一跳 PDF 和 Delta 状态；
- 更容易实现路径调试输出；
- 更容易统计射线数、平均深度和终止原因；
- 将来迁移到 GPU 时结构更接近着色器循环。

改写时应先保留递归版本作为参考，对简单场景比较平均亮度。

## 10. 推荐的代码架构调整

第 61 课不需要一次完成所有抽象，但建议沿着下面的依赖方向演进：

```text
Camera ──生成主射线──> Renderer
                         │
                         ▼
Film <──写入样本── PathIntegrator
                         │
            ┌────────────┼────────────┐
            ▼            ▼            ▼
          Scene        Material      Light
            │             │            │
            ▼             ▼            ▼
       Hittable/Shape   BSDF采样     光源采样
```

### 10.1 Core

- `Ray`：射线数据；
- `HitRecord`：交点数据；
- `Frame`：局部/世界坐标转换；
- `Camera`：主射线；
- `Film`：线性累积和显示转换。

### 10.2 Sampler

目前是全局 `thread_local` 随机数函数。短期可以继续使用。需要可复现测试时，再引入显式 `Sampler` 对象：

```cpp
Sampler sampler(pixelIndex, sampleIndex, seed);
```

这样每个像素的结果不依赖线程调度顺序。

### 10.3 Material/BSDF

短期保留现有接口，先增加：

- `Emission()`；
- `Pdf()`；
- 对 Delta/非 Delta 的明确判断。

长期可以把 `BRDF / BTDF / SampleWi / SampleWt` 合成统一的 BSDF 采样结果，但不必在实现面光源前立即重构。

### 10.4 Shape 与 SceneObject

目前 `Hittable` 只负责求交。为了面光源，应逐步增加：

- 表面积；
- 表面采样；
- 面积 PDF；
- 必要时反查某个方向命中该几何的 PDF。

如果以后材质、几何和变换的关系越来越复杂，可以明确区分：

- `Shape`：纯几何和局部求交；
- `Primitive`：Shape + Material；
- `SceneObject/Instance`：Primitive + Transform。

### 10.5 Light

统一点光源、方向光和面光源对积分器的接口：

- `SampleLi()`；
- `PdfLi()`；
- `IsDelta()`；
- `Emission()` 或 `Le()`。

积分器不应通过具体光源类型的 `if/else` 工作。

### 10.6 Integrator

`PathIntegrator` 只负责光传输估计，不负责：

- 创建线程；
- 写 PNG；
- 调用 OpenGL；
- 响应键盘和鼠标；
- 构建具体场景。

这可以保证积分器能够被单元测试或离线命令行程序直接调用。

### 10.7 Renderer

`Renderer` 负责：

- tile 或扫描线调度；
- 线程生命周期；
- 每像素和每样本循环；
- 取消、重启和进度统计；
- 把积分器结果提交给 Film。

## 11. 更后面的学习路线

完成本章主线后，可按兴趣选择以下方向。

### 11.1 环境光与 HDRI

- 射线未命中场景时查询环境贴图；
- 经纬度环境贴图方向映射；
- 按亮度乘以 $\sin\theta$ 构建二维重要性分布；
- 环境光采样与 BSDF 采样做 MIS。

### 11.2 粗糙导体和粗糙介质

- 微表面理论；
- GGX/Trowbridge-Reitz NDF；
- Smith masking-shadowing；
- Fresnel；
- 可见法线采样 VNDF；
- 粗糙反射与粗糙透射。

这是从理想镜面走向现代 PBR 材质的关键一步。

### 11.3 纹理与模型

- 在 `HitRecord` 中加入 UV；
- 常量纹理、图像纹理、棋盘纹理；
- 顶点法线插值；
- OBJ/glTF 网格；
- 法线贴图与切线空间。

### 11.4 景深和运动模糊

- 薄透镜相机；
- 镜头圆盘采样；
- 射线时间；
- 运动物体的时间相关变换。

### 11.5 参与介质

- Beer-Lambert 衰减；
- 自由飞行距离采样；
- Henyey-Greenstein 相函数；
- 体积散射与表面路径统一。

### 11.6 加速结构

当场景达到数百或数千个三角形后，再实现：

- AABB slab 求交；
- BVH 构建与遍历；
- SAH；
- 实例变换和网格级/场景级两层 BVH。

BVH 提升速度，但不会修复错误的光照估计，因此应放在发光、PDF 和 MIS 正确之后。

## 12. 第 61 课建议里程碑

- [ ] 保存第 60 课基准图和参数
- [ ] 为材质增加 `Emission()` 和 `IsEmissive()`
- [ ] 实现纯发光的 `DiffuseLightMaterial`
- [ ] 积分器支持直接命中发光表面
- [ ] 为 Triangle 实现面积和均匀表面采样
- [ ] 为 Plane 实现组合面积采样
- [ ] 定义 `SurfaceSample` 和 `LightSample`
- [ ] 实现 `AreaLight::SampleLi()`
- [ ] 正确完成面积 PDF 到立体角 PDF 的转换
- [ ] 用矩形面光源替换康奈尔盒点光源
- [ ] 实现 NEE 和可见性检测
- [ ] 为 Lambert 增加 `Pdf(wo, wi)`
- [ ] 实现 power heuristic
- [ ] 对光源采样和 BSDF 采样应用 MIS
- [ ] 正确处理 Delta 路径命中发光面
- [ ] Film 保存线性浮点累计值和样本数
- [ ] Renderer 支持逐轮增加样本
- [ ] 相机变化后取消任务并清空累积
- [ ] 增加 PDF、能量和方向相关的数值测试

## 13. 调试顺序与常见错误

| 现象 | 优先检查 |
| --- | --- |
| 灯可见但照不亮物体 | 积分器只加了 Emission，没有 NEE 或随机路径未命中灯 |
| 整体亮度随光源数量增加 | 忘记光源选择 PDF |
| 面光源结果极亮 | 面积 PDF 没有转换成立体角 PDF，或漏除 PDF |
| 面光源结果极暗 | 多除了一次距离平方，或发光面法线方向反了 |
| 光源被自己遮挡 | 阴影射线 `tMax` 到达了灯表面，或起点偏移错误 |
| 阴影边缘仍然完全锐利 | 实际仍在采样固定光源位置，或面采样退化成一点 |
| 开启 MIS 后亮度翻倍 | 两个策略没有权重，或者权重用错了对应 PDF |
| 开启 MIS 后出现黑点 | PDF 为零仍参与除法，或方向不在材质支持半球 |
| 镜面看不到灯 | Delta 路径命中灯被错误套用普通 MIS 权重 |
| 累积图越来越亮 | 累加后没有除以样本数 |
| 相机移动出现重影 | Film 未清空，或旧渲染任务仍在写入 |
| 结果每次运行完全不同且难调试 | RNG 没有固定种子或显式像素采样状态 |

## 14. 推荐实施原则

每次只引入一个新的概率因素，并用一个能独立验证的场景测试它：

1. 只实现 Emission，验证灯本身；
2. 只实现几何采样，可视化采样点；
3. 只开启光源采样，检查软阴影和平均亮度；
4. 再开启 BSDF 命中灯，观察重复计算；
5. 最后加入 MIS，并比较不同 SPP 下的方差。

任何时候都明确写出当前样本的完整概率：选择了什么、条件概率是什么、最终 PDF 相对于面积还是立体角。路径追踪中“画面看起来差不多”不能证明估计正确；PDF 单位一致、能量缩放正确、采样数量增加时均值稳定，才是更可靠的判断标准。
