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
	GLFWwindow *window = glfwCreateWindow(800, 600, "window", NULL, NULL);
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

	float screenWidth = 800, screenHeight = 600;
	glViewport(0, 0, 800, 600);

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
	unsigned int floorTex = loadTexture("./texture/wood.png");

	// 对比演示：5 个立方体各用一张纹理，便于区分不同的插值方法
	unsigned int cubeTex[5];
	const char* cubeTexPaths[5] = {
		"./texture/container.jpg",     // 1) 欧拉角 LERP
		"./texture/brickwall.jpg",     // 2) 四元数 LERP
		"./texture/matrix.jpg",        // 3) NLERP
		"./texture/container2.png",    // 4) SLERP
		"./texture/awesomeface.png"    // 
	};
	for (int i = 0; i < 5; i++)
		cubeTex[i] = loadTexture(cubeTexPaths[i]);


	Shader shader("./src/49_interpolation/shader/vertexShader.glsl", "./src/49_interpolation/shader/fragmentShader.glsl");

	glm::mat4 initialRotation = glm::mat4(1.0f);
	initialRotation = glm::rotate(initialRotation, glm::radians(-145.0f), glm::vec3(1.0, 0.0, 0.0));
	glm::mat4 finalRotation = glm::mat4(1.0f);
	finalRotation = glm::rotate(finalRotation, glm::radians(30.0f), glm::vec3(0.0, 1.0, 1.0));
	glm::mat4 initialModel = glm::mat4(1.0f);
	initialModel = glm::translate(initialModel, glm::vec3(1.0f, 1.0f, 1.0f));
	initialModel = glm::scale(initialModel, glm::vec3(.5f, .5f, .5f));
	glm::mat4 finalModel = finalRotation;



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

		shader.setMatrix4f("view", camera.GetViewMatrix());
		shader.setMatrix4f("projection", glm::perspective(glm::radians(camera.Zoom), screenWidth / screenHeight, 0.1f, 100.0f));
		shader.setInt("texture1", 0);

		// ================= 旋转插值对比演示 =================
		glm::quat q0(initialRotation);   // 起始旋转：绕 X 轴 -45°
		glm::quat q1(finalRotation);     // 结束旋转：绕 Z 轴 +30°
		q0 = glm::normalize(q0);
		q1 = glm::normalize(q1);

		// t 在 0..1 之间线性往复（三角波），便于观察正/反两向插值差异
		float cycle = fmod(glfwGetTime() * 0.5f, 2.0f);
		float t = 1.0f - fabs(cycle - 1.0f);

		glm::mat4 eulerMat = glm::mat4(glm::quat(glm::eulerAngles(q0) * (1 - t) 
			+ glm::eulerAngles(q1) * t));
		glm::mat4 lerpMat = glm::mat4(glm::normalize(q0 * (1 - t) + q1 * t));
		glm::mat4 nlerpMat = glm::mat4(glm::mix(q0, q1, t));
		glm::mat4 slerpMat = glm::mat4(glm::slerp(q0, q1, t));
		glm::mat4 axisMat = mat_lerp(initialModel, finalModel, t);

		glm::mat4 rotations[5] = { eulerMat, lerpMat, nlerpMat, slerpMat, axisMat };
		float xs[5] = { -1.6f, -0.8f, 0.0f, 0.8f, 1.6f };

		for (int i = 0; i < 5; i++)
		{
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(xs[i], 0.4f, 0.0f));
			model = model * rotations[i];
			shader.setMatrix4f("model", model);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, cubeTex[i]);
			glBindVertexArray(box.VAO);
			glDrawElements(GL_TRIANGLES, box.indices.size(), GL_UNSIGNED_INT, 0);
		}

		// 窗口标题实时显示当前的插值参数 t，方便对照动画阶段
		char title[64];
		snprintf(title, sizeof(title), "Rotation Interp Demo   t = %.2f", t);
		glfwSetWindowTitle(window, title);

		// 地面
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0, -1, 0.0));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0));
		shader.setMatrix4f("model", model);
		glBindTexture(GL_TEXTURE_2D, floorTex);

		glBindVertexArray(plane.VAO);
		glDrawElements(GL_TRIANGLES, plane.indices.size(), GL_UNSIGNED_INT, 0);

		
		glfwSwapBuffers(window);
		glfwPollEvents(); 
	}


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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
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
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return cubeMap;

	
}


