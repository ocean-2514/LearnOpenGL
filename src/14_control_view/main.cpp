#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <tool/shader.h>
#include <iostream>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <geometry/PlaneGeometry.h>
#include <geometry/BoxGeometry.h>
#include <geometry/SphereGeometry.h>

#define STB_IMAGE_IMPLEMENTATION
#include <tool/stb_image.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>     
#include <imgui/imgui_impl_opengl3.h>     

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void processInput(GLFWwindow *window);
unsigned int createShaderProgram();

glm::vec3 cameraPos = glm::vec3(0.0, 0.0, 3.0);
glm::vec3 cameraFront = glm::vec3(0.0, 0.0, -1.0);
glm::vec3 cameraUp = glm::vec3(0.0, 1.0, 0.0);
float deltatime = 1.0;
// mouse
float lastX = 400, lastY = 300;
float pitch = 0, yaw = -90;
bool firstMouse = true;
float fov = 45.0;
bool mouseLeftPressed = false;
bool mouseRightPressed = false;

int main()
{
	glfwInit();
	//设置主要和次要版本
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

	//创建imgui上下文
	IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
	// Setup Dear ImGui style，风格
    ImGui::StyleColorsDark();
	// Setup Platform/Renderer backends,设置平台和渲染器
    ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	float screenWidth = 800, screenHeight = 600;
	glViewport(0, 0, 800, 600);

	//注册窗口调整大小的回调函数
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	//注册鼠标移动回调函数  
	glfwSetCursorPosCallback(window, mouse_callback);
	//注册鼠标滚动回调函数
	glfwSetScrollCallback(window, scroll_callback);
	//注册鼠标点击回调函数
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	glEnable(GL_PROGRAM_POINT_SIZE);  //允许点的大小变化
	glEnable(GL_DEPTH_TEST);   //深度
	glDepthFunc(GL_LESS);

	PlaneGeometry plane(1.0f, 1.0f, 8, 8);  
	BoxGeometry box(1.0f, 0.5f, 0.5f, 5, 2, 2);
	SphereGeometry sphere(0.5f);
    
	Shader shader("./src/14_control_view/shader/vertexShader.glsl", "./src/14_control_view/shader/fragmentShader.glsl");


	//绑定纹理 
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	stbi_set_flip_vertically_on_load(true);
	int width, height, nrChannel;
	unsigned char *data = stbi_load("./texture/container.jpg", &width, &height, &nrChannel, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	stbi_image_free(data);

	
	float lastTime = 0.0;
	// ImVec4 clear_color = ImVec4(0.1, 0.1, 0.1, 1.0);
	while(!glfwWindowShouldClose(window))
	{
		float currentTime = glfwGetTime();
		deltatime = currentTime - lastTime;
		lastTime = currentTime;

		// std::cout << deltatime << " " << std::endl;

		processInput(window);
		
		glClearColor(0.2f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		
		glm::mat4 model = glm::mat4();
		model = glm::rotate(model, glm::radians(currentTime * 30.0f), glm::vec3(1.0, 1.0, 1.0));
		
		glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
		glm::mat4 projection = glm::perspective(fov, screenWidth / screenHeight, 0.1f, 100.0f);
		
		shader.use();
		shader.setMatrix4f("model", model);
		shader.setMatrix4f("view", view);
		shader.setMatrix4f("projection", projection);
		shader.setInt("ourTexture", 0);

		
		glBindVertexArray(box.VAO);
		glad_glDrawElements(GL_POINTS, box.indices.size(), GL_UNSIGNED_INT, 0);
		glad_glDrawElements(GL_TRIANGLES, box.indices.size(), GL_UNSIGNED_INT, 0);
	
		
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

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.05f;
	pitch += yoffset * sensitivity;
	yaw += xoffset * sensitivity;

	if (pitch > 89.0) pitch = 89.0;
	if (pitch < -89.0) pitch = -89.0;

	std::cout << pitch << " " << yaw << std::endl;
	
	float x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
	float y = sin(glm::radians(pitch));
	float z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
	
	std::cout << x << " " << y << " " << z << std::endl;

	cameraFront = glm::normalize(glm::vec3(x, y, z));
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	if(fov >= 1.0f && fov <= 45.0f)
    	fov -= yoffset * 0.05;
  	if(fov <= 1.0f)
    	fov = 1.0f;
  	if(fov >= 45.0f)
    	fov = 45.0f;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT)
		mouseLeftPressed = (action == GLFW_PRESS);
	if (button == GLFW_MOUSE_BUTTON_RIGHT)
		mouseRightPressed = (action == GLFW_PRESS);
}


void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	glm::vec3 horizontalFront = glm::normalize(glm::vec3(cameraFront.x, 0.0, cameraFront.z));
	
	float step = deltatime * 2.5f;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) 
		cameraPos += step * horizontalFront;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cameraPos -= step * horizontalFront;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cameraPos += glm::normalize(glm::cross(cameraUp, horizontalFront)) * step;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cameraPos -= glm::normalize(glm::cross(cameraUp, horizontalFront)) * step;

	if (mouseLeftPressed)
        cameraPos += cameraUp * step;
    if (mouseRightPressed)
        cameraPos -= cameraUp * step;
}
