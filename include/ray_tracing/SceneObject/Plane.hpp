#pragma once

#include "ray_tracing/Scene/SceneObject.hpp"
#include "ray_tracing/Primitive/Triangle.hpp"

class Plane : public SceneObject
{
public:
    Plane(const glm::vec3& center = glm::vec3(0.0f),
          float halfWidth = 0.5f,
          float halfHeight = 0.5f)
    {
        const glm::vec3 p0 = center + glm::vec3(-halfWidth, 0.0f, -halfHeight);
        const glm::vec3 p1 = center + glm::vec3( halfWidth, 0.0f, -halfHeight);
        const glm::vec3 p2 = center + glm::vec3( halfWidth, 0.0f,  halfHeight);
        const glm::vec3 p3 = center + glm::vec3(-halfWidth, 0.0f,  halfHeight);

        m_t1 = AddHittable<Triangle>(p0, p1, p2);
        m_t2 = AddHittable<Triangle>(p0, p2, p3);

        m_triangleArea1 = m_t1->Area();
        m_triangleArea2 = m_t2->Area();
        m_area = m_triangleArea1 + m_triangleArea2;
    }

    Plane(const glm::vec3& a,
          const glm::vec3& b,
          const glm::vec3& c,
          const glm::vec3& d)
    {
        m_t1 = AddHittable<Triangle>(a, b, c);
        m_t2 = AddHittable<Triangle>(a, c, d);

        m_triangleArea1 = m_t1->Area();
        m_triangleArea2 = m_t2->Area();
        m_area = m_triangleArea1 + m_triangleArea2;
    }

    bool SampleSurface(SurfaceSample& sample) const override {
        if (m_area <= 0.0f) {
            return false;
        }
        Triangle* triangleToSample = 
            Random(0.0f, m_area) < m_triangleArea1 ? m_t1 : m_t2;

        if (!triangleToSample->SampleSurface(sample)) {
            return false;
        }
        sample.pdfArea = 1.0f / m_area;

        return true;
    }

private:
    Triangle* m_t1;
    Triangle* m_t2;
    float m_triangleArea1, m_triangleArea2;

};
