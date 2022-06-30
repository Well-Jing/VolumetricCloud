#include <iostream>
#include <stdlib.h> 
#include <string>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Shader.h"
#include "Camera.h"

void keyCallBack(GLFWwindow* window, int key, int scancode, int action, int mode);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void doMovement();
unsigned int loadTexture2D(const std::string& path, bool gamma = true);
unsigned int loadTexture3D(const std::string& path, float width, float height, float depth, bool gamma = true);

#pragma region Camera Creation and Attribution
// Camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
bool keys[1024];
GLfloat lastX = 400, lastY = 300;
bool firstMouse = true;
glm::mat4 MVPM;
glm::mat4 LFMVPM;
#pragma endregion

#pragma region Time Attribution
//measuring time
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;
GLuint frames = 0;
GLfloat timePassed = 0.0f;
GLfloat startTime = 0.0f;
#pragma endregion

#pragma region Window Attribution
// Window dimensions
//const GLuint WIDTH = 1280, HEIGHT = 720;
const GLuint WIDTH = 512 * 2.0, HEIGHT = 512 * 2.0; // if the window is not square, some antefacts show up
const GLuint downscale = 4; //4 is best//any more and the gains dont make up for the lag
GLuint downscalesq = downscale * downscale;
GLfloat ASPECT = float(WIDTH) / float(HEIGHT);

#pragma endregion

// The MAIN function, from here we start the application and run the game loop
int main()
{
    
#pragma region Create Window
    glfwInit();
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Realtime Clouds", NULL, NULL);
    glfwMakeContextCurrent(window);

    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallBack);

    glfwSetWindowPos(window, 200, 17);//so you can see frame rate
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSwapInterval(0);//turn off vsync

    //GLEW init
    glewExperimental = GL_TRUE;
    glewInit();

    //not sure this is necessary?
    glViewport(0, 0, WIDTH, HEIGHT);
#pragma endregion
    
#pragma region Create Shader
    //Shader class built on the one in learnopengl.com
    Shader ourShader("sky.vert", "sky.frag");
    Shader postShader("tex.vert", "tex.frag");
    Shader upscaleShader("upscale.vert", "upscale.frag");
#pragma endregion

#pragma region Triagnle Vertices
    GLfloat vertices[] = {
       -1.0f, -1.0f,
       -1.0f,  3.0f,
        3.0f, -1.0f,
    };
#pragma endregion

#pragma region Buffer Creation
    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);


    //our main full size framebuffer
    GLuint fbo, fbotex;

    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &fbotex);
    glBindTexture(GL_TEXTURE_2D, fbotex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbotex, 0);


    //our secondary full size framebuffer for copying and reading from the main framebuffer
    GLuint copyfbo, copyfbotex;

    glGenFramebuffers(1, &copyfbo);
    glGenTextures(1, &copyfbotex);
    glBindTexture(GL_TEXTURE_2D, copyfbotex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, copyfbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, copyfbotex, 0);


    //our downscaled buffer that we actually render to
    GLuint subbuffer, subbuffertex;

    glGenFramebuffers(1, &subbuffer);
    glGenTextures(1, &subbuffertex);
    glBindTexture(GL_TEXTURE_2D, subbuffertex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH / downscale, HEIGHT / downscale, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, subbuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, subbuffertex, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion
    
#pragma region Load Noise Texture
    //setup noise textures
    GLuint curltex, worltex, perlworltex, weathertex;

    //stbi_set_flip_vertically_on_load(true);
    curltex = loadTexture2D("assets/curlnoise.png");
    weathertex = loadTexture2D("assets/weather.bmp");
    worltex = loadTexture3D("assets/worlnoise.bmp", 32, 32, 32);
    perlworltex = loadTexture3D("assets/perlworlnoise.tga", 128, 128, 128);
#pragma endregion 

#pragma region MVP Matrix Declaration
    //set up Model-View-Projection matrix
    //this way you only update when camera moves
    glm::mat4 view;
    view = camera.getViewMatrix();
    glm::mat4 projection;
    projection = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f);
    MVPM = projection * view;
