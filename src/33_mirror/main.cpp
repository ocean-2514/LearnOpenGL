#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <tool/shader.h>
#include <iostream>
#include <cmath>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <tool/stb_image.h>

#include <tool/shader.h>
#include <tool/framebuffer.h>
// #include <tool/model.h>
#include <tool/camera.h>
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
glm::mat4 calculateMirrorViewMatrix(Camera& camera, glm::vec3 mirrorPos, glm::vec3 mirrorNormal);
unsigned int createShaderProgram();
unsigned int loadTexture(char const *path);
void renderScene(const Shader& shader, const Camera& camera, const BoxGeometry& box, const PlaneGeometry& plane, unsigned int boxTexture, unsigned int floorTexture);
void renderPlayer(const Shader& shader, const Camera& camera, const SphereGeometry& player);

float deltaTime = 0.0;
float lastX = 400, lastY = 300;
bool firstMouse = true;

Camera mainCamera(glm::vec3(0.0, 0.0, 3.0));
Camera mirrorCamera(glm::vec3(0.0, 0.0, 3.0));

int screenWidth = 800, screenHeight = 600;

int main()
{
	glfwInit();
	//设置主要和次要版本
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

	Camera monitor{glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f};
	
	BoxGeometry box(1.0, 1.0, 1.0);
	PlaneGeometry plane(8.0, 8.0, 8, 8);
	PlaneGeometry screenPlane(4.0, 3.0, 8.0, 8.0);
	SphereGeometry player(0.1);

	unsigned int ourTexture = loadTexture("./texture/container.jpg");
	unsigned int floorTex = loadTexture("./texture/wood.png");

	Framebuffer framebuffer{};
	Framebuffer monitorBuffer{};

	Shader shader("./src/33_mirror/shader/vertexShader.glsl", "./src/33_mirror/shader/fragmentShader.glsl");
	Shader screenShader("./src/33_mirror/shader/screenVer.glsl", "./src/33_mirror/shader/screenFrag.glsl");
	Shader sphereShader("./src/33_mirror/shader/sphereVer.glsl", "./src/33_mirror/shader/sphereFrag.glsl");

	glm::vec3 mirrorPos(0.0, 0.0, -1.0);
	glm::vec3 monitorPos(0.0, 0.0, -3.0);
	float lastTime = 0.0f;

	while(!glfwWindowShouldClose(window))
	{
		float time = glfwGetTime();
		deltaTime = time - lastTime;
		lastTime = time;

		processInput(window);
		
		// 绑定到帧缓冲
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);

		// glClearColor(0.2f, 1.0f, 1.0f, 1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		
		renderScene(shader, mainCamera, box, plane, ourTexture, floorTex);

		// 
		glBindFramebuffer(GL_FRAMEBUFFER, monitorBuffer.fbo);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		renderScene(shader, monitor, box, plane, ourTexture, floorTex);
		renderPlayer(sphereShader, monitor, player);
		
		///返回默认
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(1.0, 1.0, 1.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		
		renderScene(shader, mainCamera, box, plane, ourTexture, floorTex);
		
		
		//画屏幕
		glEnable(GL_DEPTH_TEST);
		screenShader.use();

		glm::mat4 model = glm::mat4();
		model = glm::translate(model, mirrorPos);
		screenShader.setMatrix4f("model", model);
		screenShader.setMatrix4f("view", mainCamera.GetViewMatrix());
		screenShader.setMatrix4f("projection", glm::perspective(glm::radians(mainCamera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f));

		glBindTexture(GL_TEXTURE_2D, framebuffer.colorTexture);
		screenShader.setInt("screenTexture", 0);
		screenShader.setVec2("screenSize", glm::vec2(screenWidth, screenHeight));

		glBindVertexArray(screenPlane.VAO);
		glDrawElements(GL_TRIANGLES, screenPlane.indices.size(), GL_UNSIGNED_INT, 0);

		model = glm::mat4();
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0, 1.0, 0.0));
		model = glm::translate(model, monitorPos);
		screenShader.setMatrix4f("model", model);
		
		glBindTexture(GL_TEXTURE_2D, monitorBuffer.colorTexture);
		glBindVertexArray(screenPlane.VAO);
		glDrawElements(GL_TRIANGLES, screenPlane.indices.size(), GL_UNSIGNED_INT, 0);
		
		
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
	mainCamera.ProcessMouseScroll(yoffset);
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

	mainCamera.ProcessMouseMovement(xoffset, yoffset);
}


