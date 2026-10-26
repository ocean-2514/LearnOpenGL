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
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
// void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void processInput(GLFWwindow *window);
unsigned int createShaderProgram();
unsigned int loadTexture(char const *path);

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

	glm::vec3 cubePositions[] = {
		glm::vec3( 0.0f,  0.0f,  0.0f),
		glm::vec3( 2.0f,  5.0f, -15.0f),
		glm::vec3(-1.5f, -2.2f, -2.5f),
		glm::vec3(-3.8f, -2.0f, -12.3f),
		glm::vec3( 2.4f, -0.4f, -3.5f),
		glm::vec3(-1.7f,  3.0f, -7.5f),
		glm::vec3( 1.3f, -2.0f, -2.5f),
		glm::vec3( 1.5f,  2.0f, -2.5f),
		glm::vec3( 1.5f,  0.2f, -1.5f),
		glm::vec3(-1.3f,  1.0f, -1.5f)
	};

	PlaneGeometry plane(1.0f, 1.0f, 8, 8);  
	BoxGeometry box(0.5f, 0.5f, 0.5f, 5, 2, 2);
	SphereGeometry sphere(0.1f, 10.0f, 10.0f);
    
	Shader shader("./src/21_direction_light/shader/vertexShader.glsl", "./src/21_direction_light/shader/fragmentShader.glsl");
	Shader lightShader("./src/21_direction_light/shader/lightVer.glsl", "./src/21_direction_light/shader/lightFrag.glsl");


	unsigned int diffuseMap = loadTexture("./texture/container2.png");
	unsigned int specularMap = loadTexture("./texture/container2_specular.png");
	

	
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
		glBindTexture(GL_TEXTURE_2D, diffuseMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, specularMap);

		lightPos = glm::vec3(2 * cos(time), 1.0, 2 * sin(time));
		
		// glm::mat4 model = glm::mat4();
		// model = glm::rotate(model, glm::radians(time * 45.0f), glm::vec3(1.0, 1.0, 1.0));
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), screenWidth / screenHeight, 0.1f, 100.0f);
		
		shader.use();
		shader.setMatrix4f("view", view);
		shader.setMatrix4f("projection", projection);
		shader.setInt("ourTexture", 0);
		shader.setVec3("viewPos", camera.Position);
		shader.setVec3("objectColor", objectColor);
		shader.setInt("material.diffuse",  0);
		shader.setInt("material.specular", 1);
		shader.setFloat("material.shininess", 32.0f);
		shader.setVec3("light.ambient",  0.5f, 0.5f, 0.5f);
		shader.setVec3("light.diffuse",  1.0f, 1.0f, 1.0f); 
		shader.setVec3("light.specular", 1.0f, 1.0f, 1.0f); 
		shader.setVec3("light.direction", -0.2f, -1.0f, -0.3f); 
		
		glBindVertexArray(box.VAO);
		for(unsigned int i = 0; i < 10; i++)
		{
			glm::mat4 model;
			model = glm::translate(model, cubePositions[i]);
			float angle = 20.0f * i;
			model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
			shader.setMatrix4f("model", model);
			glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
			shader.setMatrix3f("normalMatrix", normalMatrix);
			
			glad_glDrawElements(GL_TRIANGLES, box.indices.size(), GL_UNSIGNED_INT, 0);
		}
		glBindVertexArray(0);
		
		//光源：
		// glm::mat4 lightmodel = glm::mat4();
		// lightmodel = glm::translate(lightmodel, lightPos);

		// lightShader.use();
		// lightShader.setVec3("lightColor", lightColor);
		// lightShader.setMatrix4f("model", lightmodel);
		// lightShader.setMatrix4f("view", view);
		// lightShader.setMatrix4f("projection", projection);

		// glBindVertexArray(sphere.VAO);
		// glad_glDrawElements(GL_TRIANGLES, sphere.indices.size(), GL_UNSIGNED_INT, 0);
	
		
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