#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include "spatial/DynamicBVH2D.h"
#include "spatial/Generator2D.h"
#include "tool/shader.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <vector>

namespace
{
constexpr int initialWidth = 1600;
constexpr int initialHeight = 1000;
constexpr float panelWidth = 290.0f;

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

bool sameIds(std::vector<ObjectId> left, std::vector<ObjectId> right)
{
    std::sort(left.begin(), left.end());
    std::sort(right.begin(), right.end());
    return left == right;
}

void regenerate(std::vector<Object2D> &objects,
                std::vector<DynamicBVHTree2D::ProxyId> &proxies,
                int count, DynamicBVHTree2D &tree)
{
    tree.clear();
    objects.clear();
    proxies.clear();
    objects.reserve(static_cast<std::size_t>(count));
    proxies.reserve(static_cast<std::size_t>(count));

    const AABB2 world = sceneBounds();
    for (int index = 0; index < count; ++index)
    {
        objects.push_back(Generator2D::GenerateObject2D(
            static_cast<ObjectId>(index), world, 12.0f, 55.0f, 25.0f, 110.0f));
        proxies.push_back(tree.createProxy(objects.back().id, objects.back().bounds));
    }
}

int updateObjects(std::vector<Object2D> &objects,
                  const std::vector<DynamicBVHTree2D::ProxyId> &proxies,
                  DynamicBVHTree2D &tree, float deltaTime)
{
    const AABB2 world = sceneBounds();
    int reinsertionCount = 0;
    for (Object2D &object : objects)
    {
        const glm::vec2 oldCenter = object.bounds.center();
        glm::vec2 offset = object.velocity * deltaTime;
        object.bounds.min += offset;
        object.bounds.max += offset;

        for (int axis = 0; axis < 2; ++axis)
        {
            if (object.bounds.min[axis] < world.min[axis])
            {
                const float correction = world.min[axis] - object.bounds.min[axis];
                object.bounds.min[axis] += correction;
                object.bounds.max[axis] += correction;
                object.velocity[axis] = std::abs(object.velocity[axis]);
            }
            else if (object.bounds.max[axis] > world.max[axis])
            {
                const float correction = world.max[axis] - object.bounds.max[axis];
                object.bounds.min[axis] += correction;
                object.bounds.max[axis] += correction;
                object.velocity[axis] = -std::abs(object.velocity[axis]);
            }
        }

        const glm::vec2 displacement = object.bounds.center() - oldCenter;
        if (tree.moveProxy(proxies[object.id], object.bounds, displacement))
            ++reinsertionCount;
    }
    return reinsertionCount;
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
                                           "Dynamic 2D BVH", nullptr, nullptr);
    if (window == nullptr)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        glfwDestroyWindow(window);
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

    Shader shader("./src/57_spacial_data_structure_bvh/shader/vertexShader.glsl",
                  "./src/57_spacial_data_structure_bvh/shader/fragmentShader.glsl");

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

    int objectCount = 250;
    bool paused = false;
    bool showObjectBounds = true;
    bool showFatBounds = true;
    bool showInternalBounds = true;
    int minInternalHeight = 2;
    int knownWidth = framebufferWidth;
    int knownHeight = framebufferHeight;
    int reinsertionCount = 0;
    long long updateMicroseconds = 0;
    long long queryMicroseconds = 0;
    double previousTime = glfwGetTime();

    DynamicBVHTree2D tree(18.0f, 2.0f);
    std::vector<Object2D> objects;
    std::vector<DynamicBVHTree2D::ProxyId> proxies;
    regenerate(objects, proxies, objectCount, tree);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        const double currentTime = glfwGetTime();
        const float deltaTime = std::min(static_cast<float>(currentTime - previousTime), 0.05f);
        previousTime = currentTime;

        if (knownWidth != framebufferWidth || knownHeight != framebufferHeight)
        {
            knownWidth = framebufferWidth;
            knownHeight = framebufferHeight;
            regenerate(objects, proxies, objectCount, tree);
            dragStart = dragEnd = glm::vec2(0.0f);
        }

