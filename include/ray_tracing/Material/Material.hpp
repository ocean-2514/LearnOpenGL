#pragma once

#include <glm/glm.hpp>
#include <ray_tracing/Common.h>

struct Ray;
struct HitRecord;

enum class DeltaEvent {
    Reflection,
    Transmission
};

struct DeltaSample {
    glm::vec3 direction{0.0f};
    glm::vec3 weight{0.0f};
    float pdf = 0.0f; // Discrete probability of selecting this event.
    DeltaEvent event = DeltaEvent::Reflection;
};

class Material {
public:
    virtual ~Material() = default;
    
    // not used currently
    virtual bool scatter(const Ray& incoming, const HitRecord& hit,
        glm::vec3& attenuation, Ray& scattered) const {
        return false;
    }
    
    // wo,wi should be normalized
    virtual glm::vec3 BRDF(const glm::vec3& wo, const glm::vec3& wi) const {
        return glm::vec3{0.0f};
    }
    // wo,wi should be normalized
    virtual glm::vec3 BTDF(const glm::vec3& wo, const glm::vec3& wi) const {
        return glm::vec3{0.0f};
    }
    
    virtual glm::vec3 Emission(const glm::vec3& wo) const {
        return glm::vec3{0.0f};
    }

    virtual bool IsSpecular() const {
        return false;
    }
    
    virtual bool IsRefractable() const {
        return false;
    }

    virtual bool IsEmissive() const {
        return false;
    }

    // Samples one discrete reflection/transmission event.  weight is the
    // complete throughput multiplier, including division by the event pdf.
    virtual bool SampleDelta(const glm::vec3& wo,
        DeltaSample& sample) const {
        return false;
    }

    virtual bool SampleWt(const glm::vec3& wo, glm::vec3& wt, float& pdf) const {
        return false;
    }

    virtual bool SampleWi(const glm::vec3& wo, glm::vec3& wi, float& pdf) const {
		pdf = 0.0f;
        wi = glm::vec3{0.0f, 1.0f, 0.0f};
        return false;
    }

    virtual float Pdf(const glm::vec3& wo, const glm::vec3& wi) const {
        return 1.0f;
    }

};

