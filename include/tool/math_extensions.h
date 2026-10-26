#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/glm.hpp>
#include <cmath>     // atan2 / sin / cos / std::abs / std::sqrt / std::log / std::exp
#include <utility>   // std::pair
#include <algorithm> // std::max
#include <vector>
#include <array>

#include "geometry/BufferGeometry.h"

//单位四元数取对数
glm::quat quat_log(glm::quat q) {
    glm::vec3 v = glm::vec3(q.x, q.y, q.z);
	float c = q.w;
	float norm = glm::length(v);
	if (norm < 1e-6) {
		return glm::quat(0.0f, 0.0f, 0.0f, 0.0f);
	}
	glm::vec3 u = v / norm;
	float t2 = atan2(norm, c);
	return glm::quat(0.0f, u.x * t2, u.y * t2, u.z * t2);
}

glm::quat quat_exp(glm::quat q) {
    glm::vec3 v = glm::vec3(q.x, q.y, q.z);
	float norm = glm::length(v);
	if (norm < 1e-6f) {
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    }
	glm::vec3 u = v / norm;
	float t2 = norm;
	float s = sin(t2);
	return glm::quat(cos(t2), u.x * s, u.y * s, u.z * s);
}

glm::quat slerp_method(glm::quat q1, glm:: quat q2, float t) {
    q1 = glm::normalize(q1);
    q2 = glm::normalize(q2);
    if (glm::dot(q1, q2) < 0.0f) q2 = -q2;

	glm::quat qdel = glm::inverse(q1) * q2;
	return q1 * quat_exp(quat_log(qdel) * t);
}

glm::mat3 slerp_method(glm::mat3 M1, glm::mat3 M2, float t) {
    return glm::mat3_cast(slerp_method(glm::quat_cast(M1), glm::quat_cast(M2), t));
}

std::pair<glm::mat3, glm::mat3> polar_decompose_newton(const glm::mat3& M, int max_iter = 30, float tolerance = 1e-6) {
    float det = glm::determinant(M);
    if (std::abs(det) < 1e-8) {
        return {glm::mat3{1.0f}, glm::mat3{1.0f}};
    }

    glm::mat3 Y = M;
    glm::mat3 Y_prev;

    for (int i = 0; i < max_iter; ++i) {
        Y_prev = Y;
        Y = 0.5f * (Y + glm::transpose(glm::inverse(Y)));

        // GLM 的 length() 只支持向量/四元数，不支持矩阵；
        // 这里用 Frobenius 范数（所有元素平方和开方）做收敛判断
        float diff = 0.0f;
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c) {
                float d = Y[r][c] - Y_prev[r][c];
                diff += d * d;
            }
        diff = std::sqrt(diff);
        if (diff < tolerance) break;
    }

    glm::mat3 U = Y;
    if (glm::determinant(U) < 0) {
        U = -U;
    }

    glm::mat3 P = glm::transpose(U) * M;
    P = 0.5f * (P + glm::transpose(P));

    return {U, P};
}

bool isOrthogonal(const glm::mat3& M, float epsilon = 1e-5f) {
    glm::mat3 I = M * glm::transpose(M);
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            if (std::abs(I[i][j] - (i == j ? 1.0f : 0.0f)) > epsilon)
                return false;
    return true;
}

// ---------- 对称矩阵的矩阵对数 / 指数（用于 P 的对数空间插值） ----------
// 说明：GLM 中 mat3 是列主序，M[c][r] 表示第 c 列、第 r 行的元素。
// 这里提供按 (row, col) 读写的辅助函数，避免索引混淆。

// 读取元素 (row, col)
static inline float mat3_el(const glm::mat3& M, int r, int c) { return M[c][r]; }
// 写入元素 (row, col)
static inline void mat3_set(glm::mat3& M, int r, int c, float v) { M[c][r] = v; }

