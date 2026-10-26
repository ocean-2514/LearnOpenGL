#pragma once

#include "ray_tracing/Sampler/Sampling.hpp"
#include <cmath>
#include <glm/glm.hpp>

inline glm::vec3 Reflect(const glm::vec3& r, const glm::vec3& n) {
    glm::vec3 rt = glm::dot(r, n) * n;
    glm::vec3 rn = r - rt;
    return rt - rn;
}

// wi, wo should be normalized
inline bool RefractLocal(const glm::vec3& wi, float ni, float nt, glm::vec3& wt) {
    float f = ni / nt;
    float cosTheta_i = glm::clamp(std::fabs(wi.z), 0.0f, 1.0f);
    float sinTheta_i_sq = 1.0f - cosTheta_i * cosTheta_i;
    float sinTheta_t_sq = f * f * sinTheta_i_sq;
    if (sinTheta_t_sq > 1.0f) return false;

    float cosTheta_t = std::sqrt(std::max(0.0f, 1.0f - sinTheta_t_sq));

    // Transmission must leave through the hemisphere opposite to wi.
    wt.z = wi.z >= 0.0f ? -cosTheta_t : cosTheta_t;
    wt.x = -wi.x * f;
    wt.y = -wi.y * f;

    return true;
}

