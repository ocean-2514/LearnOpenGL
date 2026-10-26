#pragma once

#include "ray_tracing/Core/Frame.hpp"
#include "ray_tracing/Scene/Scene.hpp"
#include "ray_tracing/Sampler/Sampling.hpp"
#include <cmath>
#include <glm/glm.hpp>

class PathIntegrator {
public:
    struct Settings {
        int minDepth = 3;
        int maxDepth = 10;
        float survivalChance = 0.8f;
        float rayMinT = 0.01f;
        float shadowRayMinT = 0.001f;
        float rayMaxT = 1000.0f;
    };

    explicit PathIntegrator(const Scene& scene)
        : m_scene(scene) {
    }

    PathIntegrator(const Scene& scene, const Settings& settings) 
        : m_scene(scene), m_settings(settings) {
    }

    glm::vec3 Li(const Ray& ray) const {
        // return LiLoop(ray);
        return Li(ray, 0, glm::vec3{0.0f}, 0.0f, true);
    }

private:
    glm::vec3 Li(const Ray& ray, int depth,
        const glm::vec3& previousPoint, float previousBsdfPdf,
        bool previousBounceWasDelta) const {
        glm::vec3 color{0.0f};
        if (depth >= m_settings.maxDepth) {
            return color;
        }

        float rouletteWeight = 1.0f;
        if (depth > m_settings.minDepth) {
            if (Random01() > m_settings.survivalChance) {
                return color;
            }
            rouletteWeight /= m_settings.survivalChance;
        }

        HitRecord record;
        if (!m_scene.hit(ray, m_settings.rayMinT, m_settings.rayMaxT, record)) {
            return color;
        }

        const glm::vec3& point = record.point;
        const glm::vec3& normal = record.normal;
        const Material* material = record.material;
        const glm::vec3 wo = -glm::normalize(ray.direction);

        const glm::vec3 outwardNormal = record.frontFace
            ? record.normal
            : -record.normal;
        const glm::vec3 frameNormal = 
            material->IsRefractable() || material->IsEmissive()
            ? outwardNormal
            : record.normal;
        const Frame frame(frameNormal);
        const glm::vec3 woLocal = frame.ToLocal(wo);

        if (material->IsEmissive()) {
            float weight = 1.0f;
            if (depth > 0 && !previousBounceWasDelta) {
                const float lightPdf = LightPdf(
                    previousPoint, glm::normalize(ray.direction));
                weight = PowerHeuristic(previousBsdfPdf, lightPdf);
            }
            color += weight * material->Emission(woLocal);
        }

        const auto& lights = m_scene.GetLights();
        if (!material->IsSpecular() && !lights.empty()) {
            const Light* selectedLight = m_scene.GetRandomLight();
            LightSample sample{};
            if (selectedLight && selectedLight->SampleLi(point, sample)) {
                const float lightPdf = sample.pdf
                    / static_cast<float>(lights.size());
                const float shadowMaxT = sample.distance
                    - m_settings.shadowRayMinT;

                if (lightPdf > 0.0f && std::isfinite(lightPdf)
                    && shadowMaxT > m_settings.shadowRayMinT) {
                    HitRecord shadowRecord{};
                    const bool occluded = m_scene.hit(
                        Ray{point, sample.wi}, m_settings.shadowRayMinT,
                        shadowMaxT, shadowRecord) != nullptr;

                    if (!occluded) {
                        const glm::vec3 wiLocal = frame.ToLocal(sample.wi);
                        const float bsdfPdf = material->Pdf(woLocal, wiLocal);
                        const float weight = selectedLight->IsDelta()
                            ? 1.0f
                            : PowerHeuristic(lightPdf, bsdfPdf);
                        color += weight * sample.radiance
                            * material->BRDF(woLocal, wiLocal)
                            * glm::max(glm::dot(sample.wi, normal), 0.0f)
                            / lightPdf;
                    }
                }
            }
        }

        float pdf = 0.0f;
        glm::vec3 reflectedLocal;
        if (material->SampleWi(woLocal, reflectedLocal, pdf)) {
            const float reflectedCosine = std::fabs(reflectedLocal.z);
            if (pdf > 0.0f && reflectedCosine > 0.0f) {
                const Ray reflectedRay{point, frame.ToWorld(reflectedLocal)};
                color += Li(reflectedRay, depth + 1, point, pdf,
                    material->IsSpecular())
                    * material->BRDF(woLocal, reflectedLocal)
                    * reflectedCosine / pdf;
            }
        }

        glm::vec3 transmittedLocal;
        if (material->SampleWt(woLocal, transmittedLocal, pdf)) { 
            const Ray transmittedRay{point, frame.ToWorld(transmittedLocal)};
            color += Li(transmittedRay, depth + 1, point, 0.0f, true)
                * material->BTDF(woLocal, transmittedLocal)
                * std::fabs(transmittedLocal.z) / pdf;
        }

        return color * rouletteWeight;
    }

    float PowerHeuristic(float pdfA, float pdfB) const {
        float a2 = pdfA * pdfA;
        float b2 = pdfB * pdfB;
        const float denominator = a2 + b2;
        return denominator > 0.0f ? a2 / denominator : 0.0f;
    }