#pragma endregion

#pragma region Uniform ID
    //setup shader uniform info
    ourShader.use();
    GLuint uniformMatrix =   glGetUniformLocation(ourShader.ID, "MVPM");
    GLuint aspectUniform =   glGetUniformLocation(ourShader.ID, "aspect");
    GLuint checku =          glGetUniformLocation(ourShader.ID, "check");
    GLuint timeu =           glGetUniformLocation(ourShader.ID, "time");
    GLuint resolutionu =     glGetUniformLocation(ourShader.ID, "resolution");
    GLuint downscaleu =      glGetUniformLocation(ourShader.ID, "downscale");
    GLuint perlworluniform = glGetUniformLocation(ourShader.ID, "perlworl");
    GLuint worluniform =     glGetUniformLocation(ourShader.ID, "worl");
    GLuint curluniform =     glGetUniformLocation(ourShader.ID, "curl");
    GLuint weatheruniform =  glGetUniformLocation(ourShader.ID, "weather");
    GLuint psunPosition =    glGetUniformLocation(ourShader.ID, "sunPosition");//for preetham
    GLuint cameraPosition =  glGetUniformLocation(ourShader.ID, "cameraPos");

    upscaleShader.use();
    GLuint upuniformMatrix =   glGetUniformLocation(upscaleShader.ID, "MVPM");
    GLuint upLFuniformMatrix = glGetUniformLocation(upscaleShader.ID, "LFMVPM");
    GLuint upcheck =           glGetUniformLocation(upscaleShader.ID, "check");
    GLuint upresolution =      glGetUniformLocation(upscaleShader.ID, "resolution");
    GLuint updownscale =       glGetUniformLocation(upscaleShader.ID, "downscale");
    GLuint upaspect =          glGetUniformLocation(upscaleShader.ID, "aspect");
    GLuint buffuniform =       glGetUniformLocation(upscaleShader.ID, "buff");
    GLuint ponguniform =       glGetUniformLocation(upscaleShader.ID, "pong");
#pragma endregion
    
    int check = 0; // used for checkerboarding in the upscale shader
    while (!glfwWindowShouldClose(window))
    {
        // This block measures frame time in ms or fps
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        timePassed = currentFrame;
        if (timePassed - startTime > 0.25 && frames > 10) {
            //frame rate
            //std::cout<<frames/(timePassed-startTime)<<std::endl;
            //time in milliseconds
            //std::cout << deltaTime * 1000.0 << std::endl;
            startTime = timePassed;
            frames = 0;
        }
        lastFrame = currentFrame;
        frames++;

        LFMVPM = MVPM; // copy last MVP matrix for use in frame reprojection

        // check camera movement
        glfwPollEvents();
        doMovement();

        // check for errors TODO wrap in a define DEBUG
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR)
        {
            std::cout << err << std::endl;
        }


        // Write to quarter scale buffer
        glBindFramebuffer(GL_FRAMEBUFFER, subbuffer);
        glViewport(0, 0, WIDTH / downscale, HEIGHT / downscale);  // now the downscale is 4, 1/16 sized image

        ourShader.use();

        glUniform1f(timeu, timePassed);
        glUniformMatrix4fv(uniformMatrix, 1, GL_FALSE, glm::value_ptr(MVPM));
        glUniform1f(aspectUniform, ASPECT);
        glUniform1i(checku, (check) % (downscalesq));
        glUniform2f(resolutionu, GLfloat(WIDTH), GLfloat(HEIGHT));
        glUniform1f(downscaleu, GLfloat(downscale));
        glUniform3f(cameraPosition, camera.Position.x, camera.Position.y, camera.Position.z);

        glUniform1i(perlworluniform, 0); // 4 textures
        glUniform1i(worluniform, 1);
        glUniform1i(curluniform, 2);
        glUniform1i(weatheruniform, 3);

        // variables for preetham model  (A Practical Analytic Model for Daylight, 1999 ???)
        const float PI = 3.141592653589793238462643383279502884197169;
        //float theta = PI * (-0.23 + 0.25 * sin(timePassed * 0.1)); // change sun position
        float theta = PI * (-0.23 + 0.25 * sin(1 * 0.1)); // keep sun position
        float phi = 2 * PI * (-0.25);
        float sunposx = cos(phi);
        float sunposy = sin(phi) * sin(theta);
        float sunposz = sin(phi) * cos(theta);
        glUniform3f(psunPosition, GLfloat(sunposx), GLfloat(sunposy), GLfloat(sunposz));


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, perlworltex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_3D, worltex);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, curltex);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, weathertex);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);


        // upscale the buffer into full size framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, WIDTH, HEIGHT);

        upscaleShader.use();
        glUniformMatrix4fv(upLFuniformMatrix, 1, GL_FALSE, glm::value_ptr(LFMVPM));
        glUniformMatrix4fv(upuniformMatrix, 1, GL_FALSE, glm::value_ptr(MVPM));
        glUniform1i(upcheck, (check) % downscalesq);
        glUniform2f(upresolution, GLfloat(WIDTH), GLfloat(HEIGHT));
        glUniform1f(updownscale, GLfloat(downscale));
        glUniform1f(upaspect, ASPECT);

        glUniform1i(buffuniform, 0);
        glUniform1i(ponguniform, 1);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, subbuffertex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, copyfbotex);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);


        // copy the full size buffer so it can be read from next frame
        glBindFramebuffer(GL_FRAMEBUFFER, copyfbo);

        postShader.use();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fbotex);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);


        // copy to the screen
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        postShader.use();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fbotex);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);

        // Swap the screen buffers
        glfwSwapBuffers(window);
        check++;
    }
    
