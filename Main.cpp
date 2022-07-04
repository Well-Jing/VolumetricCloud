#include <iostream>
#include <stdlib.h> 
#include <string>
#include <iomanip>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/string_cast.hpp>
#include "imgui/imgui.h" // Configure ImGUI 1/5
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "Shader.h"
#include "Camera.h"
#include "Model.h"
#include "LightDirectional.h"

// import stb_image shoule be after model.h
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#pragma region Function Declaration
void keyCallBack(GLFWwindow* window);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
unsigned int loadTexture2D(const std::string& path, bool gamma = true);
unsigned int loadTexture3D(const std::string& path, float width, float height, float depth, bool gamma = true);
glm::vec3 getSunPosition(float time);
#pragma endregion

#pragma region Camera Creation and Attribution
// Camera
Camera camera(glm::vec3(0.0f, 0.0f, 0.0f));
bool keys[1024];
GLfloat lastX = 400, lastY = 300;
bool firstMouse = true;
glm::mat4 MVPM;
glm::mat4 invMVPM;
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
const GLuint WIDTH = 1280, HEIGHT = 720;  // both the width and height should be multiple of 16
//const GLuint WIDTH = 512 * 3.0, HEIGHT = 512 * 2.0; // if the window is not square, some antefacts show up
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
    //glfwSetKeyCallback(window, keyCallBack);

    glfwSetWindowPos(window, 200, 17);//so you can see frame rate
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSwapInterval(0);//turn off vsync

    //GLEW init
    glewExperimental = GL_TRUE;
    glewInit();

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    //not sure this is necessary?
    glViewport(0, 0, WIDTH, HEIGHT);
#pragma endregion
    
#pragma region Create Shader
    //Shader class built on the one in learnopengl.com
    Shader skyShader("VertexShader_sky.vert", "FragmentShader_sky.frag");
    Shader upscaleShader("VertexShader_upscale.vert", "FragmentShader_upscale.frag");
    Shader postShader("VertexShader_postprocess.vert", "FragmentShader_postprocess.frag");
    Shader blinnPhongShader("VertexShader_blinnPhong.vert", "FragmentShader_blinnPhong.frag");
#pragma endregion

#pragma region Light Declaration
    LightDirectional lightD(glm::vec3(10.0f, 30.0f, 20.0f), glm::vec3(glm::radians(110.0f), glm::radians(30.0f), 0), glm::vec3(1.0f, 0.95f, 0.8f));
#pragma endregion

#pragma region Pass light to shaders;
    blinnPhongShader.use();
    blinnPhongShader.setUniform3f("lightD.pos", glm::vec3(lightD.position.x, lightD.position.y, lightD.position.z));
    blinnPhongShader.setUniform3f("lightD.color", glm::vec3(lightD.color.x, lightD.color.y, lightD.color.z));
    blinnPhongShader.setUniform3f("lightD.dirToLight", glm::vec3(lightD.direction.x, lightD.direction.y, lightD.direction.z));
#pragma endregion

#pragma region Triagnle Vertices
    GLfloat vertices[] = {
       -1.0f, -1.0f,
       -1.0f,  3.0f,
        3.0f, -1.0f,
    };
#pragma endregion

#pragma region Load Models
    Model bunny(".\\model\\bunny\\bunny.obj");
    Model ground(".\\model\\ground\\ground.obj");
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