    float LightPdf(const glm::vec3& p, const glm::vec3& wi) const {
        const auto& lights = m_scene.GetLights();
        if (lights.empty()) return 0.0f;

        float pdf = 0.0f;
        for (const auto& light : lights) {
            pdf += light->PdfLi(p, wi);
        }
        return pdf / static_cast<float>(lights.size());
    }

    glm::vec3 LiLoop(const Ray& initialRay) const {
        glm::vec3 radiance{0.0f};
        glm::vec3 throughput{1.0f};
        Ray ray = initialRay;

        glm::vec3 previousPoint{0.0f};
        float previousBsdfPdf = 0.0f;
        bool previousBounceIsDelta = true;

        for (int depth = 0; depth < m_settings.maxDepth; ++depth) {
            HitRecord record;
            if (!m_scene.hit(ray, m_settings.rayMinT, m_settings.rayMaxT, record)) {
                break;
            }
            if (depth > m_settings.minDepth) {
                if (Random01() > m_settings.survivalChance) {
                    break;
                } else {
                    throughput /= m_settings.survivalChance;
                }
            }

            const auto& point = record.point;
            const auto* material = record.material;
            const auto& normal = record.normal;
            const glm::vec3 outwardNormal = record.frontFace ? normal : -normal;
            glm::vec3 frameNormal = 
                material->IsRefractable() || material->IsEmissive() ? 
                outwardNormal : normal;
            Frame frame{frameNormal};
            const glm::vec3 wo = glm::normalize(-ray.direction);
            const glm::vec3 woLocal = frame.ToLocal(wo);

            // emissive
            if (material->IsEmissive()) {
                float weight = 1.0f;
                if (depth > 0 && !previousBounceIsDelta && previousBsdfPdf > 0.0f) {
                    float lightPdf = LightPdf(previousPoint, -wo);
                    weight = PowerHeuristic(previousBsdfPdf, lightPdf);
                }
                radiance += weight * throughput * material->Emission(woLocal);
            }

            // NEE
            if (!material->IsSpecular()) {
                const Light* selectedLight = m_scene.GetRandomLight();
                int lightNum = m_scene.GetLights().size();
                if (selectedLight) {
                    LightSample lightSample;
                    if (selectedLight->SampleLi(point, lightSample)) {
                        glm::vec3 wi = lightSample.wi;
                        glm:: vec3 wiLocal = frame.ToLocal(wi);
                        Ray rayToLight{point, wi};
                        HitRecord temRecord;
                        if (!m_scene.hit(rayToLight, m_settings.shadowRayMinT, 
                            lightSample.distance - m_settings.shadowRayMinT, temRecord)) {
                            float pdf = lightSample.pdf / lightNum;
                            if (pdf > 0.0f && std::isfinite(pdf)) {
                                float bsdfPdf = material->Pdf(woLocal, wiLocal);
                                float weight = selectedLight->IsDelta() ? 1.0f : 
                                    PowerHeuristic(pdf, bsdfPdf);
                                radiance += weight * throughput * 
                                    material->BRDF(woLocal, wiLocal) *  
                                    lightSample.radiance
                                    * std::max(wiLocal.z, 0.0f) / pdf;
                            }
                        }
                    }
                }
            }

            // A delta BSDF chooses exactly one discrete event. Its returned
            // weight already includes the event probability correction.
            if (material->IsSpecular()) {
                DeltaSample deltaSample{};
                if (material->SampleDelta(woLocal, deltaSample)
                    && deltaSample.pdf > 0.0f
                    && std::isfinite(deltaSample.pdf)
                    && std::isfinite(deltaSample.weight.x)
                    && std::isfinite(deltaSample.weight.y)
                    && std::isfinite(deltaSample.weight.z)) {
                    throughput *= deltaSample.weight;
                    ray = Ray{point, frame.ToWorld(deltaSample.direction)};
                    previousBounceIsDelta = true;
                    previousBsdfPdf = 0.0f;
                    previousPoint = point;
                    continue;
                }
                break;
            }

            // BRDF
            {
                glm::vec3 wiLocal;
                float pdf;
                if (material->SampleWi(woLocal, wiLocal, pdf)) {
                    if (pdf > 0.0f && std::isfinite(pdf)) {
                        throughput *= material->BRDF(woLocal, wiLocal)
                            * std::fabs(wiLocal.z) / pdf;
                        ray = Ray{point, frame.ToWorld(wiLocal)};
                        previousBounceIsDelta = material->IsSpecular();
                        previousBsdfPdf = pdf;
                        previousPoint = point;
                        continue;
                    }
                }
            }

            // BTDF
            {
                glm::vec3 wiLocal;
                float pdf;
                if (material->SampleWt(woLocal, wiLocal, pdf)) {
                    if (pdf > 0.0f && std::isfinite(pdf)) {
                        throughput *= material->BTDF(woLocal, wiLocal)
                            * std::fabs(wiLocal.z) / pdf;
                        ray = Ray{point, frame.ToWorld(wiLocal)};
                        previousBounceIsDelta = true;
                        previousBsdfPdf = 0.0f;
                        previousPoint = point;
                        continue;
                    }
                }
            }

        }

        return radiance;
    }

    const Scene& m_scene;
    Settings m_settings;
};
