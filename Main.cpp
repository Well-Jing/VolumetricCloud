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
//const GLuint WIDTH = 1920, HEIGHT = 1088;  // both the width and height should be multiple of 16, so 1080->1088
//const GLuint WIDTH = 2560, HEIGHT = 1440;  // both the width and height should be multiple of 16
//const GLuint WIDTH = 512 * 2.0, HEIGHT = 512 * 2.0; // if the window is not square, some antefacts show up
const GLuint downscale = 2; //4 is best//any more and the gains dont make up for the lag
GLuint downscalesq = downscale * downscale;
GLfloat ASPECT = float(WIDTH) / float(HEIGHT);

#pragma endregion

#pragma region Shadow Map Attribution
const GLuint SHADOWMAP_WIDTH = 512, SHADOWMAP_HEIGHT = 512;
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
    glfwSwapInterval(0);

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
    Shader skyShaderSecondaryRayMarching("VertexShader_sky_secondaryRay.vert", "FragmentShader_sky_secondaryRay.frag");
    Shader skyShaderESM("VertexShader_sky_ESM.vert", "FragmentShader_sky_ESM.frag");
    Shader skyShaderBSM("VertexShader_sky_BSM.vert", "FragmentShader_sky_BSM.frag");
    Shader skyShaderFOM("VertexShader_sky_FOM.vert", "FragmentShader_sky_FOM.frag");
    Shader skyShaderSecondaryRayMarchingHigh("VertexShader_sky_secondaryRayHigh.vert", "FragmentShader_sky_secondaryRayHigh.frag");
    Shader ESMShader("VertexShader_ESM.vert", "FragmentShader_ESM.frag");
    Shader ESMFilteringShader("VertexShader_gaussianESM.vert", "FragmentShader_gaussianESM.frag");
    Shader BSMShader("VertexShader_BSM.vert", "FragmentShader_BSM.frag");
    Shader BSMFilteringShader("VertexShader_gaussianBSM.vert", "FragmentShader_gaussianBSM.frag");
    Shader FOMShader("VertexShader_FOM.vert", "FragmentShader_FOM.frag");
    Shader FOMFilteringShader("VertexShader_gaussianFOM.vert", "FragmentShader_gaussianFOM.frag");
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