// 对称 3x3 矩阵的 Jacobi 特征分解。
// 返回 {Q, eigenvalues}，Q 的列是特征向量，满足 A ≈ Q * diag(evals) * Q^T
static inline std::pair<glm::mat3, glm::vec3> symmetric_eigen_3x3(const glm::mat3& A) {
    glm::mat3 Q(1.0f);   // 累积特征向量
    glm::mat3 B = A;     // 工作副本，迭代后对角元素为特征值

    for (int iter = 0; iter < 50; ++iter) {
        // 找到绝对值最大的非对角元素 B[p][q]
        int p = 0, q = 1;
        float maxOff = 0.0f;
        for (int i = 0; i < 3; ++i)
            for (int j = i + 1; j < 3; ++j) {
                float a = std::abs(mat3_el(B, i, j));
                if (a > maxOff) { maxOff = a; p = i; q = j; }
            }
        if (maxOff < 1e-12f) break;   // 已对角化

        // Givens 旋转角度，消除 B[p][q]
        float app = mat3_el(B, p, p);
        float aqq = mat3_el(B, q, q);
        float apq = mat3_el(B, p, q);
        float theta = 0.5f * std::atan2(2.0f * apq, aqq - app);
        float c = std::cos(theta), s = std::sin(theta);

        // 更新除 p,q 外的行/列（保持对称性）
        for (int k = 0; k < 3; ++k) {
            if (k == p || k == q) continue;
            float akp = mat3_el(B, k, p), akq = mat3_el(B, k, q);
            mat3_set(B, k, p, c * akp - s * akq);
            mat3_set(B, p, k, c * akp - s * akq);
            mat3_set(B, k, q, s * akp + c * akq);
            mat3_set(B, q, k, s * akp + c * akq);
        }
        // 更新对角线
        mat3_set(B, p, p, c * c * app - 2.0f * s * c * apq + s * s * aqq);
        mat3_set(B, q, q, s * s * app + 2.0f * s * c * apq + c * c * aqq);
        mat3_set(B, p, q, 0.0f);
        mat3_set(B, q, p, 0.0f);

        // 累积特征向量到 Q（Q 的列为特征向量）
        for (int k = 0; k < 3; ++k) {
            float qkp = mat3_el(Q, k, p), qkq = mat3_el(Q, k, q);
            mat3_set(Q, k, p, c * qkp - s * qkq);
            mat3_set(Q, k, q, s * qkp + c * qkq);
        }
    }

    return {Q, glm::vec3(mat3_el(B, 0, 0), mat3_el(B, 1, 1), mat3_el(B, 2, 2))};
}

// 对称矩阵的矩阵对数：log(A) = Q * diag(ln λi) * Q^T
// 要求 A 正定；对非正特征值做 clamp 保护避免 NaN
static inline glm::mat3 mat_log_sym(const glm::mat3& A) {
    auto [Q, evals] = symmetric_eigen_3x3(A);
    glm::mat3 logD(0.0f);
    for (int i = 0; i < 3; ++i)
        logD[i][i] = std::log(std::max(evals[i], 1e-10f));
    return Q * logD * glm::transpose(Q);
}

// 对称矩阵的矩阵指数：exp(A) = Q * diag(e^λi) * Q^T
static inline glm::mat3 mat_exp_sym(const glm::mat3& A) {
    auto [Q, evals] = symmetric_eigen_3x3(A);
    glm::mat3 expD(0.0f);
    for (int i = 0; i < 3; ++i)
        expD[i][i] = std::exp(evals[i]);
    return Q * expD * glm::transpose(Q);
}

glm::mat3 mat_lerp(const glm::mat3& M1, const glm::mat3& M2, float t) {
    if (isOrthogonal(M1) && isOrthogonal(M2)) {
        return slerp_method(M1, M2, t);
    }

    auto [U1, P1] = polar_decompose_newton(M1);
    auto [U2, P2] = polar_decompose_newton(M2);

    glm::mat3 U = slerp_method(U1, U2, t);

    // P 的对数空间插值（Log-Euclidean 插值）：
    //   P(t) = exp( (1-t)*log(P1) + t*log(P2) )
    // 相比直接线性插值 (1-t)P1 + tP2：
    //   - 插值结果保持对称正定（线性插值可能破坏正定性）
    //   - 缩放/剪切随 t 更平滑地"流动"，不产生中间漂移
    glm::mat3 logP = mat_log_sym(P1) * (1.0f - t) + mat_log_sym(P2) * t;
    glm::mat3 P = mat_exp_sym(logP);

    return U * P;
}

