#pragma once

#include "ray_tracing/Material/Material.hpp"
#include <glm/glm.hpp>

class ConductorSpecularMaterial : public Material
{
public:
    ConductorSpecularMaterial(const glm::vec3& n = glm::vec3{0.5f}, 
        const glm::vec3& absorbtionCoeff = glm::vec3{2.0f}, 
        const glm::vec3& reflectionColor = glm::vec3{1.0f})
        : m_n(n), m_absorbtionCoef(absorbtionCoeff), 
        m_reflectoinColor(reflectionColor) {}

    bool scatter(const Ray& incoming, const HitRecord& hit,
        glm::vec3& attenuation, Ray& scattered) const override {
        return true;
    }
    
    glm::vec3 BRDF(const glm::vec3& wo, const glm::vec3& wi) const override {
        if (std::fabs(wo.x + wi.x) > 1e-4f
            || std::fabs(wo.y + wi.y) > 1e-4f
            || std::fabs(wo.z - wi.z) > 1e-4f) {
            return glm::vec3{0.0f};
        }

        glm::vec3 Fr = computeFresnel(wo.z);
        return Fr * m_reflectoinColor / std::max(std::fabs(wo.z), 1e-6f);
    }

    glm::vec3 BTDF(const glm::vec3& wo, const glm::vec3& wi) const override {
        return glm::vec3{0.0f};
    }

    glm::vec3 computeFresnel(float cosTheta) const {
        cosTheta = glm::clamp(std::fabs(cosTheta), 0.0f, 1.0f);
        glm::vec3 a = m_n * m_n + m_absorbtionCoef * m_absorbtionCoef;
        glm::vec3 b = 2.0f * m_n * cosTheta;
        glm::vec3 numer1 = a * cosTheta * cosTheta - b + 1.0f;
        glm::vec3 denom1 = a * cosTheta * cosTheta + b + 1.0f;
        glm::vec3 numer2 = a - b + cosTheta * cosTheta;
        glm::vec3 denom2 = a + b + cosTheta * cosTheta;
        denom2.x = std::max(denom2.x, 1e-6f);
        denom2.y = std::max(denom2.y, 1e-6f);
        denom2.z = std::max(denom2.z, 1e-6f);

        glm::vec3 rt = numer1 / denom1;
        glm::vec3 rn = numer2 / denom2;

        return 0.5f * (rt + rn);
    }

    bool IsSpecular() const override {
        return true;
    }

    bool SampleDelta(const glm::vec3& wo,
        DeltaSample& sample) const override {
        sample.direction = glm::vec3{-wo.x, -wo.y, wo.z};
        sample.event = DeltaEvent::Reflection;
        sample.pdf = 1.0f;
        sample.weight = computeFresnel(wo.z) * m_reflectoinColor;
        return true;
    }

    bool SampleWi(const glm::vec3& wo, glm::vec3& wi, float& pdf) const override {
        pdf = 1.0f;
        wi = glm::vec3{
            -wo.x, -wo.y, wo.z
        };
        return true;
    }


private:
    glm::vec3 m_n;
    glm::vec3 m_absorbtionCoef;
    glm::vec3 m_reflectoinColor;
};
