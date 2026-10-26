#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <tool/shader.h>
#include <iostream>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <tool/shader.h>
#include <tool/model.h>
#include <tool/camera.h>
// #include <imgui/imgui.h>
// #include <imgui/imgui_impl_glfw.h>     
// #include <imgui/imgui_impl_opengl3.h>     

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
// void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void processInput(GLFWwindow *window, Model& model);
unsigned int createShaderProgram();
unsigned int loadTexture(char const *path);

float deltaTime = 0.0;
float lastX = 400, lastY = 300;
bool firstMouse = true;
bool spacePressed = true;

unsigned int displayBoneId = 0;

Camera camera(glm::vec3(0.0, 0.0, 5.0));

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

	Model ourModel("./model/walking/Walking.fbx");
	// Model backpack("./model/backpack/backpack.obj");
	Shader shader("./src/46_animation/shader/vertexShader.glsl", 
		"./src/46_animation/shader/fragmentShader.glsl");
	
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

	float lastTime = 0.0f;

	while(!glfwWindowShouldClose(window))
	{
		float time = glfwGetTime();
		deltaTime = time - lastTime;
		lastTime = time;

		processInput(window, ourModel);
		
		// glClearColor(0.2f, 1.0f, 1.0f, 1.0f);
		glClearColor(0.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		
		shader.use();
 
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), screenWidth / screenHeight, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMatrix4f("projection", projection);
        shader.setMatrix4f("view", view);

        // render the loaded model
        glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, -1.8f, 0.0f));
        model = glm::scale(model, glm::vec3(0.02f, 0.02f, 0.02f));	
		shader.setMatrix4f("model", model);

		shader.setInt("displayBoneIndex", displayBoneId); 

		vector<glm::mat4> transforms;
		ourModel.GetBoneTransforms(time, transforms); 

		for (int i = 0; i < transforms.size(); ++i) {
			shader.setMatrix4f("gBones[" + to_string(i) + "]", transforms[i]);
		}
 
		shader.setInt("hasAnim", 1);
		ourModel.Draw(shader);
		
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


void processInput(GLFWwindow *window, Model& model)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (!spacePressed && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		spacePressed = true;
		if (++displayBoneId >= model.boneCount) {
			displayBoneId = 0;
		}
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
		spacePressed = false;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		camera.ProcessKeyboard(UP, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
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