        const auto updateBegin = std::chrono::steady_clock::now();
        reinsertionCount = paused ? 0 : updateObjects(objects, proxies, tree, deltaTime);
        const auto updateEnd = std::chrono::steady_clock::now();
        updateMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(
                                 updateEnd - updateBegin)
                                 .count();

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
        const bool hasSelection = glm::dot(selection.size(), selection.size()) > 4.0f;
        std::vector<ObjectId> selected;
        const auto queryBegin = std::chrono::steady_clock::now();
        if (hasSelection)
            tree.query(selection, selected);
        const auto queryEnd = std::chrono::steady_clock::now();
        queryMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(
                                queryEnd - queryBegin)
                                .count();

        std::vector<ObjectId> bruteResult;
        if (hasSelection)
        {
            for (const Object2D &object : objects)
                if (selection.intersects(object.bounds))
                    bruteResult.push_back(object.id);
        }
        const bool queryMatches = sameIds(selected, bruteResult);
        std::vector<bool> isSelected(objects.size(), false);
        for (ObjectId id : selected)
            if (id < isSelected.size())
                isSelected[id] = true;

        std::vector<DynamicBVHTree2D::DebugNode> debugNodes;
        tree.getDebugNodes(debugNodes);
        std::vector<Vertex2D> triangles;
        std::vector<Vertex2D> lines;
        triangles.reserve(objects.size() * 6);
        lines.reserve((objects.size() + debugNodes.size()) * 8 + 16);

        for (const DynamicBVHTree2D::DebugNode &node : debugNodes)
        {
            if (node.leaf && showFatBounds)
                addBoxOutline(lines, node.bounds, glm::vec3(0.16f, 0.42f, 0.29f));
            else if (!node.leaf && showInternalBounds && node.height >= minInternalHeight)
            {
                const float shade = std::min(0.22f + node.height * 0.025f, 0.55f);
                addBoxOutline(lines, node.bounds, glm::vec3(shade, 0.20f, 0.42f));
            }
        }

        for (const Object2D &object : objects)
        {
            const glm::vec3 color = isSelected[object.id]
                                        ? glm::vec3(1.0f, 0.48f, 0.14f)
                                        : object.color;
            addFilledBox(triangles, object.bounds, color);
            if (showObjectBounds)
                addBoxOutline(lines, object.bounds, glm::vec3(0.08f, 0.10f, 0.14f));
        }
        addBoxOutline(lines, sceneBounds(), glm::vec3(0.58f, 0.62f, 0.70f));
        if (hasSelection || dragging)
            addBoxOutline(lines, selection, glm::vec3(0.25f, 1.0f, 0.55f));

        ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(panelWidth - 20.0f, 0.0f), ImGuiCond_Always);
        ImGui::Begin("Dynamic BVH", nullptr,
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Objects: %d", static_cast<int>(objects.size()));
        ImGui::Text("Tree nodes: %d", static_cast<int>(debugNodes.size()));
        ImGui::Text("Tree height: %d", tree.height());
        ImGui::Text("Selected: %d", static_cast<int>(selected.size()));
        ImGui::Text("Reinsertions/frame: %d", reinsertionCount);
        ImGui::Text("Update: %lld us", updateMicroseconds);
        ImGui::Text("Query: %lld us", queryMicroseconds);
        ImGui::TextColored(queryMatches ? ImVec4(0.35f, 0.90f, 0.45f, 1.0f)
                                        : ImVec4(1.0f, 0.30f, 0.25f, 1.0f),
                           "Query validation: %s", queryMatches ? "PASS" : "MISMATCH");
        ImGui::Separator();
        ImGui::Checkbox("Pause", &paused);
        ImGui::Checkbox("Object AABBs", &showObjectBounds);
        ImGui::Checkbox("Fat leaf AABBs", &showFatBounds);
        ImGui::Checkbox("Internal AABBs", &showInternalBounds);
        ImGui::SliderInt("Min node height", &minInternalHeight, 1,
                         std::max(tree.height(), 1));
        ImGui::SliderInt("Object count", &objectCount, 1, 5000);
        if (ImGui::Button("Regenerate"))
        {
            regenerate(objects, proxies, objectCount, tree);
            dragStart = dragEnd = glm::vec2(0.0f);
        }
        ImGui::Separator();
        ImGui::TextWrapped("Objects update their proxies every frame. A proxy is reinserted only after its real AABB leaves the Fat AABB.");
        ImGui::TextWrapped("Drag in the scene to run an exact AABB range query.");
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
