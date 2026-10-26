#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "ray_tracing/Core/Ray.hpp"
#include "ray_tracing/Common.h"

class Camera
{
public:
    enum Direction {
        Up, Down, Forward, Backward, Left, Right
    };

    Camera(float width, float height,
        const glm::vec3& position = glm::vec3{0.0f, 0.0f, 3.0f}, float fov = 45.0f) 
        : m_width(width), m_height(height), m_position(position), m_fov(fov) {
        UpdateCachedMatrix();
    }

    glm::mat4 GetViewMatrix() const {
        return glm::lookAt(m_position, m_position + m_front, m_worldUp);
    }

    glm::mat4 GetProjection() const {
        return glm::perspective(glm::radians(m_fov), m_width / m_height, 0.01f, 100.0f);
    }

    float GetZoom() const {
        return m_fov;
    }

    void ProcessMouseMovement(float xoffset, float yoffset) {
        m_yaw -= xoffset * m_sensitivity;
        m_pitch += yoffset * m_sensitivity;
        m_pitch = glm::clamp(m_pitch, -89.0f, 89.0f);

        // yaw 折叠到 [-180, 180]，避免长时间运行后角度过大导致浮点精度抖动
        m_yaw = glm::mod(m_yaw, 360.0f);
        if (m_yaw > 180.0f)  m_yaw -= 360.0f;
        if (m_yaw < -180.0f) m_yaw += 360.0f;

        Update();
    }

    void ProcessKeyboard(Direction direction, float deltaTime) {
        glm::vec3 horizontalFront = glm::cross(m_worldUp, m_right);
        float velocity = deltaTime * m_moveSpeed;
        if (direction == Forward) {
            m_position += horizontalFront * velocity;
        } else if (direction == Backward) {
            m_position -= horizontalFront * velocity;
        } else if (direction == Up) {
            m_position += m_worldUp * velocity;
        } else if (direction == Down) {
            m_position -= m_worldUp * velocity;
        } else if (direction == Left) {
            m_position -= m_right * velocity;
        } else if (direction == Right) {
            m_position += m_right * velocity;
        }
        UpdateCachedMatrix();
    }

    Ray GetRay(float screenX, float screenY) const {
        // Trace through the pixel center instead of its top-left corner.
        float ndcX = (2.0f * (screenX + 0.5f)) / m_width - 1.0f;
        float ndcY = 1.0f - (2.0f * (screenY + 0.5f)) / m_height;

        return MakeRay(ndcX, ndcY);
    }
    
    Ray GetRandomRay(float screenX, float screenY) const {
        // Trace through the pixel center instead of its top-left corner.
        float ndcX = (2.0f * (screenX + Random01())) / m_width - 1.0f;
        float ndcY = 1.0f - (2.0f * (screenY + Random01())) / m_height;

        return MakeRay(ndcX, ndcY);
    }

    bool IsSameState(const Camera& other) {
        return m_position == other.m_position
            && m_yaw == other.m_yaw
            && m_pitch == other.m_pitch
            && m_fov == other.m_fov
            && m_width == other.m_width
            && m_height == other.m_height;
    }

private:
    static constexpr glm::vec3 m_worldUp{0.0f, 1.0f, 0.0f}; 
    glm::vec3 m_position;
    glm::vec3 m_front{0.0f, 0.0f, -1.0f};
    glm::vec3 m_right{1.0f, 0.0f, 0.0f};
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
    float m_fov = 45.0f;
    float m_moveSpeed = 1.5f;
    float m_sensitivity = 0.1f;
    float m_width;
    float m_height;
    glm::mat4 m_inverseViewProjection{1.0f};

    Ray MakeRay(float ndcX, float ndcY) const {
        glm::vec4 nearPoint = m_inverseViewProjection
            * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
        glm::vec4 farPoint = m_inverseViewProjection
            * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);

        nearPoint /= nearPoint.w;
        farPoint /= farPoint.w;

        glm::vec3 direction = glm::normalize(glm::vec3(farPoint - nearPoint));
        return Ray{m_position, direction};
    }

    void UpdateCachedMatrix() {
        m_inverseViewProjection = glm::inverse(GetProjection() * GetViewMatrix());
    }

    void Update() {
        glm::quat rotYaw = glm::angleAxis(glm::radians(m_yaw), m_worldUp);
        glm::quat rotPitch = glm::angleAxis(glm::radians(m_pitch), glm::vec3{1.0f, 0.0f, 0.0f});
        m_front = rotYaw * rotPitch * glm::vec3{0.0f, 0.0f, -1.0f};
        m_right = rotYaw * glm::vec3{1.0f, 0.0f, 0.0f};
        UpdateCachedMatrix();
    }
};
