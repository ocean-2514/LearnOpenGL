#pragma once

#include "ray_tracing/Core/Image.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <algorithm>

class Film {
public:
    Film(int width, int height)
        : m_image(width, height), m_width(width), m_height(height) {
        m_accumulatedRadiance.resize(width * height, glm::vec3{0.0f});
        m_sampleCount.resize(width * height, 0);
    }

    void AddSample(int x, int y, const glm::vec3& radiance, int samples)
    {
        const int index = y * m_width + x;

        m_accumulatedRadiance[index] += radiance;
        m_sampleCount[index] += samples;

        const glm::vec3 average =
            m_accumulatedRadiance[index] /
            static_cast<float>(m_sampleCount[index]);

        SetLinearPixel(x, y, average);
    }

    void SetLinearPixel(int x, int y, const glm::vec3& linearColor) {
        const glm::vec3 gammaCorrected = glm::sqrt(
            glm::max(linearColor, glm::vec3{0.0f}));
        m_image.SetPixel(x, y, gammaCorrected);
    }

    void Clear()
    {
        std::fill(m_accumulatedRadiance.begin(), 
            m_accumulatedRadiance.end(), glm::vec3{0.0f});

        std::fill(m_sampleCount.begin(),
            m_sampleCount.end(), 0);
        
        m_image.Clear();
    }

    int GetWidth() const { return m_image.GetWidth(); }
    int GetHeight() const { return m_image.GetHeight(); }

    Image& GetImage() { return m_image; }
    const Image& GetImage() const { return m_image; }

private:
    Image m_image;
    int m_width, m_height;

    std::vector<glm::vec3> m_accumulatedRadiance;
    std::vector<uint32_t> m_sampleCount;
};
