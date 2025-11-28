#include "common.h"

#include <imGui/imgui.h>
#include <imGui/backends/imgui_impl_glfw.h>
#include <imGui/backends/imgui_impl_opengl3.h>

#include "shader.h"
#include "camera.h"

// fn prototype for callback function that is called whenever window resizes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
// fn prototype for fn to handle inputs
void processInput(GLFWwindow* window);

// array to store vertices of triangle
float vertices[] = {
-0.5f, -0.5f, 0.0f,
0.5f, -0.5f, 0.0f,
0.0f, 0.5f, 0.0f
};

camera* pCam = nullptr;
shader* pShader = nullptr;

float WIDTH = 1280.f;
float HEIGHT = 720.0f;

int main(void)
{
    if (!glfwInit()) {
        std::cerr << "GLFW init failed\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    // creating a window object
    GLFWwindow* window = glfwCreateWindow(static_cast<int>(WIDTH), static_cast<int>(HEIGHT), "first window", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    // make the window's context the main context on the current thread
    glfwMakeContextCurrent(window);

    // initialize GLAD before calling any OpenGL function
    // glfwGetProcAddress defines OS specific fn that is then passed to GLAD to load address of OpenGL fn ptrs
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // IMGUI INIT
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");
    io.Fonts->AddFontDefault();
    ImGui::GetIO().FontGlobalScale = 2.0f;

    // tell OpenGL the size of rendering window 
    glViewport(0, 0, 1280, 720);
    // tell GLFW we want to call this function on every window resize
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // --- Create shaders ---
    shader newShader("shaders/render.vert", "shaders/juliaSet.frag");
    pShader = &newShader;

    // init camera
    camera newCam = camera(WIDTH, HEIGHT);
    pCam = &newCam;

    // --- Create texture to store compute shader output ---
    //GLuint outTexture;
    //glGenTextures(1, &outTexture);
    //glBindTexture(GL_TEXTURE_2D, outTexture);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1280, 720, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    //glBindImageTexture(0, outTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);


    // --- create the full screen quad VAO ---
    float quadVerts[] = {
    -1, -1,
     1, -1,
     1,  1,
    -1, -1,
     1,  1,
    -1,  1
    };

    GLuint quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glBindVertexArray(0);

    // uniforms
    glm::vec4 juliaConstant = glm::vec4(-0.04, 0.95, 0.4, -0.43);
    int aaSamples = 4;
    float epsilonValues[] = { 1e-1,1e-2,1e-3,1e-4,1e-5,1e-6 };
    float epsilon = 1e-4f;
    int epsilonIndex = 3;
    int maxIterations = 80;

    while (!glfwWindowShouldClose(window)) 
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowSize(ImVec2(650, 270), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Always);

        ImGui::Begin("Julia Controls");

        ImGui::SliderFloat("FOV", &pCam->fov, 20.f, 120.f);

        ImGui::Text("Julia Constant (Quaternion)");
        ImGui::PushItemWidth(120.0f);   // width for each slider
        ImGui::DragFloat("##i", &juliaConstant.x, 0.01f, -2.0f, 2.0f);
        ImGui::SameLine();
        ImGui::Text("i");
        ImGui::SameLine();
        ImGui::DragFloat("##j", &juliaConstant.y, 0.01f, -2.0f, 2.0f);
        ImGui::SameLine();
        ImGui::Text("j"); 
        ImGui::SameLine();
        ImGui::DragFloat("##k", &juliaConstant.z, 0.01f, -2.0f, 2.0f);
        ImGui::SameLine();
        ImGui::Text("k");
        ImGui::SameLine();
        ImGui::DragFloat("##w", &juliaConstant.w, 0.01f, -2.0f, 2.0f);
        ImGui::SameLine();
        ImGui::Text("w");
        ImGui::PopItemWidth();

        ImGui::Combo("Epsilon", &epsilonIndex, "1e-1\0 1e-2\0 1e-3\0 1e-4\0 1e-5\0 1e-6\0");
        epsilon = epsilonValues[epsilonIndex];

        ImGui::SliderInt("AA Samples", &aaSamples, 1, 64);
        ImGui::SliderInt("Max Iterations", &maxIterations, 1, 200);


        ImGui::End();

        ImGui::Render();


        pShader->bindVF();

        processInput(window);

        pCam->setUniforms(pShader);
        pShader->setUniformV4("juliaConstant", juliaConstant);
        pShader->setUniform1i("maxSteps", maxIterations);
        pShader->setUniform1f("EPSILON", epsilon);
        pShader->setUniform1i("AASAMPLES", aaSamples);

        glBindVertexArray(quadVAO);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window); 
        glfwPollEvents();
    }

    // clean up memory allocated by GLFW
    glfwTerminate();

    return 0;
}

//Whenever the window changes in size, GLFW calls this fn to resize the OpenGL rendering window
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    // update camera resolution & uniform
    pCam->updateResolution(static_cast<float>(width), static_cast<float>(height));
    pShader->setUniformV2("resolution", pCam->getResolution());
    std::cout << pCam->getResolution().x << pCam->getResolution().y << std::endl;
}

// function to handle user inputs
void processInput(GLFWwindow* window)
{
    // if user pressed ESC, we close GLFW, causing an exit of the render loop in main
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    const float speed = 1.5f * 0.01f; // adjust sensitivity

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) pCam->yaw += speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) pCam->yaw -= speed;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) pCam->pitch += speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) pCam->pitch -= speed;

    pShader->setUniformMat3("rotation", pCam->rotationMat());
}

