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
#include <tool/camera.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>     
#include <imgui/imgui_impl_opengl3.h>     

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
// void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
// void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void processInput(GLFWwindow *window);
unsigned int createShaderProgram();

float deltaTime = 0.0;
float lastX = 400, lastY = 300;
bool firstMouse = true;

Camera camera(glm::vec3(0.0, 0.0, 3.0));

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

	// glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);



	float screenWidth = 800, screenHeight = 600;
	glViewport(0, 0, 800, 600);

	//注册窗口调整大小的回调函数
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	//注册鼠标移动回调函数  
	// glfwSetCursorPosCallback(window, mouse_callback);
	//注册鼠标滚动回调函数
	glfwSetScrollCallback(window, scroll_callback);
	//注册鼠标点击回调函数
	// glfwSetMouseButtonCallback(window, mouse_button_callback);

	glEnable(GL_PROGRAM_POINT_SIZE);  //允许点的大小变化
	glEnable(GL_DEPTH_TEST);   //深度
	glDepthFunc(GL_LESS);

	PlaneGeometry plane(1.0f, 1.0f, 8, 8);  
	BoxGeometry box(0.5f, 0.5f, 0.5f, 5, 2, 2);
	SphereGeometry sphere(0.1f, 10.0f, 10.0f);
    
	Shader shader("./src/18_materia/shader/vertexShader.glsl", "./src/18_materia/shader/fragmentShader.glsl");
	Shader lightShader("./src/18_materia/shader/lightVer.glsl", "./src/18_materia/shader/lightFrag.glsl");


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

	
	glm::vec3 objectColor(1.0f, 0.5f, 0.31f);
	glm::vec3 lightColor(1.0, 1.0, 1.0);
	glm::vec3 lightPos(2.0, 1.0, 0.0);

	float lastTime = 0.0f;

	while(!glfwWindowShouldClose(window))
	{
		float time = glfwGetTime();
		deltaTime = time - lastTime;
		lastTime = time;

		processInput(window);
		
		glClearColor(0.2f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);

		lightPos = glm::vec3(2 * cos(time), 1.0, 2 * sin(time));
		
		glm::mat4 model = glm::mat4();
		model = glm::rotate(model, glm::radians(time * 45.0f), glm::vec3(1.0, 1.0, 1.0));
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), screenWidth / screenHeight, 0.1f, 100.0f);
		glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
		
		shader.use();
		shader.setMatrix4f("model", model);
		shader.setMatrix4f("view", view);
		shader.setMatrix4f("projection", projection);
		shader.setMatrix3f("normalMatrix", normalMatrix);
		shader.setInt("ourTexture", 0);
		shader.setVec3("lightColor", lightColor);
		shader.setVec3("lightPos", lightPos);
		shader.setVec3("viewPos", camera.Position);
		shader.setVec3("objectColor", objectColor);
		shader.setVec3("material.ambient",  1.0f, 0.5f, 0.31f);
		shader.setVec3("material.diffuse",  1.0f, 0.5f, 0.31f);
		shader.setVec3("material.specular", 0.5f, 0.5f, 0.5f);
		shader.setFloat("material.shininess", 32.0f);
		shader.setVec3("light.ambient",  0.2f, 0.2f, 0.2f);
		shader.setVec3("light.diffuse",  0.5f, 0.5f, 0.5f); // 将光照调暗了一些以搭配场景
		shader.setVec3("light.specular", 1.0f, 1.0f, 1.0f); 
		
		glBindVertexArray(box.VAO);
		glad_glDrawElements(GL_POINTS, box.indices.size(), GL_UNSIGNED_INT, 0);
		glad_glDrawElements(GL_TRIANGLES, box.indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		
		//光源：
		glm::mat4 lightmodel = glm::mat4();
		lightmodel = glm::translate(lightmodel, lightPos);

		lightShader.use();
		lightShader.setVec3("lightColor", lightColor);
		lightShader.setMatrix4f("model", lightmodel);
		lightShader.setMatrix4f("view", view);
		lightShader.setMatrix4f("projection", projection);

		glBindVertexArray(sphere.VAO);
		glad_glDrawElements(GL_TRIANGLES, sphere.indices.size(), GL_UNSIGNED_INT, 0);
	
		
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

// void mouse_callback(GLFWwindow* window, double xpos, double ypos)
// {
// 	if (firstMouse)
// 	{
// 		firstMouse = false;
// 		lastX = xpos;
// 		lastY = ypos;
// 	}

// 	float xoffset = xpos - lastX;
// 	float yoffset = lastY - ypos;
// 	lastX = xpos;
// 	lastY = ypos;

// 	camera.ProcessMouseMovement(xoffset, yoffset);
// }


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
}
