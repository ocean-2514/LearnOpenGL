#pragma once

#include "ray_tracing/Core/Hittable.hpp"
#include <glm/glm.hpp>

class Triangle : public Hittable
{
public:
    Triangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) 
        : m_a(a), m_b(b), m_c(c) {
        glm::vec3 e1 = m_b - m_a;
        glm::vec3 e2 = m_c - m_a;
        glm::vec3 p = glm::cross(e1, e2);
        const float doubleArea = glm::length(p);
        m_area = 0.5f * doubleArea;
        m_normal = doubleArea > 0.0f ? p / doubleArea : glm::vec3{0.0f};
    }
    
    bool hit(const Ray& ray, float tMin, float tMax,
        HitRecord& record) const override {
        const float epsilon = 1e-6f;

        glm::vec3 e1 = m_b - m_a;
        glm::vec3 e2 = m_c - m_a;
        glm::vec3 pvec = glm::cross(ray.direction, e2);
        float det = glm::dot(e1, pvec);

        if (std::abs(det) < epsilon) {
            return false;
        }

        float invDet = 1.0f / det;
        glm::vec3 tvec = ray.origin - m_a;
        float u = glm::dot(tvec, pvec) * invDet;
        if (u < 0.0f || u > 1.0f) {
            return false;
        }

        glm::vec3 qvec = glm::cross(tvec, e1);
        float v = glm::dot(ray.direction, qvec) * invDet;
        if (v < 0.0f || u + v > 1.0f) {
            return false;
        }

        float t = glm::dot(e2, qvec) * invDet;
        if (t < tMin || t > tMax) {
            return false;
        }

        record.t = t;
        record.point = ray.origin + t * ray.direction;
        record.setFaceNormal(ray, m_normal);

        return true;
    }

    bool SampleSurface(SurfaceSample& sample) const override {
        if (m_area <= 0.0f) {
            return false;
        }
        sample.position = UniformSampleTriangle(m_a, m_b, m_c);
        sample.normal = m_normal;

        sample.pdfArea = 1.0f / m_area;

        return true;
    }

private:
    glm::vec3 m_a, m_b, m_c;
    glm::vec3 m_normal;
};
