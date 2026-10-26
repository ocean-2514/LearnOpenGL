#pragma once

#include "spatial/Geometry2D.h"
#include <algorithm>
#include <random>

class Generator2D
{
public:
    static Object2D GenerateObject2D(ObjectId id, const AABB2 &world,
                                     float minSize = 18.0f, float maxSize = 55.0f,
                                     float minSpeed = 25.0f, float maxSpeed = 90.0f)
    {
        const glm::vec2 worldSize = world.size();
        const float safeMaxSize = std::max(minSize, std::min(maxSize, std::min(worldSize.x, worldSize.y)));
        const glm::vec2 objectSize(random(minSize, safeMaxSize), random(minSize, safeMaxSize));
        const glm::vec2 position(
            random(world.min.x, world.max.x - objectSize.x),
            random(world.min.y, world.max.y - objectSize.y));

        Object2D object;
        object.id = id;
        object.bounds = {position, position + objectSize};
        object.velocity = glm::vec2(randomSigned(minSpeed, maxSpeed),
                                    randomSigned(minSpeed, maxSpeed));
        object.color = glm::vec3(random(0.25f, 0.85f),
                                 random(0.35f, 0.90f),
                                 random(0.45f, 1.00f));
        return object;
    }

private:
    static std::mt19937 &engine()
    {
        static std::mt19937 value(std::random_device{}());
        return value;
    }

    static float random(float minValue, float maxValue)
    {
        std::uniform_real_distribution<float> distribution(minValue, maxValue);
        return distribution(engine());
    }

    static float randomSigned(float minMagnitude, float maxMagnitude)
    {
        const float magnitude = random(minMagnitude, maxMagnitude);
        return random(0.0f, 1.0f) < 0.5f ? -magnitude : magnitude;
    }
};
