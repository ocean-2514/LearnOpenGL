#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <tool/shader.h>
#include <iostream>
#include <cmath>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// #define STB_IMAGE_IMPLEMENTATION
// #include <tool/stb_image.h>

#include <tool/shader.h>
#include <tool/model.h>
#include <tool/camera.h>
// #include <geometry/BoxGeometry.h>
// #include <geometry/PlaneGeometry.h>
// #include <geometry/SphereGeometry.h>
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

	glm::vec3 pointLightPositions[] = {
        glm::vec3( 0.7f,  0.2f,  2.0f),
        glm::vec3( 2.3f, -3.3f, -4.0f),
        glm::vec3(-4.0f,  2.0f, -12.0f),
        glm::vec3( 0.0f,  0.0f, -3.0f)
    };
	
	//instanceVBO数据
	int amount = 100;
	std::vector<glm::mat4> modelMatrices;
	srand(static_cast<unsigned int>(time(NULL)));
	for (int i = 0; i < amount; i++) 
	{
		//scale
		float scale = (float)(rand() % 100) / 1000.0f + 0.05f; //0.05~0.15
		//translate
		float angle = (float)(rand() % 3600) / 10.0f;
		float radius = (float)(rand() % 100) / 100.0f + 4.0f; //4~5
		float translateX = radius * cos(glm::radians(angle));
		float translateZ = radius * sin(glm::radians(angle));
		float translateY = (float)(rand() % 200 - 100) / 200.0f;
		//rotate
		glm::vec3 rotateAxis = glm::normalize(glm::vec3(
			(float)(rand() % 20 - 10) / 10.0f,
			(float)(rand() % 20 - 10) / 10.0f,
			(float)(rand() % 20 - 10) / 10.0f
		));
		float theta = (float)(rand() % 3600) / 10.0f;

		glm::mat4 model = glm::mat4();
		model = glm::translate(model, glm::vec3(translateX, translateY, translateZ));
		model = glm::rotate(model, glm::radians(theta), rotateAxis);
		model = glm::scale(model, glm::vec3(scale, scale, scale));

		modelMatrices.push_back(model);
	}

	
	// BoxGeometry box(1.0, 1.0, 1.0, 1, 1, 1);
	// BoxGeometry skyBox(2.0, 2.0, 2.0);

	unsigned int planetTex = loadTexture("./model/planet/planet_Quom1200.png");
	unsigned int rockTex = loadTexture("./model/rock/Rock-Texture-Surface.jpg");
	Model planet("./model/planet/planet.obj");
	Model rock("./model/rock/rock.obj");
	Mesh rockMesh = rock.meshes[0];

	unsigned int instanceModelVBO;
	glGenBuffers(1, &instanceModelVBO);
	glBindVertexArray(rockMesh.VAO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceModelVBO);
	glBufferData(GL_ARRAY_BUFFER, amount * sizeof(glm::mat4), &modelMatrices[0], GL_STATIC_DRAW);
	for (int i = 0; i < 4; i++)
	{
		glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * sizeof(glm::vec4)));
		glEnableVertexAttribArray(3 + i);
		glVertexAttribDivisor(3 + i, 1);
	}
	
	glBindVertexArray(0);


	Shader shader("./src/45_planet/shader/vertexShader.glsl", "./src/45_planet/shader/fragmentShader.glsl");
	Shader rockShader("./src/45_planet/shader/rockVer.glsl", "./src/45_planet/shader/rockFrag.glsl");

	shader.use();
	// directional light
	shader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
	shader.setVec3("dirLight.ambient", 0.2f, 0.2f,0.2f);
	shader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
	shader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);
	// point light 1
	shader.setVec3("pointLights[0].position", pointLightPositions[0]);
	shader.setVec3("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
	shader.setVec3("pointLights[0].diffuse", 0.8f, 0.8f, 0.8f);
	shader.setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
	shader.setFloat("pointLights[0].constant", 1.0f);
	shader.setFloat("pointLights[0].linear", 0.09f);
	shader.setFloat("pointLights[0].quadratic", 0.032f);
	
	rockShader.use();
	// directional light
	rockShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
	rockShader.setVec3("dirLight.ambient", 0.5f, 0.5f,0.5f);
	rockShader.setVec3("dirLight.diffuse", 0.5f, 0.5f, 0.5f);
	rockShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);
	
	

	float lastTime = 0.0f;

	while(!glfwWindowShouldClose(window))
	{
		float time = glfwGetTime();
		deltaTime = time - lastTime;
		lastTime = time;

		processInput(window);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		
		shader.use();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), screenWidth / screenHeight, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		shader.setMatrix4f("projection", projection);
        shader.setMatrix4f("view", view);
		shader.setVec3("viewPos", camera.Position);
		
		glm::mat4 model = glm::mat4();
		model = glm::translate(model, glm::vec3(0.0, 0.0, 0.0));
		model = glm::scale(model, glm::vec3(0.5, 0.5, 0.5));
		shader.setMatrix4f("model", model);
		glm::mat4 normalMatrix = glm::transpose(glm::inverse(model));
		shader.setMatrix3f("normalMatrix", normalMatrix);
		
		// spotLight
		shader.setVec3("spotLight.position", camera.Position);
		shader.setVec3("spotLight.direction", camera.Front);
		shader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
		shader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
		shader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
		shader.setFloat("spotLight.constant", 1.0f);
		shader.setFloat("spotLight.linear", 0.09f);
		shader.setFloat("spotLight.quadratic", 0.032f);
		shader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
		shader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));
		
		planet.Draw(shader);
		
		
		rockShader.use();
		
		glm::mat4 rotateModel = glm::mat4();
		rotateModel = glm::rotate(rotateModel, glm::radians(time * 20.0f), glm::vec3(0.0, 1.0, 0.0)); 
		
		rockShader.setMatrix4f("view", view);
		rockShader.setMatrix4f("projection", projection);
		rockShader.setMatrix4f("rotate", rotateModel);
		rockShader.setVec3("viewPos", camera.Position);
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, rockTex);
		rockShader.setInt("texture_diffuse1", 0);
		rockShader.setInt("texture_specular1", 0);
		
		rockShader.setVec3("spotLight.position", camera.Position);
		rockShader.setVec3("spotLight.direction", camera.Front);
		rockShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
		rockShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
		rockShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
		rockShader.setFloat("spotLight.constant", 1.0f);
		rockShader.setFloat("spotLight.linear", 0.09f);
		rockShader.setFloat("spotLight.quadratic", 0.032f);
		rockShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
		rockShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));

		glBindVertexArray(rockMesh.VAO);
		glDrawElementsInstanced(GL_TRIANGLES, rockMesh.indices.size(), GL_UNSIGNED_INT, 0, amount);
		
		
		
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