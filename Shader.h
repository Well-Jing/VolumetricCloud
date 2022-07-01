#pragma once
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
    unsigned int ID;
    // constructor generates the shader on the fly
    Shader(const char* vertexPath, const char* fragmentPath);

    // activate the shader
    void use();

    // utility uniform functions
    void setUniformBool(const std::string& name, bool value) const;
    void setUniform1i(const std::string& name, int value) const;
    void setUniform1f(const std::string& name, float value) const;
    void setUniform2f(const std::string& name, glm::vec2 value) const;
    void setUniform3f(const std::string& name, glm::vec3 value) const;
    void setUniformMatrix(const std::string& name, glm::mat4 value) const;

private:
    // utility function for checking shader compilation/linking errors.
    void checkCompileErrors(unsigned int shader, std::string type);

};

