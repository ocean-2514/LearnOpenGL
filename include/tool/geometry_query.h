#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <optional>


template<typename T> 
T closest_point_to_line(const T& a, const T& b, const T& p) {
    T d = b - a;
    float lenSq = glm::dot(d, d);
    if (lenSq < 1e-8) return a;

    float t = glm::dot(p - a, d) / lenSq;

    return a + t * d;
}

template<typename T> 
T closest_point_to_segment(const T& a, const T& b, const T& p) {
    T d = b - a;
    float lenSq = glm::dot(d, d);
    if (lenSq < 1e-8) return a;

    float t = glm::dot(p - a, d) / lenSq;

    t = glm::clamp(t, 0.0f, 1.0f);

    return a + t * d;
}

template<typename T>
T distance_point_to_line(const T& a, const T& b, const T& p) {
    T c = closest_point_to_line(a, b, p);
    return glm::distance(p, c);
}

template<typename T>
T distance_point_to_segment(const T& a, const T& b, const T& p) {
    T c = closest_point_to_segment(a, b, p);
    return glm::distance(p, c);
}

//plane: n^T * p = c
inline std::optional<glm::vec3> intersection_line_and_plane(const glm::vec3& o, const glm::vec3& d, 
    const glm::vec3& n, const float c) {
        
    float denom = glm::dot(n, d);
    if (std::abs(denom) < 1e-6) {
        return std::nullopt;
    }
    
    float t = (c - glm::dot(n, o)) / denom;
    return o + t * d;
}
    
//plane: n^T * p = c
inline std::optional<glm::vec3> intersection_line_and_plane(const glm::vec3& a, const glm::vec3& b, 
    const glm::vec3& n, const float c) {

    return intersection_line_and_plane(a, b - a, n, c);
}

//plane: n^T * p = c, n is normalized
inline std::optional<glm::vec3> intersection_line_and_plane_normalized(const glm::vec3& a, const glm::vec3& b,
    const glm::vec3& n, const float c) {

    return intersection_line_and_plane_normalized(a, b - a, n, c);
}

//plane: n^T * p = c
inline std::optional<glm::vec3> intersection_ray_and_plane(const glm::vec3& o, const glm::vec3& d, 
    const glm::vec3& n, const float c) {

    float denom = glm::dot(n, d);
    if (std::abs(denom) < 1e-6) {
        return std::nullopt;
    }

    float t = (c - glm::dot(n, o)) / denom;
    if (t < 0) return std::nullopt;

    return o + t * d;
}

//plane: n^T * p = c
inline glm::vec3 closest_point_to_plane(const glm::vec3& n, const float c, const glm::vec3& p) {
    float lenSq = glm::dot(n, n);
    if (lenSq < 1e-8f) return p;
    return p - n * (glm::dot(n, p) - c) / lenSq;
}

//plane: n^T * p = c, n is normalized (||n|| = 1)
inline glm::vec3 closest_point_to_plane_normalized(const glm::vec3& n, const float c, const glm::vec3& p) {
    // dot(n,n)=1, 无需除以 lenSq
    return p - n * (glm::dot(n, p) - c);
}

//plane: n^T * p = c
inline float distance_point_to_plane(const glm::vec3& n, const float c, const glm::vec3& p) {
    float len = glm::length(n);
    if (len < 1e-8f) return 0.0f;
    return std::abs(glm::dot(n, p) - c) / len;
}

//plane: n^T * p = c, n is normalized (||n|| = 1)
inline float distance_point_to_plane_normalized(const glm::vec3& n, const float c, const glm::vec3& p) {
    // dot(n,p)-c 即为有符号距离, 无需除以 len
    return std::abs(glm::dot(n, p) - c);
}

template <typename T>
bool is_point_on_line(const T& a, const T& b, const T& p) {
    T d = b - a;
    T v = p - a;
    
    float lenSq = glm::length2(d);
    if (lenSq < epsilon * epsilon) {
        return glm::length2(v) < epsilon * epsilon;  
    }
    
    // 计算点到直线的距离
    float crossLenSq = glm::length2(glm::cross(v, d));  // 2D 用叉积，3D 用叉积的平方
    return crossLenSq / lenSq < epsilon * epsilon;
}

