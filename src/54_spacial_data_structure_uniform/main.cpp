#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <spatial/Generator2D.h>
#include "spatial/BruteForce2D.h"
#include "spatial/UniformGrid2D.h"
#include "tool/shader.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <vector>
#include <chrono>

namespace
{
constexpr int initialWidth = 2000;
constexpr int initialHeight = 1400;
constexpr float panelWidth = 250.0f;

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
BruteForce2D bruteForce2D;

AABB2 makeOrderedBox(const glm::vec2 &a, const glm::vec2 &b)
{
    return {glm::min(a, b), glm::max(a, b)};
}

AABB2 sceneBounds()
{
    return {{panelWidth + 20.0f, 20.0f},
            {static_cast<float>(framebufferWidth) - 20.0f,
             static_cast<float>(framebufferHeight) - 20.0f}};
}

void addFilledBox(std::vector<Vertex2D> &vertices, const AABB2 &box, const glm::vec3 &color)
{
    const glm::vec2 a(box.min.x, box.min.y);
    const glm::vec2 b(box.max.x, box.min.y);
    const glm::vec2 c(box.max.x, box.max.y);
    const glm::vec2 d(box.min.x, box.max.y);
    vertices.insert(vertices.end(), {{a, color}, {b, color}, {c, color},
                                     {a, color}, {c, color}, {d, color}});
}

void addBoxOutline(std::vector<Vertex2D> &vertices, const AABB2 &box, const glm::vec3 &color)
{
    const glm::vec2 a(box.min.x, box.min.y);
    const glm::vec2 b(box.max.x, box.min.y);
    const glm::vec2 c(box.max.x, box.max.y);
    const glm::vec2 d(box.min.x, box.max.y);
    vertices.insert(vertices.end(), {{a, color}, {b, color}, {b, color}, {c, color},
                                     {c, color}, {d, color}, {d, color}, {a, color}});
}

void regenerate(std::vector<Object2D> &objects, int count, UniformGrid2D &uniformGrid)
{
    objects.clear();
    objects.reserve(static_cast<std::size_t>(count));
	bruteForce2D.clear();
    uniformGrid.clear();
    const AABB2 world = sceneBounds();
	for (int i = 0; i < count; ++i) {
		const ObjectId id = static_cast<ObjectId>(i);
        objects.push_back(Generator2D::GenerateObject2D(id, world));
		bruteForce2D.insert(id);
		uniformGrid.insert(id, objects.back().bounds);
	}
}

void updateObjects(std::vector<Object2D> &objects, float deltaTime, UniformGrid2D &uniformGrid)
{
    const AABB2 world = sceneBounds();
    for (Object2D &object : objects)
    {
        const AABB2 oldBounds = object.bounds;
        const glm::vec2 offset = object.velocity * deltaTime;
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
                const float correction = object.bounds.max[axis] - world.max[axis];
                object.bounds.min[axis] -= correction;
                object.bounds.max[axis] -= correction;
                object.velocity[axis] = -std::abs(object.velocity[axis]);
            }
        }

