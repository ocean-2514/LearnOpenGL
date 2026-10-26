#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <tool/shader.h>
#include <iostream>
#include <cmath>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cstdio>

#define STB_IMAGE_IMPLEMENTATION
#include <tool/stb_image.h>

#include <tool/shader.h>
// #include <tool/model.h>
#include <tool/camera.h>
#include <tool/math_extensions.h>
#include <geometry/BoxGeometry.h>
#include <geometry/PlaneGeometry.h>
#include <geometry/SphereGeometry.h>
// #include <imgui/imgui.h>
// #include <imgui/imgui_impl_glfw.h>     
// #include <imgui/imgui_impl_opengl3.h>     

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
// void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void processInput(GLFWwindow *window);
unsigned int loadCubeMap(vector<std::string>& faces);
unsigned int createShaderProgram();
unsigned int loadTexture(char const *path);

float deltaTime = 0.0;
float lastX = 400, lastY = 300;
bool firstMouse = true;
float screenWidth = 1280, screenHeight = 720;
int g_segments = 100;

Camera camera(glm::vec3(0.0, 0.0, 3.0));
Camera mirrorCamera(glm::vec3(0.0, 0.0, 3.0));


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

	// 确保窗口获得输入焦点。
	// 从 VS Code 终端/调试器启动时窗口可能不会自动聚焦，
	// 此时 GLFW 不会派发鼠标/键盘事件，导致 camera 无法控制。
	glfwFocusWindow(window);

	
	glViewport(0, 0, screenWidth, screenHeight);

	//注册窗口调整大小的回调函数
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	//注册鼠标移动回调函数  
	glfwSetCursorPosCallback(window, mouse_callback);
	//注册鼠标滚动回调函数
	glfwSetScrollCallback(window, scroll_callback);
	//注册鼠标点击回调函数
	// glfwSetMouseButtonCallback(window, mouse_button_callback);

	glEnable(GL_PROGRAM_POINT_SIZE);  //允许点的大小变化
	glEnable(GL_DEPTH_TEST);   //深度
	glDepthFunc(GL_LESS);
	// 在初始化部分添加
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_MULTISAMPLE);  //多重采样
	// glEnable(GL_FRAMEBUFFER_SRGB);

		
	BoxGeometry box(1.0, 1.0, 1.0);
	PlaneGeometry plane(8.0, 8.0, 8, 8);

	unsigned int ourTexture = loadTexture("./texture/container.jpg");
	unsigned int floorTex = loadTexture("./texture/chess.png");


	Shader shader("./src/vertexShader.glsl", "./src/fragmentShader.glsl");
	Shader pointShader("./src/pointVer.glsl", "./src/pointFrag.glsl");

	std::vector<glm::vec2> points{
		{-1.0f, 0.0f}, {-1.0f, 1.0f}, 
		{1.0f, 1.0f}, {1.0f, 0.0f},
		{0.0f, 0.0f}
	};


	GLuint pointsVBO, pointsVAO;
	glGenBuffers(1, &pointsVBO);
	glGenVertexArrays(1, &pointsVAO);
	glBindVertexArray(pointsVAO);
	glBindBuffer(GL_ARRAY_BUFFER, pointsVBO);
	// 控制点固定 4 个，DYNAMIC_DRAW：每帧 SubData 更新
	glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
	glBindVertexArray(0);

	GLuint curveVAO, curveVBO;
	glGenBuffers(1, &curveVBO);
	glGenVertexArrays(1, &curveVAO);
	glBindVertexArray(curveVAO);
	glBindBuffer(GL_ARRAY_BUFFER, curveVBO);
	// 曲线顶点数固定 = g_segments + 1
	glBufferData(GL_ARRAY_BUFFER, (g_segments + 1) * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
	glBindVertexArray(0);


	shader.use();

	float lastTime = 0.0f;

	while(!glfwWindowShouldClose(window))
	{
		float time = glfwGetTime();
		deltaTime = time - lastTime;
		lastTime = time;

		processInput(window);

		glClearColor(1.0, 1.0, 1.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader.use();
		
		shader.setMatrix4f("model", glm::mat4{1.0f});
		shader.setMatrix4f("view", camera.GetViewMatrix());
		shader.setMatrix4f("projection", glm::perspective(glm::radians(camera.Zoom), screenWidth / screenHeight, 0.1f, 100.0f));

		glBindVertexArray(box.VAO);
		glDrawElements(GL_POINTS, box.indices.size(), GL_UNSIGNED_INT, 0);

		// ========== 动态更新控制点：每个点做小幅周期运动 ==========
		{
			float t = glfwGetTime();
			points[0] = glm::vec2(-1.0f + 0.8f * sin(t * 1.0f),  0.0f + 0.3f * cos(t * 1.3f));
			points[1] = glm::vec2(-1.0f + 0.2f * cos(t * 0.7f),  1.0f + 0.2f * sin(t * 1.1f));
			points[2] = glm::vec2( 1.0f + 0.2f * sin(t * 0.8f),  1.0f + 0.3f * cos(t * 0.9f));
			points[3] = glm::vec2( 1.0f + 0.3f * cos(t * 1.3f),  0.0f + 0.2f * sin(t * 0.6f));

			auto curve = bezier_curve(points, g_segments);

			// 更新控制点 VBO
			glBindBuffer(GL_ARRAY_BUFFER, pointsVBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, points.size() * sizeof(glm::vec2), points.data());
			// 更新曲线 VBO
			glBindBuffer(GL_ARRAY_BUFFER, curveVBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, curve.size() * sizeof(glm::vec2), curve.data());
		}

		pointShader.use();
		
		pointShader.setMatrix4f("model", glm::mat4{1.0f});
		pointShader.setMatrix4f("view", camera.GetViewMatrix());
		pointShader.setMatrix4f("projection", glm::perspective(glm::radians(camera.Zoom), screenWidth / screenHeight, 0.1f, 100.0f));


		glBindVertexArray(pointsVAO);
		glDrawArrays(GL_LINE_STRIP, 0, points.size());

		glBindVertexArray(curveVAO);
		glDrawArrays(GL_POINTS, 0, g_segments + 1);

		
		glfwSwapBuffers(window);
		glfwPollEvents(); 
	}

	glDeleteBuffers(1, &pointsVBO);
	glDeleteVertexArrays(1, &pointsVAO);


	glfwTerminate();
	return 0;
}

///////

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
} 

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
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


void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		camera.ProcessKeyboard(UP, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		camera.ProcessKeyboard(DOWN, deltaTime);
}

unsigned int loadTexture(char const *path)
{
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_set_flip_vertically_on_load(true);
	int width, height, nrComponents;
	unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);

	GLenum format;
	if (nrComponents == 1)
		format = GL_RED;
	else if (nrComponents == 3)
		format = GL_RGB;
	else if (nrComponents == 4)
		format = GL_RGBA;

	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "fail to load image" << std::endl;
	}

	stbi_image_free(data);
	
	return texture;
}

unsigned int loadCubeMap(vector<std::string>& faces)
{
	unsigned int cubeMap;
	glGenTextures(1, &cubeMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	stbi_set_flip_vertically_on_load(false);
	int width, height, nrChannels;
	for (int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else
		{
			std::cout << "fail to load image" << std::endl;
		}
		stbi_image_free(data);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return cubeMap;

	
}


