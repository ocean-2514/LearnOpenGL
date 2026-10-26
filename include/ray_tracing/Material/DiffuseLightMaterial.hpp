#pragma once

#include "ray_tracing/Material/Material.hpp"
#include <glm/glm.hpp>

class DiffuseLightMaterial : public Material {
public: 
    DiffuseLightMaterial(const glm::vec3& color = glm::vec3{1.0f},
        bool twoSided = false)
        : m_radiance(color), m_twoSided(twoSided) {}
    DiffuseLightMaterial(const DiffuseLightMaterial& other)
        : m_radiance(other.m_radiance), m_twoSided(other.m_twoSided) {}

    bool IsEmissive() const override { return true; }

    glm::vec3 Emission(const glm::vec3& wo) const override {
        if (m_twoSided) return m_radiance;
        if (wo.z > 0) return m_radiance;
        else return glm::vec3{0.0f};
    }


private:
    glm::vec3 m_radiance;
    bool m_twoSided = false;
};