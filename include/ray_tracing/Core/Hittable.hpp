#pragma once

#include "ray_tracing/Core/Ray.hpp"
#include "ray_tracing/Material/Material.hpp"
#include <ray_tracing/Sampler/Sampling.hpp>

struct HitRecord {
    glm::vec3 point;
    glm::vec3 normal;
    float t;
    bool frontFace;
    const Material* material;

    void setFaceNormal(const Ray& ray, const glm::vec3& outwardNormal) {
        frontFace = glm::dot(ray.direction, outwardNormal) < 0.0f;
        normal = frontFace ? outwardNormal : -outwardNormal;
    }
};

struct SurfaceSample {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f};
    float pdfArea = 0.0f;
};

class Hittable {
public:
    virtual ~Hittable() = default;
    virtual bool hit(const Ray& ray, float tMin, float tMax,
                     HitRecord& record) const = 0;
    
    virtual bool SampleSurface(SurfaceSample& sample) const {
        return false;
    }

    float Area() const {
        return m_area;
    }

protected:
    float m_area = 0.0f;
};