#pragma region Triangle Vertices
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbotex, 0);


    //our secondary full size framebuffer for copying and reading from the main framebuffer
    GLuint copyfbo, copyfbotex;

    glGenFramebuffers(1, &copyfbo);
    glGenTextures(1, &copyfbotex);
    glBindTexture(GL_TEXTURE_2D, copyfbotex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, copyfbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, copyfbotex, 0);


    //our downscaled buffer that we actually render to
    GLuint subbuffer, subbuffertex;

    glGenFramebuffers(1, &subbuffer);
    glGenTextures(1, &subbuffertex);
    glBindTexture(GL_TEXTURE_2D, subbuffertex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH / downscale, HEIGHT / downscale, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, subbuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, subbuffertex, 0);

    // Exponential Shadow Map shadow map fbo
    GLuint ESMfbo, ESMfbotex;

    glGenFramebuffers(1, &ESMfbo);
    glGenTextures(1, &ESMfbotex);
    glBindTexture(GL_TEXTURE_2D, ESMfbotex);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, ESMfbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ESMfbotex, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    GLuint ESMGaussianFBO, ESMGaussianFBOTex;

    glGenFramebuffers(1, &ESMGaussianFBO);
    glGenTextures(1, &ESMGaussianFBOTex);
    glBindTexture(GL_TEXTURE_2D, ESMGaussianFBOTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, ESMGaussianFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ESMGaussianFBOTex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // Beer Shadow Map shadow map fbo
    GLuint BSMfbo, BSMfbotex;

    glGenFramebuffers(1, &BSMfbo);
    glGenTextures(1, &BSMfbotex);
    glBindTexture(GL_TEXTURE_2D, BSMfbotex);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, BSMfbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, BSMfbotex, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    GLuint BSMGaussianFBO, BSMGaussianFBOTex;

    glGenFramebuffers(1, &BSMGaussianFBO);
    glGenTextures(1, &BSMGaussianFBOTex);
    glBindTexture(GL_TEXTURE_2D, BSMGaussianFBOTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, BSMGaussianFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, BSMGaussianFBOTex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // Fourier Opacity Map shadow map fbo
    GLuint FOMfbo;
    glGenFramebuffers(1, &FOMfbo);
    glBindFramebuffer(GL_FRAMEBUFFER, FOMfbo);
    GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

    GLuint FOMfbotex[2];
    glGenTextures(2, FOMfbotex);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, FOMfbotex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        // attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, FOMfbotex[i], 0);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    GLuint FOMGaussianFBO;
    glGenFramebuffers(1, &FOMGaussianFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FOMGaussianFBO);
    glDrawBuffers(2, attachments);

    GLuint FOMGaussianFBOTex[2];
    glGenTextures(2, FOMGaussianFBOTex);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, FOMGaussianFBOTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        // attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, FOMGaussianFBOTex[i], 0);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion
    
#pragma region Load Noise Texture
    //setup noise textures
    GLuint curltex, worltex, perlworltex, weathertex, blueNoiseTex;

    //stbi_set_flip_vertically_on_load(true);
    curltex = loadTexture2D("assets/curlnoise.png");
    weathertex = loadTexture2D("assets/weather.bmp");
    //worltex = loadTexture3D("assets/worlnoise.bmp", 32, 32, 32);
    perlworltex = loadTexture3D("assets/perlworlnoise.tga", 128, 128, 128);
    worltex = loadTexture3D("assets/noiseErosion.tga", 32, 32, 32);
    //perlworltex = loadTexture3D("assets/noiseShape.tga", 128, 128, 128);
    blueNoiseTex = loadTexture2D("assets/blueNoise1024.png");
#pragma endregion 

#pragma region MVP Matrix Declaration
    //set up Model-View-Projection matrix
    //this way you only update when camera moves
    glm::mat4 view;
    view = camera.getViewMatrix();
    glm::mat4 projection;
    projection = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f);
    MVPM = projection * view;

    glm::mat4 sunView;
    glm::mat4 sunProjection = glm::ortho(-5000.0f, 5000.0f, -5000.0f, 5000.0f, 0.0f, 10.0f);
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
    float debugValue = 1;
    int debugValueInt = 1;

    float cloudRenderTime = 0;
    float cloudRenderStart;
    float cloudRenderSum = 0;
    int cloudRenderConter = 0;
    float shadowmapTime = 0;
    float shadowmapStart;
    float shadowmapSum = 0;
    int shadowmapConter = 0;

    glm::vec3 sunPos;
    float sunTime = 1.0;

    float blueNoiseRate = 14;
    float lowSampleNum = 200;
    float highSampleNum = 500;

    int check = 0; // used for checkerboarding in the upscale shader

    int occlusionMethod = 0;
    float extinction = 30;

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

        // update MVP matrix
        //sunPos = getSunPosition(timePassed);
        sunPos = glm::normalize(getSunPosition(sunTime));
        LFMVPM = MVPM; // copy last MVP matrix for use in frame reprojection
        view = camera.getViewMatrix();
        projection = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f);
        MVPM = projection * view;
        sunView = glm::lookAt(camera.Position + 10000.0f * sunPos, 
            camera.Position + 9999.0f * sunPos,
            glm::vec3(0.0f, 1.0f, 0.0f));
        //sunView = glm::lookAt(10000.0f * sunPos,
        //    9999.0f * sunPos,
        //    glm::vec3(0.0f, 1.0f, 0.0f));

        // check for errors TODO wrap in a define DEBUG
        /*GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR)
        {
            std::cout << err << std::endl;
        }*/
        
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
            if (ImGui::Button("Second ray")) occlusionMethod = 0;
            ImGui::SameLine();
            if (ImGui::Button("ESM")) occlusionMethod = 1;
            ImGui::SameLine();
            if (ImGui::Button("BSM")) occlusionMethod = 2;
            ImGui::SameLine();
            if (ImGui::Button("FOM")) occlusionMethod = 3;
            ImGui::SameLine();
            if (ImGui::Button("Second ray high")) occlusionMethod = 4;

            //ImGui::SliderFloat("Light distance", &lightPosScaleRate, 0.3f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::SliderFloat("Camera speed", &(camera.MovementSpeed), 0.1f, 1000.0f);
            ImGui::SliderFloat("Sun time", &sunTime, 0.0f, 35.0f);
            ImGui::SliderFloat("Blue noise rate", &blueNoiseRate, 0.0f, 50.0f);
            ImGui::SliderFloat("Low sample number", &lowSampleNum, 0.0f, 3 * 512.0f);
            ImGui::SliderFloat("High sample number", &highSampleNum, 0.0f, 3 * 1024.0f);
            ImGui::SliderFloat("Debug value", &debugValue, 0.0f, 1.0f);
            ImGui::SliderInt("Debug value int", &debugValueInt, 1, 10);
            ImGui::ColorEdit3("clear color", (float*)&clear_color);
            ImGui::Text("Cloud render time %f ms/frame", cloudRenderTime);
            ImGui::Text("Shadow map generate time %f ms/frame", shadowmapTime);
            ImGui::Text("Position (%.1f, %.1f, %.1f)", camera.Position.x, camera.Position.y, camera.Position.z);
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }
    
        if (occlusionMethod == 0)
        {
            glFinish();
            cloudRenderStart = glfwGetTime();
            glBindFramebuffer(GL_FRAMEBUFFER, subbuffer);  // bind fbo, the draw texture is 
            glViewport(0, 0, WIDTH / downscale, HEIGHT / downscale);  // now the downscale is 4, 1/16 scaled image
            glDepthMask(GL_FALSE);
            skyShaderSecondaryRayMarching.use();
            skyShaderSecondaryRayMarching.setUniform1i("check", check % downscalesq);
            skyShaderSecondaryRayMarching.setUniform1i("debugValueInt", debugValueInt);
            skyShaderSecondaryRayMarching.setUniform1f("time", timePassed);
            skyShaderSecondaryRayMarching.setUniform1f("aspect", ASPECT);
            skyShaderSecondaryRayMarching.setUniform1f("downscale", (float)downscale);
            skyShaderSecondaryRayMarching.setUniform1f("blueNoiseRate", blueNoiseRate);
            skyShaderSecondaryRayMarching.setUniform1f("debugValue", debugValue);
            skyShaderSecondaryRayMarching.setUniform1f("lowSampleNum", lowSampleNum);
            skyShaderSecondaryRayMarching.setUniform1f("highSampleNum", highSampleNum);
            skyShaderSecondaryRayMarching.setUniform2f("resolution", glm::vec2((float)WIDTH, (float)HEIGHT));
            skyShaderSecondaryRayMarching.setUniform3f("cameraPos", camera.Position);
            skyShaderSecondaryRayMarching.setUniform3f("sunDirection", sunPos);
            skyShaderSecondaryRayMarching.setUniformMatrix("sunView", sunView);
            skyShaderSecondaryRayMarching.setUniformMatrix("sunProj", sunProjection);
            skyShaderSecondaryRayMarching.setUniformMatrix("MVPM", MVPM);
            skyShaderSecondaryRayMarching.setUniformMatrix("invMVPM", invMVPM);
            skyShaderSecondaryRayMarching.setUniformMatrix("invView", glm::inverse(view));
            skyShaderSecondaryRayMarching.setUniformMatrix("invProj", glm::inverse(projection));

            // set textures (weather + noise)
            skyShaderSecondaryRayMarching.setUniform1i("perlworl", 0);
            skyShaderSecondaryRayMarching.setUniform1i("worl", 1);
            skyShaderSecondaryRayMarching.setUniform1i("curl", 2);
            skyShaderSecondaryRayMarching.setUniform1i("weather", 3);
            skyShaderSecondaryRayMarching.setUniform1i("blueNoise", 4);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_3D, perlworltex);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_3D, worltex);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, curltex);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, weathertex);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, blueNoiseTex);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);
        }

        if (occlusionMethod == 1)
        {
            glFinish();
            cloudRenderStart = glfwGetTime();

            glFinish();
            shadowmapStart = glfwGetTime();
            glViewport(0, 0, SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT);
            glBindFramebuffer(GL_FRAMEBUFFER, ESMfbo);
            ESMShader.use();            
            ESMShader.setUniform1f("time", timePassed);
            ESMShader.setUniform1f("extinction", extinction);
            ESMShader.setUniform2f("resolution", glm::vec2((float)SHADOWMAP_WIDTH, (float)SHADOWMAP_HEIGHT));
            ESMShader.setUniform3f("sunDirection", sunPos);
            ESMShader.setUniformMatrix("invView", glm::inverse(sunView));
            ESMShader.setUniformMatrix("invProj", glm::inverse(sunProjection));
            
            ESMShader.setUniform1i("perlworl", 0);
            ESMShader.setUniform1i("worl", 1);
            ESMShader.setUniform1i("curl", 2);
            ESMShader.setUniform1i("weather", 3);
            ESMShader.setUniform1i("blueNoise", 4);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_3D, perlworltex);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_3D, worltex);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, curltex);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, weathertex);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, blueNoiseTex);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);
            
            glBindFramebuffer(GL_FRAMEBUFFER, ESMGaussianFBO);
            ESMFilteringShader.use();
            ESMFilteringShader.setUniformBool("gaussianDir", false);
            ESMFilteringShader.setUniform1i("kernelRadius", 6);
            ESMFilteringShader.setUniform1f("sigma", 18);

            ESMFilteringShader.setUniform1i("fbo", 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ESMfbotex);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);

            glBindFramebuffer(GL_FRAMEBUFFER, ESMfbo);
            ESMFilteringShader.use();
            ESMFilteringShader.setUniformBool("gaussianDir", true);
            ESMFilteringShader.setUniform1i("kernelRadius", 6);
            ESMFilteringShader.setUniform1f("sigma", 18);

            ESMFilteringShader.setUniform1i("fbo", 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ESMGaussianFBOTex);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);

            glFinish();
            shadowmapSum += (glfwGetTime() - shadowmapStart) * 1000;
            shadowmapConter++;
            if (shadowmapConter % 500 == 0)
            {
                shadowmapTime = shadowmapSum / 500;
                shadowmapSum = 0;
            }
            
            

            // Write to quarter scaled buffer (1/4 * 1/4 = 1/16 scaled)
            glBindFramebuffer(GL_FRAMEBUFFER, subbuffer);  // bind fbo, the draw texture is 
            glViewport(0, 0, WIDTH / downscale, HEIGHT / downscale);  // now the downscale is 4, 1/16 scaled image
            glDepthMask(GL_FALSE);
            skyShaderESM.use();
            skyShaderESM.setUniform1i("check", check % downscalesq);
            skyShaderESM.setUniform1i("debugValueInt", debugValueInt);
            skyShaderESM.setUniform1f("time", timePassed);
            skyShaderESM.setUniform1f("aspect", ASPECT);
            skyShaderESM.setUniform1f("downscale", (float)downscale);
            skyShaderESM.setUniform1f("blueNoiseRate", blueNoiseRate);
            skyShaderESM.setUniform1f("debugValue", debugValue);
            skyShaderESM.setUniform1f("lowSampleNum", lowSampleNum);
            skyShaderESM.setUniform1f("highSampleNum", highSampleNum);
            skyShaderESM.setUniform1f("extinction", extinction);
            skyShaderESM.setUniform2f("resolution", glm::vec2((float)WIDTH, (float)HEIGHT));
            skyShaderESM.setUniform3f("cameraPos", camera.Position);
            skyShaderESM.setUniform3f("sunDirection", sunPos);
            skyShaderESM.setUniformMatrix("sunView", sunView);
            skyShaderESM.setUniformMatrix("sunProj", sunProjection);
            skyShaderESM.setUniformMatrix("MVPM", MVPM);
            skyShaderESM.setUniformMatrix("invMVPM", invMVPM);
            skyShaderESM.setUniformMatrix("invView", glm::inverse(view));
            skyShaderESM.setUniformMatrix("invProj", glm::inverse(projection));

            // set textures (weather + noise)
            skyShaderESM.setUniform1i("perlworl", 0);
            skyShaderESM.setUniform1i("worl", 1);
            skyShaderESM.setUniform1i("curl", 2);
            skyShaderESM.setUniform1i("weather", 3);
            skyShaderESM.setUniform1i("blueNoise", 4);
            skyShaderESM.setUniform1i("ESM", 5);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_3D, perlworltex);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_3D, worltex);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, curltex);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, weathertex);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, blueNoiseTex);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, ESMfbotex);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);

            
        }

        if (occlusionMethod == 2)
        {
            glFinish();
            cloudRenderStart = glfwGetTime();

            glFinish();
            shadowmapStart = glfwGetTime();
            glViewport(0, 0, SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT);
            glBindFramebuffer(GL_FRAMEBUFFER, BSMfbo);
            BSMShader.use();
            BSMShader.setUniform1f("time", timePassed);
            BSMShader.setUniform1f("extinction", extinction);
            BSMShader.setUniform2f("resolution", glm::vec2((float)SHADOWMAP_WIDTH, (float)SHADOWMAP_HEIGHT));
            BSMShader.setUniform3f("sunDirection", sunPos);
            BSMShader.setUniformMatrix("invView", glm::inverse(sunView));
            BSMShader.setUniformMatrix("invProj", glm::inverse(sunProjection));

            BSMShader.setUniform1i("perlworl", 0);
            BSMShader.setUniform1i("worl", 1);
            BSMShader.setUniform1i("curl", 2);
            BSMShader.setUniform1i("weather", 3);
            BSMShader.setUniform1i("blueNoise", 4);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_3D, perlworltex);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_3D, worltex);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, curltex);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, weathertex);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, blueNoiseTex);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);

            glBindFramebuffer(GL_FRAMEBUFFER, BSMGaussianFBO);
            BSMFilteringShader.use();
            BSMFilteringShader.setUniformBool("gaussianDir", false);
            BSMFilteringShader.setUniform1i("kernelRadius", 10);
            BSMFilteringShader.setUniform1f("sigma", 8);

            BSMFilteringShader.setUniform1i("fbo", 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, BSMfbotex);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);

            glBindFramebuffer(GL_FRAMEBUFFER, BSMfbo);
            BSMFilteringShader.use();
            BSMFilteringShader.setUniformBool("gaussianDir", true);
            BSMFilteringShader.setUniform1i("kernelRadius", 10);
            BSMFilteringShader.setUniform1f("sigma", 8);

            BSMFilteringShader.setUniform1i("fbo", 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, BSMGaussianFBOTex);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);

            glFinish();
            shadowmapSum += (glfwGetTime() - shadowmapStart) * 1000;
            shadowmapConter++;
            if (shadowmapConter % 500 == 0)
            {
                shadowmapTime = shadowmapSum / 500;
                shadowmapSum = 0;
            }


            // Write to quarter scaled buffer (1/4 * 1/4 = 1/16 scaled)
            glBindFramebuffer(GL_FRAMEBUFFER, subbuffer);  // bind fbo, the draw texture is 
            glViewport(0, 0, WIDTH / downscale, HEIGHT / downscale);  // now the downscale is 4, 1/16 scaled image
            glDepthMask(GL_FALSE);
            skyShaderBSM.use();
            skyShaderBSM.setUniform1i("check", check % downscalesq);
            skyShaderBSM.setUniform1i("debugValueInt", debugValueInt);
            skyShaderBSM.setUniform1f("time", timePassed);
            skyShaderBSM.setUniform1f("aspect", ASPECT);
            skyShaderBSM.setUniform1f("downscale", (float)downscale);
            skyShaderBSM.setUniform1f("blueNoiseRate", blueNoiseRate);
            skyShaderBSM.setUniform1f("debugValue", debugValue);
            skyShaderBSM.setUniform1f("lowSampleNum", lowSampleNum);
            skyShaderBSM.setUniform1f("highSampleNum", highSampleNum);
            skyShaderBSM.setUniform1f("extinction", extinction);
            skyShaderBSM.setUniform2f("resolution", glm::vec2((float)WIDTH, (float)HEIGHT));
            skyShaderBSM.setUniform3f("cameraPos", camera.Position);
            skyShaderBSM.setUniform3f("sunDirection", sunPos);
            skyShaderBSM.setUniformMatrix("sunView", sunView);
            skyShaderBSM.setUniformMatrix("sunProj", sunProjection);
            skyShaderBSM.setUniformMatrix("MVPM", MVPM);
            skyShaderBSM.setUniformMatrix("invMVPM", invMVPM);
            skyShaderBSM.setUniformMatrix("invView", glm::inverse(view));
            skyShaderBSM.setUniformMatrix("invProj", glm::inverse(projection));

            // set textures (weather + noise)
            skyShaderBSM.setUniform1i("perlworl", 0);
            skyShaderBSM.setUniform1i("worl", 1);
            skyShaderBSM.setUniform1i("curl", 2);
            skyShaderBSM.setUniform1i("weather", 3);
            skyShaderBSM.setUniform1i("blueNoise", 4);
            skyShaderBSM.setUniform1i("BSM", 5);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_3D, perlworltex);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_3D, worltex);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, curltex);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, weathertex);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, blueNoiseTex);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, BSMfbotex);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);
        }

        if (occlusionMethod == 3)
        {
            glFinish();
            cloudRenderStart = glfwGetTime();

            glFinish();
            shadowmapStart = glfwGetTime();
            glViewport(0, 0, SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT);
            glBindFramebuffer(GL_FRAMEBUFFER, FOMfbo);
            FOMShader.use();
            FOMShader.setUniform1f("time", timePassed);
            FOMShader.setUniform1f("extinction", extinction);
            FOMShader.setUniform2f("resolution", glm::vec2((float)SHADOWMAP_WIDTH, (float)SHADOWMAP_HEIGHT));
            FOMShader.setUniform3f("sunDirection", sunPos);
            FOMShader.setUniformMatrix("invView", glm::inverse(sunView));
            FOMShader.setUniformMatrix("invProj", glm::inverse(sunProjection));

            FOMShader.setUniform1i("perlworl", 0);
            FOMShader.setUniform1i("worl", 1);
            FOMShader.setUniform1i("curl", 2);
            FOMShader.setUniform1i("weather", 3);
            FOMShader.setUniform1i("blueNoise", 4);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_3D, perlworltex);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_3D, worltex);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, curltex);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, weathertex);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, blueNoiseTex);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);

            glBindFramebuffer(GL_FRAMEBUFFER, FOMGaussianFBO);
            FOMFilteringShader.use();
            FOMFilteringShader.setUniformBool("gaussianDir", false);
            FOMFilteringShader.setUniform1i("kernelRadius", 10);
            FOMFilteringShader.setUniform1f("sigma", 8);

            FOMFilteringShader.setUniform1i("fbo0", 0);
            FOMFilteringShader.setUniform1i("fbo1", 1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, FOMfbotex[0]);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, FOMfbotex[1]);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);

            glBindFramebuffer(GL_FRAMEBUFFER, FOMfbo);
            FOMFilteringShader.use();
            FOMFilteringShader.setUniformBool("gaussianDir", true);
            FOMFilteringShader.setUniform1i("kernelRadius", 10);
            FOMFilteringShader.setUniform1f("sigma", 8);

            FOMFilteringShader.setUniform1i("fbo0", 0);
            FOMFilteringShader.setUniform1i("fbo1", 1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, FOMGaussianFBOTex[0]);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, FOMGaussianFBOTex[1]);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);

            glFinish();
            shadowmapSum += (glfwGetTime() - shadowmapStart) * 1000;
            shadowmapConter++;
            if (shadowmapConter % 500 == 0)
            {
                shadowmapTime = shadowmapSum / 500;
                shadowmapSum = 0;
            }


            // Write to quarter scaled buffer (1/4 * 1/4 = 1/16 scaled)
            glBindFramebuffer(GL_FRAMEBUFFER, subbuffer);  // bind fbo, the draw texture is 
            glViewport(0, 0, WIDTH / downscale, HEIGHT / downscale);  // now the downscale is 4, 1/16 scaled image
            glDepthMask(GL_FALSE);
            skyShaderFOM.use();
            skyShaderFOM.setUniform1i("check", check % downscalesq);
            skyShaderFOM.setUniform1i("debugValueInt", debugValueInt);
            skyShaderFOM.setUniform1f("time", timePassed);
            skyShaderFOM.setUniform1f("aspect", ASPECT);
            skyShaderFOM.setUniform1f("downscale", (float)downscale);
            skyShaderFOM.setUniform1f("blueNoiseRate", blueNoiseRate);
            skyShaderFOM.setUniform1f("debugValue", debugValue);
            skyShaderFOM.setUniform1f("lowSampleNum", lowSampleNum);
            skyShaderFOM.setUniform1f("highSampleNum", highSampleNum);
            skyShaderFOM.setUniform1f("extinction", extinction);
            skyShaderFOM.setUniform2f("resolution", glm::vec2((float)WIDTH, (float)HEIGHT));
            skyShaderFOM.setUniform3f("cameraPos", camera.Position);
            skyShaderFOM.setUniform3f("sunDirection", sunPos);
            skyShaderFOM.setUniformMatrix("sunView", sunView);
            skyShaderFOM.setUniformMatrix("sunProj", sunProjection);
            skyShaderFOM.setUniformMatrix("MVPM", MVPM);
            skyShaderFOM.setUniformMatrix("invMVPM", invMVPM);
            skyShaderFOM.setUniformMatrix("invView", glm::inverse(view));
            skyShaderFOM.setUniformMatrix("invProj", glm::inverse(projection));

            // set textures (weather + noise)
            skyShaderFOM.setUniform1i("perlworl", 0);
            skyShaderFOM.setUniform1i("worl", 1);
            skyShaderFOM.setUniform1i("curl", 2);
            skyShaderFOM.setUniform1i("weather", 3);
            skyShaderFOM.setUniform1i("blueNoise", 4);
            skyShaderFOM.setUniform1i("FOM0", 5);
            skyShaderFOM.setUniform1i("FOM1", 6);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_3D, perlworltex);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_3D, worltex);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, curltex);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, weathertex);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, blueNoiseTex);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, FOMfbotex[0]);
            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_2D, FOMfbotex[1]);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);
        }

        if (occlusionMethod == 4)
        {
            glFinish();
            cloudRenderStart = glfwGetTime();
            glBindFramebuffer(GL_FRAMEBUFFER, subbuffer);  // bind fbo, the draw texture is 
            glViewport(0, 0, WIDTH / downscale, HEIGHT / downscale);  // now the downscale is 4, 1/16 scaled image
            glDepthMask(GL_FALSE);
            skyShaderSecondaryRayMarchingHigh.use();
            skyShaderSecondaryRayMarchingHigh.setUniform1i("check", check % downscalesq);
            skyShaderSecondaryRayMarchingHigh.setUniform1i("debugValueInt", debugValueInt);
            skyShaderSecondaryRayMarchingHigh.setUniform1f("time", timePassed);
            skyShaderSecondaryRayMarchingHigh.setUniform1f("aspect", ASPECT);
            skyShaderSecondaryRayMarchingHigh.setUniform1f("downscale", (float)downscale);
            skyShaderSecondaryRayMarchingHigh.setUniform1f("blueNoiseRate", blueNoiseRate);
            skyShaderSecondaryRayMarchingHigh.setUniform1f("debugValue", debugValue);
            skyShaderSecondaryRayMarchingHigh.setUniform1f("lowSampleNum", lowSampleNum);
            skyShaderSecondaryRayMarchingHigh.setUniform1f("highSampleNum", highSampleNum);
            skyShaderSecondaryRayMarchingHigh.setUniform2f("resolution", glm::vec2((float)WIDTH, (float)HEIGHT));
            skyShaderSecondaryRayMarchingHigh.setUniform3f("cameraPos", camera.Position);
            skyShaderSecondaryRayMarchingHigh.setUniform3f("sunDirection", sunPos);
            skyShaderSecondaryRayMarchingHigh.setUniformMatrix("sunView", sunView);
            skyShaderSecondaryRayMarchingHigh.setUniformMatrix("sunProj", sunProjection);
            skyShaderSecondaryRayMarchingHigh.setUniformMatrix("MVPM", MVPM);
            skyShaderSecondaryRayMarchingHigh.setUniformMatrix("invMVPM", invMVPM);
            skyShaderSecondaryRayMarchingHigh.setUniformMatrix("invView", glm::inverse(view));
            skyShaderSecondaryRayMarchingHigh.setUniformMatrix("invProj", glm::inverse(projection));

            // set textures (weather + noise)
            skyShaderSecondaryRayMarchingHigh.setUniform1i("perlworl", 0);
            skyShaderSecondaryRayMarchingHigh.setUniform1i("worl", 1);
            skyShaderSecondaryRayMarchingHigh.setUniform1i("curl", 2);
            skyShaderSecondaryRayMarchingHigh.setUniform1i("weather", 3);
            skyShaderSecondaryRayMarchingHigh.setUniform1i("blueNoise", 4);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_3D, perlworltex);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_3D, worltex);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, curltex);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, weathertex);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, blueNoiseTex);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);
        }

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

        glFinish();
        cloudRenderSum += (glfwGetTime() - cloudRenderStart) * 1000;
        cloudRenderConter++;
        if (cloudRenderConter % 500 == 0)
        {
            cloudRenderTime = cloudRenderSum / 500;
            cloudRenderSum = 0;
        }


