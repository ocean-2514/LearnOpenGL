#pragma once

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#undef STB_IMAGE_WRITE_IMPLEMENTATION
#undef STB_IMAGE_WRITE_STATIC

#include <glm/glm.hpp>

class Image
{
public:
    Image(int width, int height)
        : m_width(width), m_height(height) {
        if (width <= 0 || height <= 0) {
            throw std::invalid_argument("Image dimensions must be positive");
        }

        m_strideBytes = m_bytesPerPixel * m_width;
        m_data.resize(static_cast<std::size_t>(m_strideBytes) * m_height, 0);
    }

    const uint8_t* GetPixel(int x, int y) const {
        x = std::clamp(x, 0, m_width - 1);
        y = std::clamp(y, 0, m_height - 1);
        return m_data.data() + PixelOffset(x, y);
    }

    void SetPixel(int x, int y, const glm::vec3& color) {
        x = std::clamp(x, 0, m_width - 1);
        y = std::clamp(y, 0, m_height - 1);

        uint8_t* pixel = m_data.data() + PixelOffset(x, y);
        pixel[0] = FloatToByte(color.r);
        pixel[1] = FloatToByte(color.g);
        pixel[2] = FloatToByte(color.b);
    }

    bool Save(const std::string& path) const {
        return stbi_write_png(path.c_str(), m_width, m_height, m_bytesPerPixel,
            m_data.data(), m_strideBytes) != 0;
    }

    const uint8_t* Data() const {
        return m_data.data();
    }

    void Clear() {
        std::fill(m_data.begin(), m_data.end(), 0);
    }

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

private:
    static constexpr int m_bytesPerPixel = 3;
    int m_strideBytes = 0;
    int m_width = 0;
    int m_height = 0;
    std::vector<uint8_t> m_data;

    std::size_t PixelOffset(int x, int y) const {
        return static_cast<std::size_t>(y) * m_strideBytes
            + static_cast<std::size_t>(x) * m_bytesPerPixel;
    }

    static uint8_t FloatToByte(float f) {
        if (f <= 0.0f) return 0;
        if (f >= 1.0f) return 255;

        return static_cast<uint8_t>(256.0f * f);
    }
};