#pragma region Delete Buffers
    // not sure if this is necessary//it certainly looks bad
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &fbo);
    glDeleteBuffers(1, &copyfbo);
    glDeleteBuffers(1, &subbuffer);
    glDeleteTextures(1, &fbotex);
    glDeleteTextures(1, &copyfbotex);
    glDeleteTextures(1, &subbuffertex);
    glDeleteTextures(1, &perlworltex);
    glDeleteTextures(1, &worltex);
    glDeleteTextures(1, &curltex);
    glDeleteTextures(1, &weathertex);
    glfwTerminate();
#pragma endregion

    return 0;
    
}

void doMovement()
{
    glm::mat4 view;
    view = camera.getViewMatrix();
    glm::mat4 projection;
    projection = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f);
    MVPM = projection * view;
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    GLfloat xoffset = xpos - lastX;
    GLfloat yoffset = lastY - ypos;  // Reversed since y-coordinates go from bottom to left

    lastX = xpos;
    lastY = ypos;

    camera.processMouseMovement(xoffset, yoffset);

    glm::mat4 view;
    view = camera.getViewMatrix();
    glm::mat4 projection;
    projection = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f);
    MVPM = projection * view;
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.processMouseScroll(yoffset);

    glm::mat4 view;
    view = camera.getViewMatrix();
    glm::mat4 projection;
    projection = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f);
    MVPM = projection * view;
}

void keyCallBack(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    //cout << key << endl;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    /*if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    }*/

    // the movement of the camera has problems
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        camera.Position += glm::vec3(100.0f, 0.0f, 0.0f);
    }
    else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        camera.Position += glm::vec3(-100.0f, 0.0f, 0.0f);
    }


}

unsigned int loadTexture2D(const std::string& path, bool gamma)
{
    std::string filePath = path;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << filePath << std::endl;
        stbi_image_free(data);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    return textureID;
}

unsigned int loadTexture3D(const std::string& path, float width, float height, float depth, bool gamma)
{
    std::string filePath = path;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int tempWidth, tempHeight, nrComponents;
    unsigned char* data = stbi_load(filePath.c_str(), &tempWidth, &tempHeight, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_3D, textureID);
        glTexImage3D(GL_TEXTURE_3D, 0, format, width, height, depth, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_3D);

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        glBindTexture(GL_TEXTURE_3D, 0);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << filePath << std::endl;
        stbi_image_free(data);
        glBindTexture(GL_TEXTURE_3D, 0);
    }

    return textureID;
}
