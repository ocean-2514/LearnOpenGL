#ifndef MY_LIGHT_H
#define MY_LIGHT_H

#include <glm/glm.hpp>
#include <tool/shader.h>
#include <string>

class Light {
public:
    glm::vec3 ambient = glm::vec3(0.1f);
    glm::vec3 diffuse = glm::vec3(1.0f);
    glm::vec3 specular = glm::vec3(1.0f);

    Light() {}
    Light(const glm::vec3 &ambient, const glm::vec3 &diffuse, const glm::vec3 &specular) 
        : ambient(ambient), diffuse(diffuse), specular(specular) {}

    virtual void setShaderLight(const std::string& name, const Shader& shader) {
        shader.use();
        shader.setVec3(name + ".ambient", ambient);
        shader.setVec3(name + ".diffuse", diffuse);
        shader.setVec3(name + ".specular", specular);
    }
    virtual ~Light() {}
};

class DirLight : public Light {
public:
    glm::vec3 direction = glm::vec3(-0.2f, -1.0f, 0.3f);

    DirLight() {}
    DirLight(const glm::vec3 &direction, const glm::vec3 &ambient = glm::vec3(0.2f), const glm::vec3 &diffuse = glm::vec3(1.0f), const glm::vec3 &specular = glm::vec3(1.0f)) 
        : Light(ambient, diffuse, specular), direction(direction) {}

    void setShaderLight(const std::string& name, const Shader& shader) override {
        Light::setShaderLight(name, shader);
        shader.setVec3(name + ".direction", direction);
    }
};

class PointLight : public Light {
public:
    glm::vec3 position = glm::vec3(0.0f);
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;

    PointLight() {}
    PointLight(const glm::vec3 &position, const glm::vec3 &ambient = glm::vec3(0.2f), const glm::vec3 &diffuse = glm::vec3(1.0f), const glm::vec3 &specular = glm::vec3(1.0f)) 
        : Light(ambient, diffuse, specular), position(position) {}

    void setShaderLight(const std::string& name, const Shader& shader) override {
        Light::setShaderLight(name, shader);
        shader.setVec3(name + ".position", position);
        shader.setFloat(name + ".constant", constant);
        shader.setFloat(name + ".linear", linear);
        shader.setFloat(name + ".quadratic", quadratic);
    }
};

class SpotLight : public Light {
public:
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 direction = glm::vec3(0.0f, 0.0f, -1.0f);
    float cutOff = glm::cos(glm::radians(12.5f));
    float outerCutOff = glm::cos(glm::radians(15.0f));
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;

    SpotLight() {}
    SpotLight(const glm::vec3 &position, const glm::vec3 &direction, const glm::vec3 &ambient = glm::vec3(0.2f), const glm::vec3 &diffuse = glm::vec3(1.0f), const glm::vec3 &specular = glm::vec3(1.0f)) 
        : Light(ambient, diffuse, specular), position(position), direction(direction) {}

    void setShaderLight(const std::string& name, const Shader& shader) override {
        Light::setShaderLight(name, shader);
        shader.setVec3(name + ".position", position);
        shader.setVec3(name + ".direction", direction);
        shader.setFloat(name + ".cutOff", cutOff);
        shader.setFloat(name + ".outerCutOff", outerCutOff);
        shader.setFloat(name + ".constant", constant);
        shader.setFloat(name + ".linear", linear);
        shader.setFloat(name + ".quadratic", quadratic);
    }
};


#endif // MY_LIGHT_H