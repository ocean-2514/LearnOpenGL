#pragma once

#include <glm/glm.hpp>

struct LightSample {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f};
    glm::vec3 wi{0.0f};
    glm::vec3 radiance{0.0f};
    float distance = 0.0f;
    float pdf = 0.0f;       // 立体角 PDF
};

class Light {
public:
    explicit Light(const glm::vec3& color = {1.0f, 1.0f, 1.0f})
        : m_radiance(color) {}
    virtual ~Light() = default;

    virtual bool SampleLi(const glm::vec3& p, LightSample& sample) const = 0;
    virtual bool IsDelta() const { return false; }
    virtual float PdfLi(const glm::vec3& p, const glm::vec3& wi) const {
        // Delta lights have no density with respect to solid angle.
        return 0.0f;
    }

protected:
    glm::vec3 m_radiance{0.0f};
};