glm::mat4 mat_lerp(const glm::mat4& M1, const glm::mat4& M2, float t) {
    glm::mat3 R1{M1};
    glm::mat3 R2{M2};
    glm::vec3 p1{M1[3]};
    glm::vec3 p2{M2[3]};

    glm::mat3 R = mat_lerp(R1, R2, t);
    glm::vec3 p = p1 * (1 - t) + p2 * t;

    glm::mat4 res{R};
    res[3] = glm::vec4(p, 1.0f);
    return res;
}


//bezier curve
float bernstein_k3(int i, float t) {
    switch (i)
    {
        case 0: return (1 - t) * (1 - t) * (1 - t);
        case 1: return 3 * (1 - t) * (1 - t) * t;
        case 2: return 3 * (1 - t) * t * t;
        case 3: return t * t * t;
        
        default: return 0.0f;
    }
}

template<typename T>
T bezier_curve_point(const std::vector<T>& points, float t) {
    vector<T> v = points;
    while (v.size() > 1)
    {
        vector<T> tem;
        tem.reserve(v.size() - 1);
        for (int i = 0; i < v.size() - 1; ++i) {
            tem.emplace_back(v[i] * (1 - t) + v[i + 1] * t);
        }
        v = std::move(tem);
    }
    return v[0];
}

template<typename T>
std::vector<T> bezier_curve(const std::vector<T>& points, int segments = 100) {
    vector<T> res;
    res.reserve(segments + 1);
    for (int i = 0; i <= segments; ++i) {
        res.emplace_back(bezier_curve_point(points, (float)i / segments));
    }
    return res;
}

template<typename T>
std::vector<T> bezier_curve_formula_k3(const std::vector<T>& points, int segments = 100) {
    if (points.size() != 4) return {};

    std::vector<std::array<float, 4>> basis;
    basis.reserve(segments + 1);
    for (int i = 0; i <= segments; ++i) {
        float f = (float)i / segments;
        basis.emplace_back(std::array<float, 4>{
            bernstein_k3(0, f),
            bernstein_k3(1, f),
            bernstein_k3(2, f),
            bernstein_k3(3, f)
        });
    }

    std::vector<T> res;
    res.reserve(segments + 1);


    for (int i = 0; i <= segments; ++i) {
        T sum{};
        for (int j = 0; j < points.size(); ++j) {
            sum += points[j] * basis[i][j];
        }
        res.emplace_back(sum);
    }
    return res;
}

template<typename T>
T bezier_surface_point(const std::vector<std::vector<T>>& points, float u, float v) {
    std::vector<T> bezierCurvePoints;
    bezierCurvePoints.reserve(points.size());

    for (int i = 0; i < points.size(); ++i) {
        bezierCurvePoints.emplace_back(bezier_curve_point(points[i], u));
    }
    return bezier_curve_point(bezierCurvePoints, v);
}

template<typename T>
std::vector<std::vector<T>> bezier_surface(
    const std::vector<std::vector<T>>& points, 
    int u_segments = 100, int v_segments = 100) {
    std::vector<std::vector<T>> res(static_cast<size_t>(u_segments + 1), std::vector<T>(static_cast<size_t>(v_segments + 1)));
    for (int i = 0; i <= u_segments; ++i) {
        for (int j = 0; j <= v_segments; ++j) {
            res[i][j] = bezier_surface_point(points, (float)i / u_segments, (float)j / v_segments);
        }
    }
    return res;
}

// ---------- Bernstein 公式版曲面（4×4 控制网格，三次张量积） ----------
//  S(u,v) = Σ_{k=0}^3 Σ_{l=0}^3  B_{k,3}(u) B_{l,3}(v) P_{kl}
template<typename T>
std::vector<std::vector<T>> bezier_surface_formula_k3(
    const std::vector<std::vector<T>>& points,
    int u_segments = 100, int v_segments = 100)
{
    if (points.size() != 4 || points[0].size() != 4)
        return {};

    // 预计算 u / v 方向各采样点的 Bernstein 基函数值
    std::vector<std::array<float, 4>> basis_u(u_segments + 1);
    for (int i = 0; i <= u_segments; ++i) {
        float f = (float)i / u_segments;
        basis_u[i] = {bernstein_k3(0, f), bernstein_k3(1, f),
                      bernstein_k3(2, f), bernstein_k3(3, f)};
    }
    std::vector<std::array<float, 4>> basis_v(v_segments + 1);
    for (int j = 0; j <= v_segments; ++j) {
        float f = (float)j / v_segments;
        basis_v[j] = {bernstein_k3(0, f), bernstein_k3(1, f),
                      bernstein_k3(2, f), bernstein_k3(3, f)};
    }

    std::vector<std::vector<T>> surf(u_segments + 1, std::vector<T>(v_segments + 1));
    for (int i = 0; i <= u_segments; ++i) {
        for (int j = 0; j <= v_segments; ++j) {
            T sum{};   // 零初始化（对 glm::vec2 / vec3 即 {0,0} / {0,0,0}）
            for (int k = 0; k < 4; ++k)
                for (int l = 0; l < 4; ++l)
                    sum += points[k][l] * (basis_u[i][k] * basis_v[j][l]);
            surf[i][j] = sum;
        }
    }
    return surf;
}