        uniformGrid.update(object.id, oldBounds, object.bounds);
    }
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

    GLFWwindow *window = glfwCreateWindow(initialWidth, initialHeight, "2D Spatial Sandbox", nullptr, nullptr);
    if (window == nullptr)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(2);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        glfwTerminate();
        return -1;
    }
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
	glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

	Shader shader("./src/54_spacial_data_structure_uniform/shader/vertexShader.glsl", 
		"./src/54_spacial_data_structure_uniform/shader/fragmentShader.glsl");

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

    int objectCount = 80;
    int gridSize = 100;
    int queryMethod = 1;
    bool paused = false;
    bool showBounds = true;
	bool showUnselected = true;
    bool showGrid = true;
    std::vector<Object2D> objects;
    UniformGrid2D uniformGrid(framebufferWidth, framebufferHeight, gridSize);
    int gridWorldWidth = framebufferWidth;
    int gridWorldHeight = framebufferHeight;
    regenerate(objects, objectCount, uniformGrid);

    double previousTime = glfwGetTime();
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
			
		const double currentTime = glfwGetTime();
		const float deltaTime = std::min(static_cast<float>(currentTime - previousTime), 0.05f);
		previousTime = currentTime;

        if (gridWorldWidth != framebufferWidth || gridWorldHeight != framebufferHeight)
        {
            gridWorldWidth = framebufferWidth;
            gridWorldHeight = framebufferHeight;
            uniformGrid = UniformGrid2D(gridWorldWidth, gridWorldHeight, gridSize);
            for (const Object2D &object : objects)
                uniformGrid.insert(object.id, object.bounds);
        }

		if (!paused)
		updateObjects(objects, deltaTime, uniformGrid);

		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			dragStart -= glm::vec2(deltaTime * 100, 0.0f);
			dragEnd -= glm::vec2(deltaTime * 100, 0.0f);
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			dragStart += glm::vec2(deltaTime * 100, 0.0f);
			dragEnd += glm::vec2(deltaTime * 100, 0.0f);
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			dragStart += glm::vec2(0.0f, deltaTime * 100);
			dragEnd += glm::vec2(0.0f, deltaTime * 100);
		}
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			dragStart -= glm::vec2(0.0f, deltaTime * 100);
			dragEnd -= glm::vec2(0.0f, deltaTime * 100);
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
        if (!ImGui::GetIO().WantCaptureMouse && leftPressed && !dragging && mouseInScene)
        {
            dragging = true;
            dragStart = mouse;
            dragEnd = mouse;
        }
        if (dragging && leftPressed)
            dragEnd = mouse;
        if (dragging && !leftPressed)
        {
            dragEnd = mouse;
            dragging = false;
        }

        const AABB2 selection = makeOrderedBox(dragStart, dragEnd);
        const glm::vec2 selectionSize = selection.size();
        const bool hasSelection = glm::dot(selectionSize, selectionSize) > 4.0f;
        int selectedCount = 0;

		auto timeBegin = std::chrono::steady_clock::now();
        std::vector<ObjectId> selectedIds;
        if (hasSelection)
        {
            if (queryMethod == 0)
                selectedIds = bruteForce2D.queryRange(selection, objects);
            else
                selectedIds = uniformGrid.queryRange(selection, objects);
        }
		auto timeEnd = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(timeEnd - timeBegin);

        bool queryMatchesBruteForce = true;
        if (hasSelection)
        {
            std::vector<ObjectId> bruteIds = bruteForce2D.queryRange(selection, objects);
            std::vector<ObjectId> gridIds = uniformGrid.queryRange(selection, objects);
            std::sort(bruteIds.begin(), bruteIds.end());
            std::sort(gridIds.begin(), gridIds.end());
            queryMatchesBruteForce = bruteIds == gridIds;
        }

        std::vector<bool> selected(objects.size(), false);
        for (ObjectId id : selectedIds)
        {
            if (id < selected.size())
                selected[id] = true;
        }
        selectedCount = static_cast<int>(selectedIds.size());

        std::vector<Vertex2D> triangles;
        std::vector<Vertex2D> lines;
        triangles.reserve(objects.size() * 6);
		lines.reserve(objects.size() * 8 +
                      static_cast<std::size_t>(uniformGrid.rows() + uniformGrid.columns()) * 2 + 16);
		if (!showUnselected) {
			for (ObjectId id : selectedIds) {
				const Object2D &object = objects[id];
				addFilledBox(triangles, object.bounds,
							 glm::vec3(1.0f, 0.55f, 0.18f));
				if (showBounds)
					addBoxOutline(lines, object.bounds, glm::vec3(0.08f, 0.10f, 0.14f));
			}
		} else {
			for (const Object2D &object : objects)
			{
				const bool isSelected = selected[object.id];
				addFilledBox(triangles, object.bounds,
							 isSelected ? glm::vec3(1.0f, 0.55f, 0.18f) : object.color);
				if (showBounds)
					addBoxOutline(lines, object.bounds, glm::vec3(0.08f, 0.10f, 0.14f));
			}
		}

        if (showGrid)
        {
            const glm::vec3 gridColor(0.16f, 0.19f, 0.24f);
            const AABB2 world = sceneBounds();
            for (int column = 0; column <= uniformGrid.columns(); ++column)
            {
                const float x = static_cast<float>(column * uniformGrid.gridSize());
                if (x >= world.min.x && x <= world.max.x)
                    lines.insert(lines.end(), {{{x, world.min.y}, gridColor}, {{x, world.max.y}, gridColor}});
            }
            for (int row = 0; row <= uniformGrid.rows(); ++row)
            {
                const float y = static_cast<float>(row * uniformGrid.gridSize());
                if (y >= world.min.y && y <= world.max.y)
                    lines.insert(lines.end(), {{{world.min.x, y}, gridColor}, {{world.max.x, y}, gridColor}});
            }
        }

        addBoxOutline(lines, sceneBounds(), glm::vec3(0.55f, 0.60f, 0.68f));
        if (hasSelection || dragging)
            addBoxOutline(lines, selection, glm::vec3(1.0f, 0.85f, 0.20f));

        ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(panelWidth - 20.0f, 0.0f), ImGuiCond_Always);
        ImGui::Begin("2D Sandbox", nullptr,
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Objects: %d", static_cast<int>(objects.size()));
        ImGui::Text("Selected: %d", selectedCount);
		ImGui::Text("Query: %s", queryMethod == 0 ? "Brute force" : "Uniform grid");
		ImGui::Text("Query time: %lld us", static_cast<long long>(duration.count()));
		ImGui::TextColored(queryMatchesBruteForce ? ImVec4(0.35f, 0.90f, 0.45f, 1.0f)
		                                              : ImVec4(1.0f, 0.30f, 0.25f, 1.0f),
		                   "Grid validation: %s", queryMatchesBruteForce ? "PASS" : "MISMATCH");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Separator();
        ImGui::Checkbox("Pause", &paused);
        ImGui::Checkbox("Show outlines", &showBounds);
        ImGui::Checkbox("Show grid", &showGrid);
        ImGui::Checkbox("Show unselected", &showUnselected);
        const char *queryMethods[] = {"Brute force", "Uniform grid"};
        ImGui::Combo("Query method", &queryMethod, queryMethods, 2);
        ImGui::SliderInt("Count", &objectCount, 1, 10000);
        bool rebuildGrid = ImGui::SliderInt("Cell size", &gridSize, 20, 1000);
        if (rebuildGrid)
        {
            uniformGrid = UniformGrid2D(framebufferWidth, framebufferHeight, gridSize);
            for (const Object2D &object : objects)
                uniformGrid.insert(object.id, object.bounds);
        }
        if (ImGui::Button("Regenerate"))
        {
            regenerate(objects, objectCount, uniformGrid);
            dragStart = dragEnd = glm::vec2(0.0f);
        }
        ImGui::Separator();
        ImGui::TextWrapped("Drag with the left mouse button in the scene to create a query rectangle.");
        ImGui::TextWrapped("Switch query methods to compare brute force with the uniform grid.");
        ImGui::End();

        glClearColor(0.055f, 0.065f, 0.085f, 1.0f);
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