inline bool is_point_in_plane(const glm::vec3& n, const float c, 
    const glm::vec3& p, float epsilon = 1e-6) {

    float len = glm::length(n);
    if (len < epsilon) {
        return false;
    }
    
    float dist = std::abs(glm::dot(n, p) - c) / len;
    return dist < epsilon;
}

// n is normalized (||n|| = 1)
inline bool is_point_in_plane_normalized(const glm::vec3& n, const float c,
    const glm::vec3& p, float epsilon = 1e-6) {
    // 无需除以 length(n), 直接用 |dot(n,p)-c| 即是有符号距离的绝对值
    return std::abs(glm::dot(n, p) - c) < epsilon;
}

inline std::optional<glm::vec3> intersection_ray_and_triangle(const glm::vec3& o, const glm::vec3& d, 
    const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {

    // Möller–Trumbore 算法
    const float epsilon = 1e-6f;

    glm::vec3 e1 = b - a;
    glm::vec3 e2 = c - a;
    glm::vec3 pvec = glm::cross(d, e2);
    float det = glm::dot(e1, pvec);

    // 射线与三角形平面平行
    if (std::abs(det) < epsilon) {
        return std::nullopt;
    }

    float invDet = 1.0f / det;
    glm::vec3 tvec = o - a;
    float u = glm::dot(tvec, pvec) * invDet;
    if (u < 0.0f || u > 1.0f) {
        return std::nullopt;
    }

    glm::vec3 qvec = glm::cross(tvec, e1);
    float v = glm::dot(d, qvec) * invDet;
    if (v < 0.0f || u + v > 1.0f) {
        return std::nullopt;
    }

    float t = glm::dot(e2, qvec) * invDet;
    if (t < 0.0f) {
        return std::nullopt;
    }

    return o + t * d;
}

// sphere: ||p - center|| = r
inline std::optional<glm::vec3> intersection_ray_and_sphere(const glm::vec3& o, const glm::vec3& d,
    const glm::vec3& center, const float r) {

    const float epsilon = 1e-6f;

    glm::vec3 oc = o - center;
    float A = glm::dot(d, d);
    float B = 2.0f * glm::dot(oc, d);
    float C = glm::dot(oc, oc) - r * r;

    float discriminant = B * B - 4.0f * A * C;
    if (discriminant < 0.0f) {
        return std::nullopt;
    }

    float sqrtD = std::sqrt(discriminant);
    // 先尝试较小的 t（最近的交点）
    float t = (-B - sqrtD) / (2.0f * A);
    if (t < 0.0f) {
        // 射线起点在球内，尝试较大的 t（射出点）
        t = (-B + sqrtD) / (2.0f * A);
        if (t < 0.0f) {
            return std::nullopt;
        }
    }

    return o + t * d;
}

// AABB box: [boxMin, boxMax]，Slab 方法
inline std::optional<glm::vec3> intersection_ray_and_box(const glm::vec3& o, const glm::vec3& d,
    const glm::vec3& boxMin, const glm::vec3& boxMax) {

    const float epsilon = 1e-8f;

    float tEnter = -std::numeric_limits<float>::infinity();
    float tExit  =  std::numeric_limits<float>::infinity();

    for (int i = 0; i < 3; ++i) {
        if (std::abs(d[i]) < epsilon) {
            // 射线在该轴方向平行，若起点不在 slab 内则无交点
            if (o[i] < boxMin[i] || o[i] > boxMax[i]) {
                return std::nullopt;
            }
        } else {
            float invD = 1.0f / d[i];
            float t1 = (boxMin[i] - o[i]) * invD;
            float t2 = (boxMax[i] - o[i]) * invD;
            if (t1 > t2) std::swap(t1, t2);

            tEnter = std::max(tEnter, t1);
            tExit  = std::min(tExit,  t2);

            if (tEnter > tExit || tExit < 0.0f) {
                return std::nullopt;
            }
        }
    }

    float t = tEnter >= 0.0f ? tEnter : tExit;
    return o + t * d;
}

