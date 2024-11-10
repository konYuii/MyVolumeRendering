#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../include/shader.h"
#include "../include/camera.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_opengl3.h"

#include <iostream>
#include <vector>
#include <iomanip> // 用于 std::setprecision
#include <chrono>  // 用于时间计算

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void updateFrameRate(GLFWwindow* window, double deltaTime);
void changeTransferFunction(Shader& shader);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;

// 相机
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f));
float yaw = 0.0f;
float pitch = 0.0f;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

// 体纹理参数
const unsigned int x_dim = 584;
const unsigned int y_dim = 584;
const unsigned int z_dim = 631;

// 传输函数设置
glm::vec3 color_transfer_value[4] = {
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.62f, 0.36f, 0.18f),
    glm::vec3(0.88f, 0.60f, 0.29f),
    glm::vec3(1.0f, 1.0f, 1.0f)
};
int color_transfer_threshold[4] = {
    -3024, -800, 0, 3071
};
float opacity_transfer_value[4] = {
    0.0f, 0.0f, 0.4f, 0.8f
};
int opacity_transfer_threshold[4] = {
    -3024, -800, 300, 3071
};

int main()
{
//读取和转换体数据
#pragma region
    std::ifstream raw_file("../cbct.raw", std::ios::binary);
    if (!raw_file.is_open())
    {
        throw std::runtime_error("Failed to open raw file");
    }

    // 计算总元素数量
    size_t total_elements = x_dim * y_dim * z_dim;
    std::vector<int16_t> raw_data(total_elements);
    std::vector<float> volume_data(total_elements);

    // 读取文件到 raw_data
    raw_file.read(reinterpret_cast<char *>(raw_data.data()), total_elements * sizeof(int16_t));
    raw_file.close();

    for (int i = 0; i < raw_data.size(); ++i)
    {
        //raw_data[i] += 1000;
        volume_data[i] = static_cast<float>(raw_data[i]);
    }
#pragma endregion

// 窗口和正方体
#pragma region
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Volume Rendering", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // 初始化GLEW
    glewExperimental = GL_TRUE; // 启用现代特性
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW 初始化失败!" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);


    // build and compile our shader program
    // ------------------------------------
    Shader ourShader("../shader/shader.vs", "../shader/shader.fs"); // you can name your shader files however you like

    float vertices[] = {
        // positions          // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
         0.5f, 0.5f,  0.5f,  1.0f, 1.0f, 1.0f,
        -0.5f, 0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
    };
    unsigned int indices[] = {
        //前
        0, 1, 2,
        2, 3, 0,
        //后
        4, 5, 6,
        6, 7, 4,
        //左
        4, 0, 3,
        3, 7, 4,
        //右
        1, 5, 6,
        6, 2, 1,
        //上
        3, 2, 6,
        6, 7, 3,
        //下
        0, 4, 5,
        5, 1, 0
    };
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
#pragma endregion

    // 体纹理
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_3D, texture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, x_dim, y_dim, z_dim, 0, GL_RGBA, GL_FLOAT, &transformed_data[0][0][0]);
    //glTexImage3D(GL_TEXTURE_3D, 0, GL_R16I, x_dim, y_dim, z_dim, 0, GL_RED_INTEGER, GL_SHORT, &raw_data[0]);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, x_dim, y_dim, z_dim, 0, GL_RED, GL_FLOAT, &(volume_data[0]));

    // imgui
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450");
    bool show_another_window = true;
    ImGui::SetNextWindowSize(ImVec2(SCR_WIDTH, 100));

    ourShader.use();
    ourShader.setInt("volume", 0);
    ourShader.setVec3("cameraPos", camera.Position);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // input
        processInput(window);
        glfwPollEvents();

        // 体绘制
        #pragma region 
        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
        //glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, texture);
        // render the triangle
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 50.0f);
        ourShader.setMat4("projection", projection);

        // camera/view transformation
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("view", view);

        // model transformation
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));
        ourShader.setMat4("model", model);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        #pragma endregion

        // 更新帧率
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        updateFrameRate(window, deltaTime);

        // imgui
        #pragma region 
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Volume Rendering", &show_another_window);
        // 体纹理参数
        ImGui::ColorEdit3("color0", (float*)&color_transfer_value[0]);
        ImGui::ColorEdit3("color1", (float*)&color_transfer_value[1]);
        ImGui::ColorEdit3("color2", (float*)&color_transfer_value[2]);
        ImGui::ColorEdit3("color3", (float*)&color_transfer_value[3]);
        ImGui::SliderInt4("threshold", &color_transfer_threshold[0], -3000, 3000);
        ImGui::SliderFloat4("opacity", &opacity_transfer_value[0], 0.0f, 1.0f);
        ImGui::SliderInt4("opacity_threshold", &opacity_transfer_threshold[0], -3000, 3000);
        if (ImGui::Button("Set Transfer Function"))
            changeTransferFunction(ourShader);
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        #pragma endregion

        glfwSwapBuffers(window);
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        pitch += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        pitch -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        yaw += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        yaw -= 1.0f;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void updateFrameRate(GLFWwindow* window, double deltaTime) {
    static int frameCount = 0;
    static double elapsedTime = 0.0;
    
    frameCount++;
    elapsedTime += deltaTime;

    // 每秒更新一次帧率
    if (elapsedTime >= 1.0) {
        // 更新窗口标题，显示当前帧率
        std::string title = "Volume Rendering - FPS: " + std::to_string(frameCount);
        glfwSetWindowTitle(window, title.c_str());
        
        frameCount = 0;
        elapsedTime = 0.0;
    }
}

void changeTransferFunction(Shader& shader){
    shader.setVec4("color0", glm::vec4(color_transfer_value[0], (float)color_transfer_threshold[0]));
    shader.setVec4("color1", glm::vec4(color_transfer_value[1], (float)color_transfer_threshold[1]));
    shader.setVec4("color2", glm::vec4(color_transfer_value[2], (float)color_transfer_threshold[2]));
    shader.setVec4("color3", glm::vec4(color_transfer_value[3], (float)color_transfer_threshold[3]));
    shader.setVec2("opacity0", glm::vec2(opacity_transfer_value[0], (float)opacity_transfer_threshold[0]));
    shader.setVec2("opacity1", glm::vec2(opacity_transfer_value[1], (float)opacity_transfer_threshold[1]));
    shader.setVec2("opacity2", glm::vec2(opacity_transfer_value[2], (float)opacity_transfer_threshold[2]));
    shader.setVec2("opacity3", glm::vec2(opacity_transfer_value[3], (float)opacity_transfer_threshold[3]));
}