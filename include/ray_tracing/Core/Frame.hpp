#pragma once

#include <glm/glm.hpp>

class Frame {
public:
    explicit Frame(glm::vec3 normal) {
        normal = glm::normalize(normal);
        glm::vec3 tangent = glm::cross(normal, glm::vec3{1.0f, 0.0f, 0.0f});
        if (glm::dot(tangent, tangent) < 1e-8f) {
            tangent = glm::cross(normal, glm::vec3{0.0f, 1.0f, 0.0f});
        }
        tangent = glm::normalize(tangent);
        const glm::vec3 bitangent = glm::cross(tangent, normal);

        m_localToWorld = glm::mat3{bitangent, tangent, normal};
        m_worldToLocal = glm::transpose(m_localToWorld);
    }

    glm::vec3 ToWorld(const glm::vec3& local) const {
        return m_localToWorld * local;
    }

    glm::vec3 ToLocal(const glm::vec3& world) const {
        return m_worldToLocal * world;
    }

private:
    glm::mat3 m_localToWorld{1.0f};
    glm::mat3 m_worldToLocal{1.0f};
};
