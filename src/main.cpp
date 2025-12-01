#include "common.h"

#include <imGui/imgui.h>
#include <imGui/backends/imgui_impl_glfw.h>
#include <imGui/backends/imgui_impl_opengl3.h>

#include "shader.h"
#include "camera.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

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

    GLFWwindow* window = glfwCreateWindow(static_cast<int>(WIDTH), static_cast<int>(HEIGHT), "first window", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // initialize glad
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

    glViewport(0, 0, 1280, 720);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // init shader & camera
    shader newShader("shaders/render.vert", "shaders/juliaSet.frag");
    pShader = &newShader;
    camera newCam = camera(WIDTH, HEIGHT);
    pCam = &newCam;
    pShader->bindVF();              // ok since we only got 1 shader
    pCam->setUniforms(pShader);
    pShader->updateSettings();

    // full screen quad VAO 
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
    float epsilonValues[] = { 1e-1f,1e-2f,1e-3f,1e-4f,1e-5f,1e-6f };
    int epsilonIndex = 2;

    while (!glfwWindowShouldClose(window)) 
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowSize(ImVec2(650, 270), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Always);

        ImGui::Begin("Shader Controls");

        ImGui::SliderFloat("FOV", &pShader->currSet.fov, 20.f, 120.f);

        ImGui::Text("Julia Constant (Quaternion)");
        ImGui::PushItemWidth(120.0f);   // width for each slider
        ImGui::DragFloat("##w", &pShader->currSet.juliaConstant.x, 0.01f, -2.0f, 2.0f);
        ImGui::SameLine();
        ImGui::Text("w");
        ImGui::SameLine();
        ImGui::DragFloat("##i", &pShader->currSet.juliaConstant.y, 0.01f, -2.0f, 2.0f);
        ImGui::SameLine();
        ImGui::Text("i"); 
        ImGui::SameLine();
        ImGui::DragFloat("##j", &pShader->currSet.juliaConstant.z, 0.01f, -2.0f, 2.0f);
        ImGui::SameLine();
        ImGui::Text("j");
        ImGui::SameLine();
        ImGui::DragFloat("##k", &pShader->currSet.juliaConstant.w, 0.01f, -2.0f, 2.0f);
        ImGui::SameLine();
        ImGui::Text("k");
        ImGui::PopItemWidth();

        ImGui::Combo("Epsilon", &epsilonIndex, "1e-1\0 1e-2\0 1e-3\0 1e-4\0 1e-5\0 1e-6\0");
        pShader->currSet.epsilon = epsilonValues[epsilonIndex];
        ImGui::SliderInt("AA Samples", &pShader->currSet.aaSamples, 1, 32);
        ImGui::SliderInt("Max Iterations", &pShader->currSet.maxIterations, 1, 200);

        ImGui::End();
        ImGui::Render();

        processInput(window);
        if (pShader->settingsChanged())
        {
            pCam->setUniforms(pShader);
            pShader->updateSettings();
        }

        glBindVertexArray(quadVAO);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window); 
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    // update camera resolution & uniform
    pCam->updateResolution(static_cast<float>(width), static_cast<float>(height));
    pShader->setUniformV2("resolution", pCam->getResolution());
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    const float speed = 1.5f * 0.01f; // adjust sensitivity

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) pCam->yaw += speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) pCam->yaw -= speed;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) pCam->pitch += speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) pCam->pitch -= speed;

    pShader->setUniformMat3("rotation", pCam->rotationMat());
}

