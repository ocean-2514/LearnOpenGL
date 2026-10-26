#pragma once

#include "ray_tracing/Material/Material.hpp"
#include "ray_tracing/Core/Hittable.hpp"
#include "ray_tracing/Core/Ray.hpp"
#include "ray_tracing/Common.h"

class LambertMaterial : public Material {
public:
    LambertMaterial(const glm::vec3& albedo = {1.0f, 1.0f, 1.0f})
        : m_albedo(albedo) {}

    bool scatter(const Ray& incoming, const HitRecord& hit,
        glm::vec3& attenuation, Ray& scattered) const override {

        return true;
    }
    
    glm::vec3 BRDF(const glm::vec3& wo, const glm::vec3& wi) const override {
        return m_albedo * INV_PI;
    }

    bool SampleWi(const glm::vec3& wo, glm::vec3& wi, float& pdf) const override {
        float e1 = Random01();
        float e2 = Random01();

        float sinTheta = std::sqrt(e1);
        float cosTheta = std::sqrt(1 - e1);
        float phi = 2 * PI * e2;

        pdf = cosTheta * INV_PI;
        
        wi = glm::vec3{
            sinTheta * std::cos(phi), 
            sinTheta * std::sin(phi), 
            cosTheta
        };

        return true;
    }

    float Pdf(const glm::vec3& wo, const glm::vec3& wi) const override {
        return std::max(0.0f, wi.z) * INV_PI;
    }

private:    
    glm::vec3 m_albedo;
};
