#pragma once

#include "ray_tracing/Light/Light.hpp"

class SpotLight : public Light {
public:
    bool IsDelta() const override { return true; }

    // attenuation{a, b, c} : a * x^2 + b * x + c
    SpotLight(const glm::vec3& color = {1.0f, 1.0f, 1.0f},
              const glm::vec3& pos = {0.0f, 0.0f, 3.0f},
              const glm::vec3& direction = {0.0f, 0.0f, -1.0f},
              const glm::vec3& attenuation = {0.032f, 0.09f, 1.0f},
              float innerCone = 30.0f, float outerCone = 60.0f)
        : Light(color)
        , m_position(pos)
        , m_direction(glm::normalize(direction))
        , m_attenuations(attenuation)
        , m_cosInnerCone(std::cos(glm::radians(innerCone)))
        , m_cosOuterCone(std::cos(glm::radians(outerCone))) {
    }

    bool SampleLi(const glm::vec3& p,
        LightSample& sample) const override {
        glm::vec3 radiance;

        const glm::vec3 toPoint = p - m_position;
        const float distance = glm::length(toPoint);
        if (distance <= 0.0f) {
            return false;
        } else {
            const glm::vec3 lightDir = toPoint / distance;
            const float cosTheta = glm::dot(m_direction, lightDir);
            const float denom = m_attenuations[0] * distance * distance +
                                m_attenuations[1] * distance +
                                m_attenuations[2];
    
            if (cosTheta < m_cosOuterCone) {
                radiance = glm::vec3{0.0f};
            } else if (denom <= 0.0f) {
                radiance = m_radiance;
            } else if (cosTheta >= m_cosInnerCone) {
                radiance = m_radiance / denom;
            } else {
                const float falloff = (cosTheta - m_cosOuterCone) /
                                      (m_cosInnerCone - m_cosOuterCone);
                radiance = m_radiance * falloff / denom;
            }
        }

        sample.distance = distance;
        sample.normal = (p - m_position) / distance;
        sample.pdf = 1.0f;
        sample.position = m_position;
        sample.radiance = radiance;
        sample.wi = -sample.normal;

        return true;
    } 
    

private:
    glm::vec3 m_position;
    glm::vec3 m_direction;
    glm::vec3 m_attenuations;

    float m_cosInnerCone;
    float m_cosOuterCone;
};
