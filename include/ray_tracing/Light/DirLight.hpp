#pragma once

#include "ray_tracing/Light/Light.hpp"

class DirLight : public Light {
public:
    bool IsDelta() const override { return true; }

    DirLight(const glm::vec3& color = {1.0f, 1.0f, 1.0f},
             const glm::vec3& dir = {0.0f, -1.0f, 0.0f})
        : Light(color), m_direction(glm::normalize(dir)) {}

    bool SampleLi(const glm::vec3& p,
        LightSample& sample) const override {
        sample.position = p - m_direction * 10000.0f;
        sample.radiance = m_radiance;
        sample.distance = 10000.0f;
        sample.normal = m_direction;
        sample.pdf = 1.0f;
        sample.wi = -m_direction;

        return true;
    } 

private:
    glm::vec3 m_direction;
};
