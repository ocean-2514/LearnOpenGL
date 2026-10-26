#pragma once

#include "ray_tracing/Core/Hittable.hpp"

class Sphere : public Hittable
{
public:
    Sphere(glm::vec3 center = glm::vec3{0.0f}, float r = 1.0f) 
        : m_center(center), m_radius(r) {
        m_area = 4 * PI * m_radius * m_radius;
    }

    bool hit(const Ray& ray, float tMin, float tMax,
        HitRecord& record) const override {
        const float epsilon = 1e-6f;
        const auto& d = ray.direction;
        const auto& o = ray.origin;

        glm::vec3 oc = o - m_center;
        float A = glm::dot(d, d);
        float B = 2.0f * glm::dot(oc, d);
        float C = glm::dot(oc, oc) - m_radius * m_radius;

        float discriminant = B * B - 4.0f * A * C;
        if (discriminant < 0.0f) {
            return false;
        }

        float sqrtD = std::sqrt(discriminant);
        float t = (-B - sqrtD) / (2.0f * A);
        if (t < tMin || t > tMax) {
            t = (-B + sqrtD) / (2.0f * A);
            if (t < tMin || t > tMax) {
                return false;
            }
        }

        record.point = o + t * d;
        glm::vec3 outWardNormal = (record.point - m_center) / m_radius;
        record.setFaceNormal(ray, outWardNormal);
        record.t = t;

        return true;
    }

    bool SampleSurface(SurfaceSample& sample) const override {
        if (m_area <= 0.0f) {
            return false;
        }
        glm::vec3 normal = UniformSampleSphere();
        sample.normal = normal;
        sample.position = m_center + normal * m_radius;
        sample.pdfArea = 1.0f / m_area;
        return true;
    }

private:
    glm::vec3 m_center{0.0f};
    float m_radius = 1.0f;
};
