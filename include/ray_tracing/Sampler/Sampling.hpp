#pragma once

#include <cmath>
#include <random>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

inline constexpr float PI = glm::pi<float>();
inline constexpr float INV_PI = 1.0f / PI;

inline float Random01() {
    thread_local std::mt19937 generator(std::random_device{}());
    thread_local std::uniform_real_distribution<float> distribution{0.0f, 1.0f};
    return distribution(generator);
}

inline int RandomInt(int min, int max) {
    thread_local std::mt19937 generator(std::random_device{}());
    thread_local std::uniform_int_distribution<int> distribution(min, max);
    return distribution(generator);
}

inline float Random(float minValue, float maxValue) {
    return Random01() * (maxValue - minValue) + minValue;
}

inline glm::vec3 GetSphereCoordinate(float theta, float phi) {
    return {
        std::sin(theta) * std::cos(phi),
        std::sin(theta) * std::sin(phi),
        std::cos(theta)
    };
}

inline glm::vec2 UniformSampleDisk(float radius) {
    const float radiusSample = radius * std::sqrt(Random01());
    const float theta = 2.0f * PI * Random01();
    return {radiusSample * std::cos(theta), radiusSample * std::sin(theta)};
}

inline glm::vec3 UniformSampleHemisphere() {
    const float cosTheta = Random01();
    const float sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);
    const float phi = 2.0f * PI * Random01();
    return {
        sinTheta * std::cos(phi),
        sinTheta * std::sin(phi),
        cosTheta
    };
}

inline glm::vec3 UniformSampleSphere() {
    const float cosTheta = 1.0f - 2 * Random01();
    const float sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);
    const float phi = 2 * PI * Random01();
    return {
        sinTheta * std::cos(phi),
        sinTheta * std::sin(phi),
        cosTheta
    };
}

inline glm::vec3 UniformSampleTriangle(const glm::vec3& p0,
    const glm::vec3& p1, const glm::vec3& p2) {
    const float sqrtSample = std::sqrt(Random01());
    const float u = 1.0f - sqrtSample;
    const float v = Random01() * sqrtSample;
    return (1.0f - u - v) * p0 + u * p1 + v * p2;
}
