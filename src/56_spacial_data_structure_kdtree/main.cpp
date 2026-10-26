#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include "spatial/KDTree.h"
#include "tool/shader.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace
{
constexpr int initialWidth = 1600;
constexpr int initialHeight = 1000;
constexpr float panelWidth = 270.0f;
constexpr float pointRadius = 4.0f;

struct Vertex2D
{
    glm::vec2 position;
    glm::vec3 color;
};

int framebufferWidth = initialWidth;
int framebufferHeight = initialHeight;
bool dragging = false;
glm::vec2 dragStart(0.0f);
glm::vec2 dragEnd(0.0f);

AABB2 sceneBounds()
{
    return {{panelWidth + 20.0f, 20.0f},
            {static_cast<float>(framebufferWidth) - 20.0f,
             static_cast<float>(framebufferHeight) - 20.0f}};
}

AABB2 orderedBox(const glm::vec2 &a, const glm::vec2 &b)
{
    return {glm::min(a, b), glm::max(a, b)};
}

float distanceSquared(const glm::vec2 &a, const glm::vec2 &b)
{
    const glm::vec2 delta = a - b;
    return glm::dot(delta, delta);
}

void addFilledBox(std::vector<Vertex2D> &vertices, const AABB2 &box,
                  const glm::vec3 &color)
{
    const glm::vec2 a(box.min.x, box.min.y);
    const glm::vec2 b(box.max.x, box.min.y);
    const glm::vec2 c(box.max.x, box.max.y);
    const glm::vec2 d(box.min.x, box.max.y);
    vertices.insert(vertices.end(), {{a, color}, {b, color}, {c, color},
                                     {a, color}, {c, color}, {d, color}});
}

void addBoxOutline(std::vector<Vertex2D> &vertices, const AABB2 &box,
                   const glm::vec3 &color)
{
    const glm::vec2 a(box.min.x, box.min.y);
    const glm::vec2 b(box.max.x, box.min.y);
    const glm::vec2 c(box.max.x, box.max.y);
    const glm::vec2 d(box.min.x, box.max.y);
    vertices.insert(vertices.end(), {{a, color}, {b, color}, {b, color}, {c, color},
                                     {c, color}, {d, color}, {d, color}, {a, color}});
}

void addLine(std::vector<Vertex2D> &vertices, const glm::vec2 &a,
             const glm::vec2 &b, const glm::vec3 &color)
{
    vertices.push_back({a, color});
    vertices.push_back({b, color});
}

void addPoint(std::vector<Vertex2D> &triangles, const Point2D &point,
              float radius, const glm::vec3 &color)
{
    addFilledBox(triangles, {point.pos - glm::vec2(radius),
                             point.pos + glm::vec2(radius)}, color);
}

void regenerate(std::vector<Point2D> &points, int count, KDTree &tree)
{
    static std::mt19937 engine(std::random_device{}());
    const AABB2 world = sceneBounds();
    std::uniform_real_distribution<float> xDistribution(world.min.x + 8.0f,
                                                         world.max.x - 8.0f);
    std::uniform_real_distribution<float> yDistribution(world.min.y + 8.0f,
                                                         world.max.y - 8.0f);

    points.clear();
    points.reserve(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index)
    {
        points.push_back({static_cast<ObjectId>(index),
                          {xDistribution(engine), yDistribution(engine)}});
    }
    tree.build(points);
}

std::vector<Point2D> bruteRange(const std::vector<Point2D> &points,
                                const AABB2 &range)
{
    std::vector<Point2D> result;
    for (const Point2D &point : points)
    {
        if (range.contains(point.pos))
            result.push_back(point);
    }
    return result;
}

std::vector<Point2D> bruteKNearest(const std::vector<Point2D> &points,
                                   const glm::vec2 &query, std::size_t k)
{
    std::vector<Point2D> result = points;
    std::sort(result.begin(), result.end(), [&query](const Point2D &a, const Point2D &b) {
        const float aDistance = distanceSquared(a.pos, query);
        const float bDistance = distanceSquared(b.pos, query);
        if (aDistance != bDistance)
            return aDistance < bDistance;
        return a.id < b.id;
    });
    if (result.size() > k)
        result.resize(k);
    return result;
}

bool sameIds(std::vector<Point2D> a, std::vector<Point2D> b)
{
    const auto byId = [](const Point2D &left, const Point2D &right) {
        return left.id < right.id;
    };
    std::sort(a.begin(), a.end(), byId);
    std::sort(b.begin(), b.end(), byId);
    if (a.size() != b.size())
        return false;
    for (std::size_t index = 0; index < a.size(); ++index)
    {
        if (a[index].id != b[index].id)
            return false;
    }
    return true;
}

void framebufferSizeCallback(GLFWwindow *, int width, int height)
{
    framebufferWidth = std::max(width, 1);
    framebufferHeight = std::max(height, 1);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
}
} // namespace