#pragma region Configure ImGUI
    // Configure ImGUI 2/5
    const char* glsl_version = "#version 130";
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImGui::StyleColorsDark();
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
#pragma endregion

    
    camera.MovementSpeed = 20;
    glm::vec3 sunPos;
    float sunTime = 1.0;

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
        
        /*glm::mat4 newMat = glm::inverse(view) * (glm::inverse(projection) * projection) * view;
        glm::mat4 newMat2 = glm::inverse(view) * glm::inverse(projection) * projection * view;
        glm::mat4 newMatView = glm::inverse(view) * view;
        glm::mat4 newMatProj = glm::inverse(projection) * projection;
        glm::mat4 translate = glm::translate(glm::mat4(1.0f), glm::vec3(10000.0f, 11110.0f, 22222.0f));
        translate[2][0] = 0.1f;
        translate[1][0] = 0.2f;
        translate[3][2] = 0.2f;
        glm::mat4 invTranslate = glm::inverse(translate);

        glm::mat4 resultMat = invTranslate * translate;

        if (int(timePassed) - int(lastFrame))
        {
            std::cout.setf(std::ios::fixed);
            std::cout << std::setprecision(5) << std::setw(8) << std::endl;

            std::cout << "Translate Matrix: " << std::endl;
            std::cout << std::setw(10) << translate[0][0] << " " << std::setw(10) << translate[0][1] << " " << std::setw(10) << translate[0][2] << " " << std::setw(10) << translate[0][3] << "   " << std::endl;
            std::cout << std::setw(10) << translate[1][0] << " " << std::setw(10) << translate[1][1] << " " << std::setw(10) << translate[1][2] << " " << std::setw(10) << translate[1][3] << "   " << std::endl;
            std::cout << std::setw(10) << translate[2][0] << " " << std::setw(10) << translate[2][1] << " " << std::setw(10) << translate[2][2] << " " << std::setw(10) << translate[2][3] << "   " << std::endl;
            std::cout << std::setw(10) << translate[3][0] << " " << std::setw(10) << translate[3][1] << " " << std::setw(10) << translate[3][2] << " " << std::setw(10) << translate[3][3] << "   " << std::endl;

            std::cout << "==================================================================" << std::endl;
        }*/



        lastFrame = currentFrame;
        frames++;

        LFMVPM = MVPM; // copy last MVP matrix for use in frame reprojection

        // update MVP matrix
        view = camera.getViewMatrix();
        projection = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f);
        MVPM = projection * view;
        //invMVPM = glm::inverse(view) * glm::inverse(projection);   

        // check for errors TODO wrap in a define DEBUG
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR)
        {
            std::cout << err << std::endl;
        }

        keyCallBack(window);
        glCullFace(GL_FRONT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // Configure ImGUI 3/5
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            ImGui::Begin("ImGUI interface");                          // Create a window called "Hello, world!" and append into it.

            //ImGui::Checkbox("Show effect with Normal Mapping", &displayBump);
            //ImGui::Checkbox("Rotate the point light", &rotateLight);
            //ImGui::SliderFloat("Light distance", &lightPosScaleRate, 0.3f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::SliderFloat("Camera speed", &(camera.MovementSpeed), 0.1f, 1000.0f);
            ImGui::SliderFloat("Sun time", &sunTime, 0.0f, 35.0f);
            ImGui::ColorEdit3("clear color", (float*)&clear_color);
            ImGui::Text("Position (%.1f, %.1f, %.1f)", camera.Position.x, camera.Position.y, camera.Position.z);
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }


        // Write to quarter scale buffer (1/16 sized)
        glBindFramebuffer(GL_FRAMEBUFFER, subbuffer);  // bind fbo, the draw texture is 
        glViewport(0, 0, WIDTH / downscale, HEIGHT / downscale);  // now the downscale is 4, 1/16 sized image
        glDepthMask(GL_FALSE);
        skyShader.use();
        skyShader.setUniform1i("check", check % downscalesq);
        skyShader.setUniform1f("time", timePassed);
        skyShader.setUniform1f("aspect", ASPECT);
        skyShader.setUniform1f("downscale", (float)downscale);
        skyShader.setUniform2f("resolution", glm::vec2((float)WIDTH, (float)HEIGHT));
        skyShader.setUniform3f("cameraPos", camera.Position);
        skyShader.setUniformMatrix("MVPM", MVPM);
        skyShader.setUniformMatrix("invMVPM", invMVPM);
        skyShader.setUniformMatrix("invView", glm::inverse(view));
        skyShader.setUniformMatrix("invProj", glm::inverse(projection));

        //sunPos = getSunPosition(timePassed);
        sunPos = getSunPosition(sunTime);
        skyShader.setUniform3f("sunDirection", sunPos);

        // set textures (weather + noise)
        skyShader.setUniform1i("perlworl", 0);
        skyShader.setUniform1i("worl", 1);
        skyShader.setUniform1i("curl", 2);
        skyShader.setUniform1i("weather", 3);
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
        upscaleShader.setUniform1i("check", check % downscalesq);
        upscaleShader.setUniform1f("downscale", (float)downscale);
        upscaleShader.setUniform1f("aspect", ASPECT);
        upscaleShader.setUniform2f("resolution", glm::vec2((float)WIDTH, (float)HEIGHT));
        upscaleShader.setUniformMatrix("MVPM", MVPM);
        upscaleShader.setUniformMatrix("invMVPM", invMVPM);
        upscaleShader.setUniformMatrix("invView", glm::inverse(view));
        upscaleShader.setUniformMatrix("invProj", glm::inverse(projection));
        upscaleShader.setUniformMatrix("LFMVPM", LFMVPM);

        // set textures
        upscaleShader.setUniform1i("buff", 0);
        upscaleShader.setUniform1i("pong", 1);
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


        // copy to the main screen (display)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        postShader.use();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fbotex);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);

