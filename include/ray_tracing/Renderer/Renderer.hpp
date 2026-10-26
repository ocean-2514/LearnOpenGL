#pragma once

#include "ray_tracing/Core/Camera.hpp"
#include "ray_tracing/Core/Film.hpp"
#include "ray_tracing/Integrator/PathIntegrator.hpp"
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <iostream>

struct RenderTaskDesc {
    Film& film;
    std::mutex& filmMutex;
    std::atomic<bool>& filmDirty;
    Camera& camera;
    int samplesPerPass = 1;
};

class Renderer {
public:
    explicit Renderer(const PathIntegrator& integrator, int blockSize = 50)
        : m_integrator(integrator), m_blockSize(blockSize) {
    }

    void Render(Film& film, std::mutex& filmMutex,
        std::atomic<bool>& filmDirty, const Camera& camera,
        int samplesPerPixel) const {
        std::atomic<int> nextBlock{0};
        const int width = film.GetWidth();
        const int height = film.GetHeight();
        const int columns = (width + m_blockSize - 1) / m_blockSize;
        const int rows = (height + m_blockSize - 1) / m_blockSize;
        const int blockCount = columns * rows;

        auto renderBlocks = [&]() {
            while (true) {
                const int blockIndex = nextBlock.fetch_add(1);
                if (blockIndex >= blockCount) {
                    break;
                }

                const int blockY = blockIndex / columns;
                const int blockX = blockIndex % columns;
                const int xBegin = blockX * m_blockSize;
                const int yBegin = blockY * m_blockSize;
                const int xEnd = std::min(xBegin + m_blockSize, width);
                const int yEnd = std::min(yBegin + m_blockSize, height);

                std::vector<glm::vec3> tileColors;
                tileColors.reserve(static_cast<std::size_t>(xEnd - xBegin)
                    * static_cast<std::size_t>(yEnd - yBegin));

                for (int y = yBegin; y < yEnd; ++y) {
                    for (int x = xBegin; x < xEnd; ++x) {
                        glm::vec3 color{0.0f};
                        for (int sample = 0; sample < samplesPerPixel; ++sample) {
                            color += m_integrator.Li(camera.GetRandomRay(x, y));
                        }
                        color /= static_cast<float>(samplesPerPixel);
                        tileColors.push_back(color);
                    }
                }

                {
                    std::lock_guard<std::mutex> lock(filmMutex);
                    std::size_t pixel = 0;
                    for (int y = yBegin; y < yEnd; ++y) {
                        for (int x = xBegin; x < xEnd; ++x) {
                            film.SetLinearPixel(x, y, tileColors[pixel++]);
                        }
                    }
                }
                filmDirty.store(true);
            }
        };

        const unsigned int hardwareThreads = std::thread::hardware_concurrency();
        const std::size_t threadCount = std::max<std::size_t>(1, hardwareThreads);
        std::vector<std::thread> threads;
        threads.reserve(threadCount);
        for (std::size_t i = 0; i < threadCount; ++i) {
            threads.emplace_back(renderBlocks);
        }
        for (auto& thread : threads) {
            thread.join();
        }
    }
    
    void RenderPass(RenderTaskDesc& taskDesc) const {
        Film& film = taskDesc.film;
        std::mutex& filmMutex = taskDesc.filmMutex;
        std::atomic<bool>& filmDirty = taskDesc.filmDirty;
        const Camera& camera = taskDesc.camera; 
        int samplesThisPass = taskDesc.samplesPerPass;

        std::atomic<int> nextBlock{0};
        const int width = film.GetWidth();
        const int height = film.GetHeight();
        const int columns = (width + m_blockSize - 1) / m_blockSize;
        const int rows = (height + m_blockSize - 1) / m_blockSize;
        const int blockCount = columns * rows;

        m_passRunning = true;
        auto renderBlocks = [&, this]() {
            while (m_running && !m_cancelRequested) {
                const int blockIndex = nextBlock.fetch_add(1);
                if (blockIndex >= blockCount) {
                    break;
                }

                const int blockY = blockIndex / columns;
                const int blockX = blockIndex % columns;
                const int xBegin = blockX * m_blockSize;
                const int yBegin = blockY * m_blockSize;
                const int xEnd = std::min(xBegin + m_blockSize, width);
                const int yEnd = std::min(yBegin + m_blockSize, height);

                std::vector<glm::vec3> tileColors;
                tileColors.reserve(static_cast<std::size_t>(xEnd - xBegin)
                    * static_cast<std::size_t>(yEnd - yBegin));

                for (int y = yBegin; y < yEnd && m_running && !m_cancelRequested; ++y) {
                    for (int x = xBegin; x < xEnd; ++x) {
                        glm::vec3 color{0.0f};
                        for (int sample = 0; sample < samplesThisPass
                            && m_running && !m_cancelRequested; ++sample) {
                            color += m_integrator.Li(camera.GetRandomRay(x, y));
                        }
                        if (!m_running || m_cancelRequested) {
                            break;
                        }
                        tileColors.push_back(color);
                    }
                }

                if (!m_running || m_cancelRequested) {
                    break;
                }

                {
                    std::lock_guard<std::mutex> lock(filmMutex);
                    std::size_t pixel = 0;
                    for (int y = yBegin; y < yEnd; ++y) {
                        for (int x = xBegin; x < xEnd; ++x) {
                            film.AddSample(x, y, tileColors[pixel++], samplesThisPass);
                        }
                    }
                }
                filmDirty.store(true);
            }
        };

        const unsigned int hardwareThreads = std::thread::hardware_concurrency();
        const std::size_t threadCount = std::max<std::size_t>(1, hardwareThreads);
        std::vector<std::thread> threads;
        threads.reserve(threadCount);
        for (std::size_t i = 0; i < threadCount; ++i) {
            threads.emplace_back(renderBlocks);
        }
        for (auto& thread : threads) {
            thread.join();
        }

        m_passRunning = false;
        m_passFinished.notify_all();
    }

    void ResetRequestCancelled(bool cancelRequested) {
        m_cancelRequested = cancelRequested;
    }

    void Cancel() {
        m_cancelRequested = true;
    }

    bool IsCancelled() const {
        return m_cancelRequested.load();
    }

    void WaitForPass() const {
        std::unique_lock<std::mutex> lock(m_passMutex);
        m_passFinished.wait(lock, [this]() {
            return !m_passRunning.load();
        });
    }

    void Stop() {
        m_running = false;
    }

private:
    const PathIntegrator& m_integrator;
    std::atomic<bool> m_cancelRequested = false;
    std::atomic<bool> m_running = true;
    mutable std::atomic<bool> m_passRunning = false;
    mutable std::mutex m_passMutex;
    mutable std::condition_variable m_passFinished;
    int m_blockSize;
};