int main()
{
    if (glfwInit() == GLFW_FALSE)
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *window = glfwCreateWindow(initialWidth, initialHeight,
                                           "Static 2D KD-Tree", nullptr, nullptr);
    if (window == nullptr)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        glfwTerminate();
        return -1;
    }
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glViewport(0, 0, framebufferWidth, framebufferHeight);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    Shader shader("./src/56_spacial_data_structure_kdtree/shader/vertexShader.glsl",
                  "./src/56_spacial_data_structure_kdtree/shader/fragmentShader.glsl");

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex2D),
                          reinterpret_cast<void *>(sizeof(glm::vec2)));
    glEnableVertexAttribArray(1);

    int pointCount = 300;
    int k = 8;
    bool showSplits = true;
    bool showRangeResult = true;
    int knownWidth = framebufferWidth;
    int knownHeight = framebufferHeight;
    std::vector<Point2D> points;
    KDTree tree;
    regenerate(points, pointCount, tree);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        if (knownWidth != framebufferWidth || knownHeight != framebufferHeight)
        {
            knownWidth = framebufferWidth;
            knownHeight = framebufferHeight;
            regenerate(points, pointCount, tree);
            dragStart = dragEnd = glm::vec2(0.0f);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        double mouseX = 0.0;
        double mouseY = 0.0;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        const glm::vec2 mouse(static_cast<float>(mouseX), static_cast<float>(mouseY));
        const bool mouseInScene = sceneBounds().contains(mouse);
        const bool leftPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        if (!ImGui::GetIO().WantCaptureMouse && mouseInScene && leftPressed && !dragging)
        {
            dragging = true;
            dragStart = dragEnd = mouse;
        }
        if (dragging && leftPressed)
            dragEnd = mouse;
        if (dragging && !leftPressed)
        {
            dragEnd = mouse;
            dragging = false;
        }

        const AABB2 selection = orderedBox(dragStart, dragEnd);
        const glm::vec2 selectionSize = selection.size();
        const bool hasSelection = glm::dot(selectionSize, selectionSize) > 4.0f;

        const auto knnBegin = std::chrono::steady_clock::now();
        const std::vector<Point2D> nearestPoints = mouseInScene
            ? tree.kNearest(mouse, static_cast<std::size_t>(k))
            : std::vector<Point2D>{};
        const auto knnEnd = std::chrono::steady_clock::now();
        const auto knnTime = std::chrono::duration_cast<std::chrono::microseconds>(
            knnEnd - knnBegin);

        const auto rangeBegin = std::chrono::steady_clock::now();
        const std::vector<Point2D> rangePoints = hasSelection
            ? tree.queryRange(selection)
            : std::vector<Point2D>{};
        const auto rangeEnd = std::chrono::steady_clock::now();
        const auto rangeTime = std::chrono::duration_cast<std::chrono::microseconds>(
            rangeEnd - rangeBegin);

        const bool knnMatches = !mouseInScene || sameIds(
            nearestPoints,
            bruteKNearest(points, mouse, static_cast<std::size_t>(k)));
        const bool rangeMatches = !hasSelection || sameIds(
            rangePoints, bruteRange(points, selection));

        std::vector<bool> inKnn(points.size(), false);
        std::vector<bool> inRange(points.size(), false);
        for (const Point2D &point : nearestPoints)
            inKnn[point.id] = true;
        for (const Point2D &point : rangePoints)
            inRange[point.id] = true;

        std::vector<Vertex2D> triangles;
        std::vector<Vertex2D> lines;
        triangles.reserve(points.size() * 6 + 6);
        lines.reserve(points.size() * 4 + 16);

        if (showSplits)
        {
            std::vector<KDTree::SplitLine> splitLines;
            tree.getSplitLines(sceneBounds(), splitLines);
            for (const KDTree::SplitLine &line : splitLines)
            {
                addLine(lines, line.begin, line.end,
                        line.axis == 0 ? glm::vec3(0.20f, 0.28f, 0.38f)
                                       : glm::vec3(0.27f, 0.20f, 0.36f));
            }
        }

        for (const Point2D &point : points)
        {
            glm::vec3 color(0.72f, 0.78f, 0.88f);
            float radius = pointRadius;
            if (showRangeResult && inRange[point.id])
                color = glm::vec3(0.25f, 0.90f, 0.50f);
            if (inKnn[point.id])
            {
                color = glm::vec3(1.0f, 0.48f, 0.16f);
                radius = pointRadius + 2.0f;
            }
            if (!nearestPoints.empty() && point.id == nearestPoints.front().id)
            {
                color = glm::vec3(1.0f, 0.90f, 0.15f);
                radius = pointRadius + 4.0f;
            }
            addPoint(triangles, point, radius, color);
        }

        if (mouseInScene)
        {
            for (const Point2D &point : nearestPoints)
                addLine(lines, mouse, point.pos, glm::vec3(0.45f, 0.30f, 0.18f));
            addFilledBox(triangles, {mouse - glm::vec2(3.0f), mouse + glm::vec2(3.0f)},
                         glm::vec3(0.30f, 0.85f, 1.0f));
        }

        addBoxOutline(lines, sceneBounds(), glm::vec3(0.58f, 0.62f, 0.70f));
        if (hasSelection || dragging)
            addBoxOutline(lines, selection, glm::vec3(0.25f, 1.0f, 0.55f));

        ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(panelWidth - 20.0f, 0.0f), ImGuiCond_Always);
        ImGui::Begin("Static KD-Tree", nullptr,
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Points: %d", static_cast<int>(points.size()));
        ImGui::Text("K: %d", k);
        ImGui::Text("Range hits: %d", static_cast<int>(rangePoints.size()));
        ImGui::Text("KNN time: %lld us", static_cast<long long>(knnTime.count()));
        ImGui::Text("Range time: %lld us", static_cast<long long>(rangeTime.count()));
        ImGui::TextColored(knnMatches ? ImVec4(0.35f, 0.90f, 0.45f, 1.0f)
                                      : ImVec4(1.0f, 0.30f, 0.25f, 1.0f),
                           "KNN validation: %s", knnMatches ? "PASS" : "MISMATCH");
        ImGui::TextColored(rangeMatches ? ImVec4(0.35f, 0.90f, 0.45f, 1.0f)
                                        : ImVec4(1.0f, 0.30f, 0.25f, 1.0f),
                           "Range validation: %s", rangeMatches ? "PASS" : "MISMATCH");
        ImGui::Separator();
        ImGui::Checkbox("Show split lines", &showSplits);
        ImGui::Checkbox("Show range hits", &showRangeResult);
        ImGui::SliderInt("K nearest", &k, 1, 64);
        ImGui::SliderInt("Point count", &pointCount, 1, 10000);
        if (ImGui::Button("Regenerate"))
        {
            regenerate(points, pointCount, tree);
            dragStart = dragEnd = glm::vec2(0.0f);
        }
        ImGui::Separator();
        ImGui::TextWrapped("Move the mouse in the scene to query K nearest points.");
        ImGui::TextWrapped("Drag with the left mouse button to perform an AABB range query.");
        ImGui::End();

        glClearColor(0.045f, 0.052f, 0.070f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        shader.use();
        shader.setVec2("uViewport", {framebufferWidth, framebufferHeight});
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(triangles.size() * sizeof(Vertex2D)),
                     triangles.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(triangles.size()));
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(lines.size() * sizeof(Vertex2D)),
                     lines.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lines.size()));

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