//interpolation
template<typename T>
std::vector<T> compute_tangents_catmull_rom(const std::vector<T>& points, float tension = 0.5f) {
    size_t n = points.size();
    std::vector<T> tangents{n};

    for (int i = 0; i < n; ++i) {
        if (i == 0) {
            tangents[i] = tension * (points[1] - points[0]);
        } else if (i == n - 1) {
            tangents[i] = tension * (points[n - 1] - points[n - 2]);
        } else {
            tangents[i] = tension * (points[i + 1] - points[i - 1]);
        }
    }

    return tangents;
}

template<typename T>
std::vector<T> hermite_curve_interpolation(const std::vector<T> points, int segments = 30) {
    int n = points.size();
    std::vector<T> tangents = compute_tangents_catmull_rom(points);
    std::vector<T> result;
    result.reserve((n - 1) * segments + 1);

    std::vector<std::array<float, 4>> basis;
    basis.reserve(segments + 1);
    for (int i = 0; i <= segments; ++i) {
        float t = (float)i / segments;
        float t2 = t * t, t3 = t2 * t;
        // 标准三次 Hermite 基函数
        basis.emplace_back(std::array<float, 4>{
             2.f * t3 - 3.f * t2 + 1.f,   // h00(t)：p0 的权重（修正：3→2）
            -2.f * t3 + 3.f * t2,         // h10(t)：p1 的权重
                  t3 - 2.f * t2 + t,       // h01(t)：m0 的权重
                  t3 -       t2           // h11(t)：m1 的权重
        });
    }

    for (int i = 0; i < n - 1; ++i) {
        const T& p0 = points[i];
        const T& p1 = points[i + 1];
        const T& t0 = tangents[i];
        const T& t1 = tangents[i + 1];

        for (int j = 0; j < segments; ++j) {
            T p = p0 * basis[j][0] + p1 * basis[j][1] + t0 * basis[j][2] + t1 * basis[j][3];
            result.emplace_back(p);
        }
    }

    result.emplace_back(points[n - 1]);

    return result;
}


// ---------- Catmull‑Rom 样条（四点直接公式，不显式计算切线） ----------
//  C(t) = 0.5 * [ (-t³+2t²-t)P₋₁ + (3t³-5t²+2)P₀ + (-3t³+4t²+t)P₁ + (t³-t²)P₂ ]
// 边界用 clamp-to-edge 处理
template<typename T>
std::vector<T> catmull_rom_spline(const std::vector<T>& points, int segments = 20) {
    int n = points.size();
    if (n < 2) return {};

    // 预计算 t 对应到四个基函数
    std::vector<std::array<float, 4>> basis(segments + 1);
    for (int i = 0; i <= segments; ++i) {
        float t = (float)i / segments;
        float t2 = t * t, t3 = t2 * t;
        basis[i] = {
            -0.5f * t3 +       t2 - 0.5f * t,    // P_{k-1}
             1.5f * t3 - 2.5f * t2 + 1.0f,       // P_k
            -1.5f * t3 + 2.0f * t2 + 0.5f * t,   // P_{k+1}
             0.5f * t3 - 0.5f * t2               // P_{k+2}
        };
    }

    // 边界索引 clamp
    auto pt = [&](int idx) -> const T& {
        if (idx < 0)       return points[0];
        if (idx >= n)      return points[n - 1];
        return points[idx];
    };

    std::vector<T> result;
    result.reserve((n - 1) * segments + 1);

    for (int i = 0; i < n - 1; ++i) {
        const T& p0 = pt(i - 1);
        const T& p1 = points[i];
        const T& p2 = points[i + 1];
        const T& p3 = pt(i + 2);

        for (int j = 0; j < segments; ++j)
            result.emplace_back(p0 * basis[j][0] + p1 * basis[j][1]
                              + p2 * basis[j][2] + p3 * basis[j][3]);
    }
    result.emplace_back(points[n - 1]);

    return result;
}


