#pragma once

#include "ray_tracing/Material/Material.hpp"
#include "ray_tracing/Common.h"
#include <glm/glm.hpp>

class DielectricSpecularMaterial : public Material
{
public:
    DielectricSpecularMaterial(float n = 1.3f, 
        const glm::vec3& transmissionColor = glm::vec3{1.0f})
        : m_n(n), m_transmissionColor(transmissionColor) {}

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

        float ni, nt;
        if (wi.z > 0.0f) {
            ni = 1.0f;
            nt = m_n;
        } else {
            ni = m_n;
            nt = 1.0f;
        }

        glm::vec3 wt;
        float Fr = 1.0f;
        if (RefractLocal(wi, ni, nt, wt)) {
            Fr = Fresnel(ni, nt, wi.z, wt.z);
        }

        return glm::vec3{Fr} / std::max(fabs(wi.z), 1e-6f);
    }

    glm::vec3 BTDF(const glm::vec3& wo, const glm::vec3& wi) const override {
        // ni = nt
        float no, nt;
        if (wo.z > 0.0f) {
            no = 1.0f;
            nt = m_n;
        } else {
            no = m_n;
            nt = 1.0f;
        }

        glm::vec3 wt;
        if (!RefractLocal(wo, no, nt, wt)) {
            return glm::vec3{0.0f};
        }
        if (std::fabs(wt.x - wi.x) > 1e-4f
            || std::fabs(wt.y - wi.y) > 1e-4f
            || std::fabs(wt.z - wi.z) > 1e-4f) {
            return glm::vec3{0.0f};
        }

        float Fr = Fresnel(no, nt, wo.z, wt.z);
        return (1.0f - Fr) * no * no / (nt * nt * std::max(1e-6f, fabs(wi.z))) 
            * m_transmissionColor; 
    }

    float Fresnel(float ni, float nt, float cosTheta_i, float cosTheta_t) const {
        cosTheta_i = glm::clamp(std::fabs(cosTheta_i), 0.0f, 1.0f);
        cosTheta_t = glm::clamp(std::fabs(cosTheta_t), 0.0f, 1.0f);

        float a1 = nt * cosTheta_i, b1 = ni * cosTheta_t;
        float r1 = (a1 - b1) / std::max(a1 + b1, 1e-6f);

        float a2 = ni * cosTheta_i, b2 = nt * cosTheta_t;
        float r2 = (a2 - b2) / std::max(a2 + b2, 1e-6f);
        return glm::clamp(0.5f * (r1 * r1 + r2 * r2), 0.0f, 1.0f);
    }

    bool IsSpecular() const override {
        return true;
    }
    
    bool IsRefractable() const override {
        return true;
    }

    bool SampleDelta(const glm::vec3& wo,
        DeltaSample& sample) const override {
        const float ni = wo.z > 0.0f ? 1.0f : m_n;
        const float nt = wo.z > 0.0f ? m_n : 1.0f;

        glm::vec3 transmitted;
        if (!RefractLocal(wo, ni, nt, transmitted)) {
            sample.direction = glm::vec3{-wo.x, -wo.y, wo.z};
            sample.event = DeltaEvent::Reflection;
            sample.pdf = 1.0f;
            sample.weight = glm::vec3{1.0f};
            return true;
        }

        const float fresnel = Fresnel(ni, nt, wo.z, transmitted.z);
        if (Random01() < fresnel) {
            sample.direction = glm::vec3{-wo.x, -wo.y, wo.z};
            sample.event = DeltaEvent::Reflection;
            sample.pdf = fresnel;
            // The Fresnel coefficient cancels the event-selection pdf.
            sample.weight = glm::vec3{1.0f};
        } else {
            sample.direction = transmitted;
            sample.event = DeltaEvent::Transmission;
            sample.pdf = 1.0f - fresnel;
            // Preserve the radiance-transport eta scaling used by BTDF().
            sample.weight = m_transmissionColor * (ni * ni) / (nt * nt);
        }
        return sample.pdf > 0.0f;
    }

    bool SampleWi(const glm::vec3& wo, glm::vec3& wi, float& pdf) const override {
        pdf = 1.0f;
        wi = glm::vec3{
            -wo.x, -wo.y, wo.z
        };
        return true;
    }

    bool SampleWt(const glm::vec3& wo, glm::vec3& wt, float& pdf) const override {
        float no, nt;
        if (wo.z > 0.0f) {
            no = 1.0f;
            nt = m_n;
        } else {
            no = m_n;
            nt = 1.0f;
        }
        pdf = 1.0f;

        return RefractLocal(wo, no, nt, wt);
    }

private:
    float m_n;
    glm::vec3 m_transmissionColor;
};
