#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <atomic>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <thread>
#include <glm/glm.hpp>

#include <tool/shader.h>
#include <ray_tracing/RayTracing.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow *window, std::atomic<bool>& shouldStop);
void updateImage(const Image& image, GLuint texture);


float deltaTime = 0.0;
float screenWidth = 800.0f, screenHeight = 600.0f;
float lastX = screenWidth / 2, lastY = screenHeight / 2;
bool firstMouse = true;
int SamplePerPixel = 10;

Camera camera(screenWidth, screenHeight, glm::vec3{0.0f, 0.0f, 2.85f});

int main()
{
	glfwInit();
	//设置主要和次要版本
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	//创建窗口对象
	GLFWwindow *window = glfwCreateWindow(screenWidth, screenHeight, "window", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	//初始化GLAD块
	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	
	
	glViewport(0, 0, screenWidth, screenHeight);

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);

	glEnable(GL_PROGRAM_POINT_SIZE);  //允许点的大小变化
	glEnable(GL_DEPTH_TEST);   //深度
	glDepthFunc(GL_LESS);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_MULTISAMPLE);  //多重采样
	// glEnable(GL_FRAMEBUFFER_SRGB); 

	
	float screen_vertices[] = {
		-1.0f,  1.0f,  0.0f,  0.0f, // top left
		-1.0f, -1.0f,  0.0f,  1.0f, // bottom left
		 1.0f, -1.0f,  1.0f,  1.0f, // bottom right
		 1.0f,  1.0f,  1.0f,  0.0f  // top right
	};
    uint32_t screen_indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    uint32_t screen_vao, screen_vbo, screen_ebo;
    glGenVertexArrays(1, &screen_vao);
    glGenBuffers(1, &screen_vbo);
    glGenBuffers(1, &screen_ebo);

    glBindVertexArray(screen_vao);

    glBindBuffer(GL_ARRAY_BUFFER, screen_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screen_vertices), screen_vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, screen_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(screen_indices), screen_indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
	
	Shader shader("./src/61_path_tracing_2/shader/vertexShader.glsl", 
		"./src/61_path_tracing_2/shader/fragmentShader.glsl");

	int imageWidth = static_cast<int>(screenWidth);
	int imageHeight = static_cast<int>(screenHeight);
	Film film{imageWidth, imageHeight};

	GLuint imageTexture;
	glGenTextures(1, &imageTexture);
	glBindTexture(GL_TEXTURE_2D, imageTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);	
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glBindTexture(GL_TEXTURE_2D, 0);

	Scene world;
	const glm::vec3 white{0.73f, 0.73f, 0.73f};
	const glm::vec3 red{0.65f, 0.05f, 0.05f};
	const glm::vec3 green{0.12f, 0.45f, 0.15f};
	const glm::vec3 blue{0.1f, 0.1f, 0.85f};
	// Cornell box: open at z = 1, back wall at z = -1.
	world.AddSceneObject<Plane>(
		glm::vec3{-1.0f, -1.0f,  1.0f}, glm::vec3{ 1.0f, -1.0f,  1.0f},
		glm::vec3{ 1.0f, -1.0f, -1.0f}, glm::vec3{-1.0f, -1.0f, -1.0f})
		->AddMaterial<ConductorSpecularMaterial>(); // floor
	world.AddSceneObject<Plane>(
		glm::vec3{-1.0f,  1.0f, -1.0f}, glm::vec3{ 1.0f,  1.0f, -1.0f},
		glm::vec3{ 1.0f,  1.0f,  1.0f}, glm::vec3{-1.0f,  1.0f,  1.0f})
		->AddMaterial<LambertMaterial>(white); // ceiling
	world.AddSceneObject<Plane>(
		glm::vec3{-1.0f, -1.0f, -1.0f}, glm::vec3{ 1.0f, -1.0f, -1.0f},
		glm::vec3{ 1.0f,  1.0f, -1.0f}, glm::vec3{-1.0f,  1.0f, -1.0f})
		->AddMaterial<LambertMaterial>(white); // back wall
	world.AddSceneObject<Plane>( 
		glm::vec3{-1.0f, -1.0f, -1.0f}, glm::vec3{-1.0f, -1.0f,  1.0f},
		glm::vec3{-1.0f,  1.0f,  1.0f}, glm::vec3{-1.0f,  1.0f, -1.0f})
		->AddMaterial<LambertMaterial>(red); // left wall
	world.AddSceneObject<Plane>(
		glm::vec3{ 1.0f, -1.0f,  1.0f}, glm::vec3{ 1.0f, -1.0f, -1.0f},
		glm::vec3{ 1.0f,  1.0f, -1.0f}, glm::vec3{ 1.0f,  1.0f,  1.0f})
		->AddMaterial<LambertMaterial>(green); // right wall
	// world.AddSceneObject<Plane>(
	// 	glm::vec3{ 0.5f, -0.3f,  0.3f}, glm::vec3{ 0.5f, -0.3f, -0.3f},
	// 	glm::vec3{ 0.5f,  0.3f, -0.3f}, glm::vec3{ 0.5f,  0.3f,  0.3f})
	// 	->AddMaterial<DiffuseLightMaterial>(blue, true); // light

	// The current renderer has no emissive area material yet, so use a point
	// light just below the ceiling as an approximation of the Cornell ceiling light.
	// world.AddLight<PointLight>(
	// 	glm::vec3{4.0f, 4.0f, 4.0f}, glm::vec3{0.0f, 0.85f, 0.0f},
	// 	glm::vec3{0.032f, 0.09f, 1.0f}); 

	auto* lightObj = world.AddSceneObject<Plane>( 
		glm::vec3{-0.5f,  0.85f, -0.5f}, glm::vec3{ 0.5f,  0.85f, -0.5f},
		glm::vec3{ 0.5f,  0.85f,  0.5f}, glm::vec3{-0.5f,  0.85f,  0.5f});
	auto* lightMaterial = lightObj
		->AddMaterial<DiffuseLightMaterial>(glm::vec3{10.0f}, false); 
	world.AddLight<AreaLight>(lightObj, lightMaterial); 

	// Both spheres rest on the floor (center.y - radius == -1).
	world.AddHittable<Sphere>(glm::vec3{-0.4f, -0.45f, -0.25f}, 0.55f) 
		->AddMaterial<ConductorSpecularMaterial>();
	world.AddHittable<Sphere>(glm::vec3{0.45f, -0.65f, 0.25f}, 0.35f) 
		->AddMaterial<DielectricSpecularMaterial>();

	float lastTime = 0.0f;

	std::mutex imageMutex;
	std::atomic<bool> imageDirty{false};
	Camera renderCamera = camera;
	const PathIntegrator integrator{world};
	Renderer renderer{integrator};
	RenderTaskDesc taskDesc{
		film, imageMutex, imageDirty, renderCamera
	};
	std::atomic<bool> shouldStop = false;
	std::thread renderThread{[&]() {
		while (!shouldStop.load()) {
			if (renderer.IsCancelled()) {
				std::this_thread::yield();
				continue;
			}
			renderer.RenderPass(taskDesc);
		}
	}};

	while(!glfwWindowShouldClose(window))
	{
		float time = glfwGetTime();
		deltaTime = time - lastTime;
		lastTime = time;

		processInput(window, shouldStop);

		if (shouldStop) {
			renderer.Stop();
		}

		glClearColor(0.0, 0.0, 0.0, 0.0); 
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (!renderCamera.IsSameState(camera)) {
			renderer.Cancel();
			renderer.WaitForPass();
			renderCamera = camera;
			taskDesc.camera = renderCamera;
			{
				std::lock_guard<std::mutex> lock(imageMutex);
				film.Clear();
			}
			renderer.ResetRequestCancelled(false);
		}

		if (imageDirty.exchange(false)) {
			std::lock_guard<std::mutex> lock(imageMutex);
			updateImage(film.GetImage(), imageTexture);
		}

		shader.use();
		shader.setInt("imageTexture", 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, imageTexture);

		glBindVertexArray(screen_vao);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		
		
		glfwSwapBuffers(window);
		glfwPollEvents(); 
	}

	renderer.Stop();
	renderer.Cancel();
	renderThread.join();

	glfwTerminate();
	return 0;
}

///////

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
} 

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		firstMouse = false;
		lastX = xpos;
		lastY = ypos;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}


void processInput(GLFWwindow *window, std::atomic<bool>& shouldStop)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
		shouldStop = true;
	}

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera::Forward, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera::Backward, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera::Left, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera::Right, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera::Down, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera::Up, deltaTime);
}

void updateImage(const Image& image, GLuint texture) {
	glBindTexture(GL_TEXTURE_2D, texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image.GetWidth(), image.GetHeight(),
		GL_RGB, GL_UNSIGNED_BYTE, image.Data());
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glBindTexture(GL_TEXTURE_2D, 0);
}