//b-spline —— Cox‑de Boor 递归定义
//  N_{i,0}(t) = 1  if t_i ≤ t < t_{i+1}, else 0
//  N_{i,k}(t) = (t-t_i)/(t_{i+k}-t_i)·N_{i,k-1}(t) + (t_{i+k+1}-t)/(t_{i+k+1}-t_{i+1})·N_{i+1,k-1}(t)
float bslipe_basis(int i, int k, float t, const std::vector<float>& knots) {
    if (k == 0) {
        return (t >= knots[i] && t < knots[i + 1]) ? 1.0f : 0.0f;
    }

    float left = 0.0f, right = 0.0f;

    float denomL = knots[i + k] - knots[i];          // 修正：k-1 → k
    if (denomL != 0.0f)
        left = (t - knots[i]) / denomL * bslipe_basis(i, k - 1, t, knots);

    float denomR = knots[i + k + 1] - knots[i + 1];  // 修正：k → k+1
    if (denomR != 0.0f)
        right = (knots[i + k + 1] - t) / denomR * bslipe_basis(i + 1, k - 1, t, knots);

    return left + right;
}


std::vector<float> generate_knots_clamped(int n, int k) {
    int numKnots = n + k + 1;
    std::vector<float> knots(numKnots);
    for (int i = 0; i <= k; ++i)        knots[i] = 0.0f;     // 前 k+1 个 = 0
    for (int i = n; i < numKnots; ++i)  knots[i] = 1.0f;     // 后 k+1 个 = 1
    int inner = n - k - 1;
    for (int i = 1; i <= inner; ++i)
        knots[k + i] = (float)i / (inner + 1);                // 中间均匀
    return knots;
}


// B‑spline 曲线（clamped 节点向量，曲线通过首尾控制点）
template<typename T>
std::vector<T> bslipe_curve(const std::vector<T>& points, int degree = 3, int segments = 100) {
    int n = (int)points.size();
    if (n < 2) return {};

    int k = degree;
    if (k > n - 1) k = n - 1;

    std::vector<float> knots = generate_knots_clamped(n, k);

    std::vector<T> result;
    result.reserve(segments + 1);

    for (int i = 0; i <= segments; ++i) {
        float t = (float)i / segments;
        T sum{};
        for (int j = 0; j < n; ++j)
            sum += points[j] * bslipe_basis(j, k, t, knots);
        result.emplace_back(sum);
    }

    return result;
}

template<typename T>
std::vector<std::vector<T>> bslipe_surface(
    const std::vector<std::vector<T>>& points, 
    int degree = 3, int u_segments = 100, int v_segments = 100) {

    int n1 = (int)points.size();
    if (n1 == 0) return {};
    int n2 = (int)points[0].size();
    if (n2 == 0) return {};

    int k = degree;
    if (k > n1 - 1) k = n1 - 1;
    if (k > n2 - 1) k = n2 - 1;

    std::vector<std::vector<T>> res(static_cast<size_t>(u_segments + 1),
                                    std::vector<T>(static_cast<size_t>(v_segments + 1)));
    
    std::vector<float> u_knots = generate_knots_clamped(n1, k);
    std::vector<float> v_knots = generate_knots_clamped(n2, k);

    for (int i = 0; i <= u_segments; ++i) {
        for (int j = 0; j <= v_segments; ++j) {
            float u = (float)i / u_segments;
            float v = (float)j / v_segments;

            T sum{};
            for (int x = 0; x < n1; ++x)
                for (int y = 0; y < n2; ++y)
                    sum += points[x][y] * bslipe_basis(x, k, u, u_knots) 
                                        * bslipe_basis(y, k, v, v_knots);
            res[i][j] = sum;
        }
    }
    
    return res;
}

