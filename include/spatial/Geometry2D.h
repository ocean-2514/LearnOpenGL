#pragma once

#include <glm/glm.hpp>
#include <cstdint>

using ObjectId = std::uint32_t;

struct AABB2
{
    glm::vec2 min{0.0f};
    glm::vec2 max{0.0f};

    glm::vec2 center() const { return (min + max) * 0.5f; }
    glm::vec2 size() const { return max - min; }

    float area() const {
        return (max.x - min.x) * (max.y - min.y);
    }

    bool contains(const glm::vec2 &point) const
    {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y;
    }

    bool contains(const AABB2& other) const {
        return min.x <= other.min.x && max.x >= other.max.x &&
               min.y <= other.min.y && max.y >= other.max.y;
    }

    bool intersects(const AABB2 &other) const
    {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y;
    }

    static AABB2 merge(const AABB2& a, const AABB2& b) {
        return {
            {std::min(a.min.x, b.min.x), std::min(a.min.y, b.min.y)}, 
            {std::max(a.max.x, b.max.x), std::max(a.max.y, b.max.y)}
        };
    }
};

struct Object2D
{
    ObjectId id = 0;
    AABB2 bounds;
    glm::vec2 velocity{0.0f};
    glm::vec3 color{0.35f, 0.65f, 0.95f};

    static void checkAndResolveCollision(Object2D& a, Object2D& b) {
        if (!a.bounds.intersects(b.bounds)) return;
        glm::vec2 overlap{
            std::min(a.bounds.max.x - b.bounds.min.x, b.bounds.max.x - a.bounds.min.x),
            std::min(a.bounds.max.y - b.bounds.min.y, b.bounds.max.y - a.bounds.min.y)
        };
        // Touching edges are not penetration.
        if (overlap.x <= 0.0f || overlap.y <= 0.0f) return;

        int axis = overlap.x < overlap.y ? 0 : 1;
        const float sign = (a.bounds.center()[axis] < b.bounds.center()[axis]) ? -1.0f : 1.0f;
        const float separation = 0.5f * overlap[axis] + 0.01f;
        a.bounds.max[axis] += sign * separation;
        a.bounds.min[axis] += sign * separation;
        b.bounds.max[axis] -= sign * separation;
        b.bounds.min[axis] -= sign * separation;

        // Equal-mass elastic response along the collision normal. If the
        // objects are already separating, only correct their penetration.
        const float normalFromAToB = -sign;
        const float closingSpeed =
            (a.velocity[axis] - b.velocity[axis]) * normalFromAToB;
        if (closingSpeed > 0.0f)
            std::swap(a.velocity[axis], b.velocity[axis]);
    }

    AABB2 fatBounds() const {
        glm::vec2 margin = bounds.size() * 0.5f;
        return AABB2{
            bounds.min - margin, bounds.max + margin
        };
    }
};

struct Point2D {
    ObjectId id = 0;
    glm::vec2 pos{0.0f};
};
