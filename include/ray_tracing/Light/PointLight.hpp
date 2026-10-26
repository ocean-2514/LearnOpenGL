#pragma once

#include "ray_tracing/Light/Light.hpp"

class PointLight : public Light {
public:
    bool IsDelta() const override { return true; }

    // attenuation{a, b, c} : a * x^2 + b * x + c
    PointLight(const glm::vec3& color = {1.0f, 1.0f, 1.0f},
               const glm::vec3& pos = {0.0f, 2.0f, 0.0f},
               const glm::vec3& attenuation = {0.032f, 0.09f, 1.0f})
        : Light(color), m_position(pos), m_attenuations(attenuation) {}

    bool SampleLi(const glm::vec3& p,
        LightSample& sample) const override {
        
        glm::vec3 radiance;
        const float distance = glm::length(p - m_position);
        if (distance <= 0.0f) {
            return false;
        } else {
            const float denom = m_attenuations[0] * distance * distance +
                                m_attenuations[1] * distance +
                                m_attenuations[2];
            if (denom <= 0.0f) {
                radiance = m_radiance;
            } else {
                radiance = m_radiance / denom;
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
    glm::vec3 m_attenuations;
};