void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		mainCamera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		mainCamera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		mainCamera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		mainCamera.ProcessKeyboard(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		mainCamera.ProcessKeyboard(UP, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		mainCamera.ProcessKeyboard(DOWN, deltaTime);
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

glm::mat4 calculateMirrorViewMatrix(Camera& camera, glm::vec3 mirrorPos, glm::vec3 mirrorNormal) 
{
    // 计算反射平面（镜子平面）
    float d = -glm::dot(mirrorNormal, mirrorPos); // 平面方程: normal·x + d = 0
    
    // 创建反射矩阵
    glm::mat4 reflectMatrix;
    reflectMatrix[0][0] = 1 - 2 * mirrorNormal.x * mirrorNormal.x;
    reflectMatrix[0][1] = -2 * mirrorNormal.x * mirrorNormal.y;
    reflectMatrix[0][2] = -2 * mirrorNormal.x * mirrorNormal.z;
    reflectMatrix[0][3] = -2 * mirrorNormal.x * d;
    
    reflectMatrix[1][0] = -2 * mirrorNormal.x * mirrorNormal.y;
    reflectMatrix[1][1] = 1 - 2 * mirrorNormal.y * mirrorNormal.y;
    reflectMatrix[1][2] = -2 * mirrorNormal.y * mirrorNormal.z;
    reflectMatrix[1][3] = -2 * mirrorNormal.y * d;
    
    reflectMatrix[2][0] = -2 * mirrorNormal.x * mirrorNormal.z;
    reflectMatrix[2][1] = -2 * mirrorNormal.y * mirrorNormal.z;
    reflectMatrix[2][2] = 1 - 2 * mirrorNormal.z * mirrorNormal.z;
    reflectMatrix[2][3] = -2 * mirrorNormal.z * d;
    
    reflectMatrix[3][3] = 1;
    
    // 应用反射矩阵到视图矩阵
    glm::mat4 originalView = camera.GetViewMatrix();
    return reflectMatrix * originalView;
}

void renderScene(const Shader& shader, const Camera& camera, const BoxGeometry& box, const PlaneGeometry& plane, unsigned int boxTexture, unsigned int floorTexture) {
	shader.use();

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(1.0, 0.0, 1.0));
	glm::mat4 normalMat = glm::transpose(glm::inverse(model));
	shader.setMatrix4f("model", model);
	shader.setMatrix3f("normalMatrix", normalMat);
	shader.setMatrix4f("view", camera.GetViewMatrix());
	shader.setMatrix4f("projection", glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f));
	shader.setInt("material.diffuse", 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, boxTexture);
	glBindVertexArray(box.VAO);
	glDrawElements(GL_TRIANGLES, box.indices.size(), GL_UNSIGNED_INT, 0);	//box

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0, -0.51, 0.0));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0));
	normalMat = glm::transpose(glm::inverse(model));
	shader.setMatrix4f("model", model);
	shader.setMatrix3f("normalMatrix", normalMat);

	glBindTexture(GL_TEXTURE_2D, floorTexture);
	glBindVertexArray(plane.VAO);
	glDrawElements(GL_TRIANGLES, plane.indices.size(), GL_UNSIGNED_INT, 0);	//plane
}

void renderPlayer(const Shader& shader, const Camera& camera, const SphereGeometry& player) {
	shader.use();

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, mainCamera.Position);
	shader.setMatrix4f("model", model);
	shader.setMatrix4f("view", camera.GetViewMatrix());
	shader.setMatrix4f("projection", glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f));

	glBindVertexArray(player.VAO);
	glDrawElements(GL_TRIANGLES, player.indices.size(), GL_UNSIGNED_INT, 0);	//player
}