#pragma once

#include "ray_tracing/Scene/SceneObject.hpp"
#include "ray_tracing/Core/Ray.hpp"
#include "ray_tracing/Light/Light.hpp"
#include <vector>
#include <type_traits>
#include <glm/glm.hpp>

class Scene
{
public:
    SceneObject* hit(const Ray& ray, float tMin, float tMax,
        HitRecord& record) const {
        if (tMax < tMin) return nullptr;
        float max = tMax;
        bool hit = false;
        SceneObject* result = nullptr;
        for (auto& obj : m_objects) {
            if (obj->hit(ray, tMin, max, record)) {
                result = obj.get();
                hit = true;
                max = record.t;
            }
        }
        return result;
    }

    template<typename T, typename... Args,
        std::enable_if_t<std::is_base_of_v<SceneObject, T>, int> = 0>
    T* AddSceneObject(Args&&... args) {
        auto obj = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = obj.get();
        m_objects.push_back(std::move(obj));
        return ptr;
    }

    template<typename T, typename... Args,
        std::enable_if_t<!std::is_base_of_v<SceneObject, T>, int> = 0>
    SceneObject* AddHittable(Args&&... args) {
        auto obj = std::make_unique<SceneObject>();
        obj->AddHittable<T>(std::forward<Args>(args)...);
        SceneObject* ptr = obj.get();
        m_objects.push_back(std::move(obj));
        return ptr;
    }

    template<typename T, typename... Args,
        std::enable_if_t<std::is_base_of_v<Light, T>, int> = 0>
    T* AddLight(Args&&... args) {
        auto light = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = light.get();
        m_lights.push_back(std::move(light));
        return ptr;
    }

    const std::vector<std::unique_ptr<Light>>& GetLights() const {
        return m_lights;
    }

    const Light* GetRandomLight() const {
        if (m_lights.empty()) {
            return nullptr;
        }
        return m_lights[RandomInt(0, m_lights.size() - 1)].get();
    }


private:
    std::vector<std::unique_ptr<SceneObject>> m_objects;
    std::vector<std::unique_ptr<Light>> m_lights;
};