#pragma region Draw Bunny
        glCullFace(GL_BACK);
        blinnPhongShader.use();
        glm::mat4 modelMat = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));
        modelMat = glm::translate(modelMat, glm::vec3(0.0f, 0.0f, -20.0f));
        blinnPhongShader.setUniformMatrix("modelMat", modelMat);
        blinnPhongShader.setUniformMatrix("viewMat", view);
        blinnPhongShader.setUniformMatrix("projMat", projection);

        //bunny.Draw(&blinnPhongShader);
#pragma endregion

#pragma region Draw Ground
        blinnPhongShader.use();
        modelMat = glm::scale(glm::mat4(1.0f), glm::vec3(100.0f, 1.0f, 100.0f));
        modelMat = glm::translate(modelMat, glm::vec3(0.0f, -0.5f, 0.0f));
        blinnPhongShader.setUniformMatrix("modelMat", modelMat);
        blinnPhongShader.setUniformMatrix("viewMat", view);
        blinnPhongShader.setUniformMatrix("projMat", projection);

        //ground.Draw(&blinnPhongShader);
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

        float memory = width * height * nrComponents * 8 / 1024;
        std::cout << "Succeed load 2D texture at : " << filePath << ", memory occupy : " << memory << " KB (add mipmap: " 
            << memory * 4 / 3 << " KB)" << std::endl;
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
        //glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);  // when clamp to border, all the cloud disappear (why?)
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        float memory = width * height * depth * nrComponents * 8 / 1024;
        std::cout << "Succeed load 3D texture at : " << filePath << ", memory occupy : " << memory << " KB (add mipmap: " 
            << memory * 8 / 7 << " KB)" << std::endl;
        stbi_image_free(data);
        glBindTexture(GL_TEXTURE_3D, 0);
    }
    else
    {
        std::cout << "Texture failed to load at : " << filePath << std::endl;
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


unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}
#pragma endregion