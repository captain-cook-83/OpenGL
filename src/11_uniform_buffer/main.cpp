#define STB_IMAGE_IMPLEMENTATION

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <tool/shader.h>
#include <tool/stb_image.h>
#include <tool/gui.h>
#include <geometry/BoxGeometry.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
unsigned int loadCubemap(vector<std::string> faces);
glm::mat4 caculateProjection();

const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 1200;

const char* glsl_version = "#version 330";

int currentScrWidth;
int currentScrHeight;

unsigned int uboScreen;
unsigned int uboMatrices;

int main()
{
    glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

    // 初始化 IMGUI 上下文
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    BoxGeometry boxGeometry(1.0f, 1.0f, 1.0f);

    unsigned int texture0, texture1;

    // 生成纹理0
    glGenTextures(1, &texture0);
    glBindTexture(GL_TEXTURE_2D, texture0);

    // 为当前绑定的纹理对象设置环绕、过滤方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 加载贴图
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    unsigned char *imageData = stbi_load("./src/11_uniform_buffer/container.png", &width, &height, &nrChannels, 0);
    if (imageData)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
        glGenerateMipmap(GL_TEXTURE_2D);            // 手动开启多级渐远纹理，或者直接使用上面的 glTexImage2D(GL_TEXTURE_2D, n, ...)
    } 
    else 
    {
        std::cout << "Failed to load texture0" << std::endl;
    }
    stbi_image_free(imageData);

    // 生成纹理1
    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);

    // 为当前绑定的纹理对象设置环绕、过滤方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 加载贴图
    stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    imageData = stbi_load("./src/11_uniform_buffer/container_specular.png", &width, &height, &nrChannels, 0);
    if (imageData)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
        glGenerateMipmap(GL_TEXTURE_2D);            // 手动开启多级渐远纹理，或者直接使用上面的 glTexImage2D(GL_TEXTURE_2D, n, ...)
    } 
    else 
    {
        std::cout << "Failed to load texture1" << std::endl;
    }
    stbi_image_free(imageData);

    glm::vec3 cubePositions[] = {
        glm::vec3( 0.0f,  0.0f,  0.0f), 
        glm::vec3( 2.0f,  5.0f, -15.0f), 
        glm::vec3(-1.5f, -2.2f, -2.5f),  
        glm::vec3(-3.8f, -2.0f, -12.3f),  
        glm::vec3( 2.4f, -0.4f, -3.5f),  
        glm::vec3(-1.7f,  3.0f, -7.5f),  
        glm::vec3( 1.3f, -2.0f, -2.5f),  
        glm::vec3( 1.5f,  2.0f, -2.5f), 
        glm::vec3( 1.5f,  0.2f, -1.5f), 
        glm::vec3(-1.3f,  1.0f, -1.5f)  
    };

    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    // glFrontFace(GL_CW);          // 告诉OpenGL现在顺时针顺序代表的是正向面

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // 天空和
    vector<std::string> faces
    {
        "./src/11_uniform_buffer/skybox/right.jpg",
        "./src/11_uniform_buffer/skybox/left.jpg",
        "./src/11_uniform_buffer/skybox/top.jpg",
        "./src/11_uniform_buffer/skybox/bottom.jpg",
        "./src/11_uniform_buffer/skybox/front.jpg",
        "./src/11_uniform_buffer/skybox/back.jpg"
    };
    unsigned int cubemapTexture = loadCubemap(faces);

    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // 设置 Screen Uniform 缓冲对象
    glGenBuffers(1, &uboScreen);
    glBindBuffer(GL_UNIFORM_BUFFER, uboScreen);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(int), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboScreen);

    // 设置 Matrices Uniform 缓冲对象
    int ubouboMatricesSize = 2 * sizeof(glm::mat4);
    glGenBuffers(1, &uboMatrices);
    glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
    glBufferData(GL_UNIFORM_BUFFER, ubouboMatricesSize, NULL, GL_STATIC_DRAW);
    
    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -4.0f));
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(view), glm::value_ptr(view));
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(view) + sizeof(glm::mat4), sizeof(unsigned int), &cubemapTexture);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, uboMatrices);
    // glBindBufferRange(GL_UNIFORM_BUFFER, 1, uboMatrices, 0, ubouboMatricesSize);

    Shader outShader("./src/11_uniform_buffer/shader/vertex.glsl", "./src/11_uniform_buffer/shader/fragment.glsl");
    unsigned int modelTransLoc = glGetUniformLocation(outShader.ID, "model");
    unsigned int lightColorTransLoc = glGetUniformLocation(outShader.ID, "light.color");
    unsigned int lightVPosTransLoc = glGetUniformLocation(outShader.ID, "light.position");
    unsigned int ambientColorTransLoc = glGetUniformLocation(outShader.ID, "light.ambient");
    unsigned int shininessTransLoc = glGetUniformLocation(outShader.ID, "shininess");

    outShader.use();
    outShader.setFloat("light.constant", 1.0f);
    outShader.setFloat("light.linear", 0.14f);
    outShader.setFloat("light.quadratic", 0.07f);
    outShader.setInt("texture0", 0);
    outShader.setInt("texture1", 1);
    outShader.setInt("skybox", 2);

    glActiveTexture(GL_TEXTURE0); 
    glBindTexture(GL_TEXTURE_2D, texture0);
    glActiveTexture(GL_TEXTURE1); 
    glBindTexture(GL_TEXTURE_2D, texture1);
    glActiveTexture(GL_TEXTURE2); 
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    unsigned int outScreen = glGetUniformBlockIndex(outShader.ID, "Screen");
    glUniformBlockBinding(outShader.ID, outScreen, 0);

    unsigned int outMatrices = glGetUniformBlockIndex(outShader.ID, "Matrices");
    glUniformBlockBinding(outShader.ID, outMatrices, 1);

    Shader skyboxShader("./src/11_uniform_buffer/shader/skybox_vertex.glsl", "./src/11_uniform_buffer/shader/skybox_fragment.glsl");
    skyboxShader.use();
    glActiveTexture(GL_TEXTURE0); 
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    unsigned int skyboxMatrices = glGetUniformBlockIndex(skyboxShader.ID, "Matrices");
    glUniformBlockBinding(skyboxShader.ID, skyboxMatrices, 1);

    glm::vec3 lightPos = glm::vec3(-1.2f, 1.0f, -1.5f);

    int shininess = 32;
    ImVec4 ambientColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    ImVec4 lightColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

    framebuffer_size_callback(window, SCR_WIDTH, SCR_HEIGHT);
    
    while (!glfwWindowShouldClose(window))
	{
        processInput(window);

        glm::mat4 dynamicView = glm::rotate(view, float(glfwGetTime()), glm::vec3(0.0f, 1.0f, 0.0f));
        glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(dynamicView), glm::value_ptr(dynamicView));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glClearColor(0.14f, 0.16f, 0.16f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        outShader.use();
    
        glm::vec3 lightVPos = glm::vec3(dynamicView * glm::vec4(lightPos, 1.0f));
        glUniform3fv(lightVPosTransLoc, 1, glm::value_ptr(lightVPos));
        glUniform3f(lightColorTransLoc, lightColor.x, lightColor.y, lightColor.z);
        glUniform3f(ambientColorTransLoc, ambientColor.x, ambientColor.y, ambientColor.z);
        glUniform1i(shininessTransLoc, shininess);
        
        glBindVertexArray(boxGeometry.VAO);
        for(unsigned int i = 0; i < 10; i++)
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), cubePositions[i]);
            model = glm::rotate(model, float(i + glfwGetTime()), glm::vec3(1.0, 0.5, 0.0));
            glUniformMatrix4fv(modelTransLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawElements(GL_TRIANGLES, boxGeometry.indices.size(), GL_UNSIGNED_INT, 0);
        }

        // 渲染天空盒
        skyboxShader.use();
        glBindVertexArray(skyboxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // GUI 渲染
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Settings");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::SliderInt("Shininess", &shininess, 8.0f, 128.0f);
        ImGui::ColorEdit3("Ambient Color", (float*)&ambientColor);
        ImGui::ColorEdit3("Light Color", (float*)&lightColor); 
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        // 交换缓冲并查询IO事件
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

    glBindVertexArray(0);

    boxGeometry.dispose();

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    currentScrWidth = width;
    currentScrHeight = height;

    glBindBuffer(GL_UNIFORM_BUFFER, uboScreen);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(currentScrWidth), &currentScrWidth);
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(currentScrWidth), sizeof(currentScrHeight), &currentScrHeight);

    glm::mat4 projection = caculateProjection();
    glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(projection), glm::value_ptr(projection));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glViewport(0, 0, width, height);
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(false);
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

glm::mat4 caculateProjection()
{
    return glm::perspective(glm::radians(45.0f), (float)currentScrWidth / (float)currentScrHeight, 0.1f, 100.0f);
}