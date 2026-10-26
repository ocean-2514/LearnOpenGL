#pragma once

#include "ray_tracing/Scene/SceneObject.hpp"
#include "ray_tracing/Material/DiffuseLightMaterial.hpp"
#include "ray_tracing/Light/Light.hpp"
#include "ray_tracing/Core/Frame.hpp"
#include <cmath>
#include <limits>

class AreaLight : public Light
{
public:
    AreaLight(const SceneObject* object, const DiffuseLightMaterial* material) 
        : m_object(object), m_material(material) {}

    virtual bool SampleLi(const glm::vec3& p, LightSample& sample) const override {
        if (!m_object || !m_material) {
            return false;
        }

        SurfaceSample surfaceSample{};
        if (!m_object->SampleSurface(surfaceSample)
            || surfaceSample.pdfArea <= 0.0f) {
            return false;
        }

        sample.normal = surfaceSample.normal;
        sample.position = surfaceSample.position;
        glm::vec3 toLight = sample.position - p;
        const float distanceSquared = glm::dot(toLight, toLight);
        if (distanceSquared <= 0.0f) {
            return false;
        }
        const float distance = std::sqrt(distanceSquared);
        sample.distance = distance;
        sample.wi = toLight / distance;

        const Frame lightFrame(sample.normal);
        sample.radiance = m_material->Emission(
            lightFrame.ToLocal(-sample.wi));
        if (glm::dot(sample.radiance, sample.radiance) <= 0.0f) {
            return false;
        }

        const float cosAtLight = std::abs(
            glm::dot(sample.normal, -sample.wi));
        if (cosAtLight <= 0.0f) {
            return false;
        }
        sample.pdf = surfaceSample.pdfArea * distanceSquared / cosAtLight;

        return std::isfinite(sample.pdf) && sample.pdf > 0.0f;
    }

    float PdfLi(const glm::vec3& p, const glm::vec3& wi) const override {
        if (!m_object || !m_material || m_object->Area() <= 0.0f) {
            return 0.0f;
        }

        const float wiLengthSquared = glm::dot(wi, wi);
        if (wiLengthSquared <= 0.0f) return 0.0f;
        const glm::vec3 direction = wi / std::sqrt(wiLengthSquared);

        HitRecord record{};
        if (!m_object->hit(Ray{p, direction}, 0.001f,
            std::numeric_limits<float>::max(), record)) {
            return 0.0f;
        }
        if (record.material != m_material) return 0.0f;

        const float distSq = glm::dot(p - record.point, p - record.point);
        const glm::vec3 outwardNormal = record.frontFace
            ? record.normal : -record.normal;
        const Frame lightFrame(outwardNormal);
        const glm::vec3 emitted = m_material->Emission(
            lightFrame.ToLocal(-direction));
        if (glm::dot(emitted, emitted) <= 0.0f) return 0.0f;

        const float cosTheta = std::abs(
            glm::dot(outwardNormal, -direction));

        if (cosTheta <= 0.0f) {
            return 0.0f;
        }

        const float pdfArea = 1.0f / m_object->Area();
        return pdfArea * distSq / cosTheta;
    }


private:
    const SceneObject* m_object;
    const DiffuseLightMaterial* m_material;
};
