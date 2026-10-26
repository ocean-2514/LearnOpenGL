#pragma once

#include "ray_tracing/Core/Hittable.hpp"
#include "ray_tracing/Material/Material.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <memory>
#include <vector>

class SceneObject : public Hittable
{
public:
    SceneObject() = default;

    explicit SceneObject(const glm::mat4& objectToWorld)
        : m_objectToWorld(objectToWorld)
        , m_worldToObject(glm::inverse(objectToWorld))
        , m_normalMatrix(glm::mat3(glm::transpose(m_worldToObject))) {
    }

    bool hit(const Ray& ray, float tMin, float tMax,
        HitRecord& record) const override {
        Ray objectRay = worldToObjectRay(ray);
        float max = tMax;
        bool hit = false;
        HitRecord localRecord{};

        for (const auto& obj : m_objects) {
            if (obj->hit(objectRay, tMin, max, localRecord)) {
                hit = true;
                max = localRecord.t;
                record = localRecord;
            }
        }

        if (!hit) {
            return false;
        }

        record.point = objectToWorldPoint(record.point);
        record.material = m_material.get();
        // Child primitives store a face-forward normal. Recover their geometric
        // outward normal before transforming it and determining frontFace again
        // in world space.
        glm::vec3 localOutwardNormal = record.frontFace
            ? record.normal
            : -record.normal;
        glm::vec3 worldOutwardNormal = objectToWorldNormal(localOutwardNormal);
        record.setFaceNormal(ray, worldOutwardNormal);
        return true;
    }

    void setTransform(const glm::mat4& objectToWorld) {
        m_objectToWorld = objectToWorld;
        m_worldToObject = glm::inverse(objectToWorld);
        m_normalMatrix = glm::mat3(glm::transpose(m_worldToObject));
    }

    void setTransform(const glm::mat4& objectToWorld,
        const glm::mat4& worldToObject) {
        m_objectToWorld = objectToWorld;
        m_worldToObject = worldToObject;
        m_normalMatrix = glm::mat3(glm::transpose(m_worldToObject));
    }

    void setObjectToWorld(const glm::mat4& objectToWorld) {
        setTransform(objectToWorld);
    }

    void setWorldToObject(const glm::mat4& worldToObject) {
        m_worldToObject = worldToObject;
        m_objectToWorld = glm::inverse(worldToObject);
        m_normalMatrix = glm::mat3(glm::transpose(m_worldToObject));
    }

    const glm::mat4& objectToWorld() const {
        return m_objectToWorld;
    }

    const glm::mat4& worldToObject() const {
        return m_worldToObject;
    }

    void setMaterial(const std::shared_ptr<Material>& material) {
        m_material = material;
    }

    template<typename T, typename... Args,
        std::enable_if_t<std::is_base_of_v<Hittable, T>, int> = 0>
    T* AddHittable(Args&&... args) {
        auto obj = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = obj.get();
        m_objects.push_back(std::move(obj));
        return ptr;
    }
    
    template<typename T, typename... Args,
        std::enable_if_t<std::is_base_of_v<Material, T>, int> = 0>
    T* AddMaterial(Args&&... args) {
        auto mat = std::make_shared<T>(std::forward<Args>(args)...);
        T* ptr = mat.get();
        m_material = std::move(mat);
        return ptr;
    }

    // simply sample the first object's surface
    bool SampleSurface(SurfaceSample& sample) const override {
        if (m_objects.empty()) return false;
        if (!m_objects[0]->SampleSurface(sample)) return false;

        const glm::vec3 transformedNormal = m_normalMatrix * sample.normal;
        const float normalLength = glm::length(transformedNormal);
        const float areaScale = std::abs(glm::determinant(glm::mat3(m_objectToWorld)))
            * normalLength;
        if (areaScale <= 0.0f) return false;

        sample.position = glm::vec3(
            m_objectToWorld * glm::vec4(sample.position, 1.0f));
        sample.normal = transformedNormal / normalLength;
        sample.pdfArea /= areaScale;
        return true;
    }

private:
    Ray worldToObjectRay(const Ray& ray) const {
        Ray objectRay{};
        objectRay.origin = glm::vec3(m_worldToObject * glm::vec4(ray.origin, 1.0f));

        // Keep the transformed direction unnormalized.  Then
        // objectRay(t) = worldToObject * ray(t), so t remains a world-ray
        // parameter and can be compared with hits from other scene objects.
        objectRay.direction = glm::vec3(
            m_worldToObject * glm::vec4(ray.direction, 0.0f));
        return objectRay;
    }

    glm::vec3 objectToWorldPoint(const glm::vec3& point) const {
        return glm::vec3(m_objectToWorld * glm::vec4(point, 1.0f));
    }

    glm::vec3 objectToWorldNormal(const glm::vec3& normal) const {
        glm::vec3 worldNormal = m_normalMatrix * normal;
        return glm::normalize(worldNormal);
    }
    
    std::vector<std::unique_ptr<Hittable>> m_objects;
    glm::mat4 m_objectToWorld{1.0f};
    glm::mat4 m_worldToObject{1.0f};
    glm::mat3 m_normalMatrix{1.0f};
    std::shared_ptr<Material> m_material;
};