#pragma region Draw Bunny
        glCullFace(GL_BACK);
        blinnPhongShader.use();
        glm::mat4 modelMat = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));
        modelMat = glm::translate(modelMat, glm::vec3(0.0f, 0.0f, -20.0f));
        blinnPhongShader.setUniformMatrix("modelMat", modelMat);
        blinnPhongShader.setUniformMatrix("viewMat", view);
        blinnPhongShader.setUniformMatrix("projMat", projection);

        bunny.Draw(&blinnPhongShader);
#pragma endregion

#pragma region Draw Ground
        blinnPhongShader.use();
        modelMat = glm::scale(glm::mat4(1.0f), glm::vec3(100.0f, 1.0f, 100.0f));
        modelMat = glm::translate(modelMat, glm::vec3(0.0f, -0.5f, 0.0f));
        blinnPhongShader.setUniformMatrix("modelMat", modelMat);
        blinnPhongShader.setUniformMatrix("viewMat", view);
        blinnPhongShader.setUniformMatrix("projMat", projection);

        ground.Draw(&blinnPhongShader);
#pragma endregion

        // Configure ImGUI 4/5
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap the screen buffers
        glfwSwapBuffers(window);
        // check camera movement
        glfwPollEvents();
        check++;
    }
    
    // Configure ImGUI 5/5
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

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


#pragma region Functions
void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
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
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
        firstMouse = true;
    /*glm::mat4 view;
    view = camera.getViewMatrix();
    glm::mat4 projection;
    projection = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f);
    MVPM = projection * view;*/
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.processMouseScroll(yoffset);
}

void keyCallBack(GLFWwindow* window)
{
    

    // this might be useful in the future
    /*if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    }*/

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        // the movement of the camera has problems
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
            camera.processKeyboard(Camera_Movement::FORWARD, deltaTime);
        }
        else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            camera.processKeyboard(Camera_Movement::BACKWARD, deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            camera.processKeyboard(Camera_Movement::LEFT, deltaTime);
        }
        else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            camera.processKeyboard(Camera_Movement::RIGHT, deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        {
            camera.processKeyboard(Camera_Movement::UP, deltaTime);
        }
        else if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        {
            camera.processKeyboard(Camera_Movement::DOWN, deltaTime);
        }
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

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
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

glm::vec3 getSunPosition(float time)
{
    const float PI = 3.141592653589793238462643383279502884197169;
    float theta = PI * (-0.23 + 0.25 * sin(time * 0.1)); // change sun position as time passing
    float phi = 2 * PI * (-0.25);
    float sunposx = cos(phi);
    float sunposy = sin(phi) * sin(theta);
    float sunposz = sin(phi) * cos(theta);

    return glm::vec3(sunposx, sunposy, sunposz);
}
#pragma endregion