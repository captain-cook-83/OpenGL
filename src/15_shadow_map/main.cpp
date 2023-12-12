#define STB_IMAGE_IMPLEMENTATION

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <tool/shader.h>
#include <tool/stb_image.h>
#include <tool/gui.h>
#include <geometry/BoxGeometry.h>
#include <geometry/PlaneGeometry.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
unsigned int loadCubemap(vector<std::string> faces);
unsigned int loadTexture(const char* path, GLint internalFormat, GLenum format);
unsigned int createSkyBox();
unsigned int createInstanceBuffer(GLuint vao);
void updateViewAndProjection(glm::mat4 view, glm::mat4 projection, glm::mat4 lightVP);

// 天空盒
const vector<std::string> faces
{
    "./src/15_shadow_map/skybox/right.jpg",
    "./src/15_shadow_map/skybox/left.jpg",
    "./src/15_shadow_map/skybox/top.jpg",
    "./src/15_shadow_map/skybox/bottom.jpg",
    "./src/15_shadow_map/skybox/front.jpg",
    "./src/15_shadow_map/skybox/back.jpg"
};

const unsigned int SCR_WIDTH = 2880;
const unsigned int SCR_HEIGHT = 1620;

const unsigned int SHADOW_WIDTH = 1024 * 2;
const unsigned int SHADOW_HEIGHT = 1024 * 2;

const float perspectiveLightNear = 1.0f;
const float perspectiveLightFar = 30.0f;

const char* glsl_version = "#version 330";

int currentScrWidth;
int currentScrHeight;

unsigned int uboScreen;
unsigned int uboMatrices;
unsigned int uboLightCamera;

int main()
{
    glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);                                    // 设置 4x MSAA

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

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_MULTISAMPLE);                                       // 显示开启 MSAA

    glm::mat4 view;
    view = glm::rotate(glm::mat4(1.0f), float(glm::radians(25.0)), glm::vec3(1.0f, 0.0f, 0.0f));
    view = glm::translate(view, glm::vec3(0.0f, -3.0f, -6.0f) * 1.2f);
    view = glm::rotate(view, float(glm::radians(60.0)), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::vec3 lightDir = glm::vec3(-1.8f, 1.5f, -1.0f) * 5.0f;
    glm::mat4 orthoLightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 20.0f);
    glm::mat4 perspectiveLightProjection = glm::perspective(glm::radians(60.0f), 1.0f, perspectiveLightNear, perspectiveLightFar);
    glm::mat4 lightView = glm::lookAt(lightDir, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    
    unsigned int cubemapTexture = loadCubemap(faces);
    unsigned int texture0 = loadTexture("./src/15_shadow_map/container.png", GL_RGBA, GL_RGBA);
    unsigned int texture1 = loadTexture("./src/15_shadow_map/container_specular.png", GL_RGBA, GL_RGBA);
    unsigned int planeTexture = loadTexture("./src/15_shadow_map/wood.png", GL_RGB, GL_RGB);

    // 设置 Screen Uniform 缓冲对象
    glGenBuffers(1, &uboScreen);
    glBindBuffer(GL_UNIFORM_BUFFER, uboScreen);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(int), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboScreen);

    // 设置 Matrices Uniform 缓冲对象
    glGenBuffers(1, &uboMatrices);
    glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
    glBufferData(GL_UNIFORM_BUFFER, 3 * sizeof(glm::mat4), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, uboMatrices);

    // 设置 Matrices Uniform 缓冲对象
    glGenBuffers(1, &uboLightCamera);
    glBindBuffer(GL_UNIFORM_BUFFER, uboLightCamera);
    glBufferData(GL_UNIFORM_BUFFER, 3 * sizeof(float), NULL, GL_STATIC_DRAW);           // bool == float
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, uboLightCamera);

    framebuffer_size_callback(window, SCR_WIDTH, SCR_HEIGHT);

    Shader outShader("./src/15_shadow_map/shader/vertex.glsl", "./src/15_shadow_map/shader/fragment.glsl");
    unsigned int lightColorTransLoc = glGetUniformLocation(outShader.ID, "light.color");
    unsigned int lightDirTransLoc = glGetUniformLocation(outShader.ID, "light.dir");
    unsigned int ambientColorTransLoc = glGetUniformLocation(outShader.ID, "light.ambient");
    unsigned int shininessTransLoc = glGetUniformLocation(outShader.ID, "shininess");

    glUniformBlockBinding(outShader.ID, glGetUniformBlockIndex(outShader.ID, "Screen"), 0);
    glUniformBlockBinding(outShader.ID, glGetUniformBlockIndex(outShader.ID, "Matrices"), 1);
    glUniformBlockBinding(outShader.ID, glGetUniformBlockIndex(outShader.ID, "LightCamera"), 2);
    
    outShader.use();
    outShader.setInt("texture0", 0);
    outShader.setInt("texture1", 1);
    outShader.setInt("shadowMap", 2);
    glUniform3f(lightColorTransLoc, 1.0f, 1.0f, 1.0f);
    glUniform3f(ambientColorTransLoc, 0.4f, 0.4f, 0.5f);

    Shader planeShader("./src/15_shadow_map/shader/plane_vertex.glsl", "./src/15_shadow_map/shader/plane_fragment.glsl");
    unsigned int pLightDirTransLoc = glGetUniformLocation(planeShader.ID, "light.dir");
    unsigned int pShininessTransLoc = glGetUniformLocation(planeShader.ID, "shininess");
    glUniformBlockBinding(planeShader.ID, glGetUniformBlockIndex(planeShader.ID, "Matrices"), 1);
    glUniformBlockBinding(planeShader.ID, glGetUniformBlockIndex(planeShader.ID, "LightCamera"), 2);

    planeShader.use();
    planeShader.setInt("texture0", 0);
    planeShader.setInt("shadowMap", 1);
    glm::mat4 planeModel = glm::mat4(1.0f);
    planeModel = glm::rotate(planeModel, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    planeModel = glm::rotate(planeModel, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    planeModel = glm::translate(planeModel, glm::vec3(0.0, 0.0f, -1.0f));
    glUniform3f(glGetUniformLocation(planeShader.ID, "light.color"), 1.0f, 1.0f, 1.0f);
    glUniform3f(glGetUniformLocation(planeShader.ID, "light.ambient"), 0.4f, 0.4f, 0.5f);
    glUniformMatrix4fv(glGetUniformLocation(planeShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(planeModel));

    Shader skyboxShader("./src/15_shadow_map/shader/skybox_vertex.glsl", "./src/15_shadow_map/shader/skybox_fragment.glsl");
    glUniformBlockBinding(skyboxShader.ID, glGetUniformBlockIndex(skyboxShader.ID, "Matrices"), 1);

    PlaneGeometry planeGeometry(20.0f, 20.0f, 20.0f, 20.0f);
    BoxGeometry boxGeometry(1.0f, 1.0f, 1.0f);
    unsigned int instanceBuffer = createInstanceBuffer(boxGeometry.VAO);
    unsigned int skyboxVAO = createSkyBox();

    // 实例化数组
    int instanceNum = 7;
    glm::mat4 unitMatrix = glm::mat4(1.0f);
    glm::mat4 *cubeModelMatrixes = new glm::mat4[instanceNum]{
        glm::translate(unitMatrix, glm::vec3( 0.0f,  0.0f,  0.0f)), 
        glm::translate(unitMatrix, glm::vec3(-2.0f, 2.0f, 0.0f)),   
        glm::translate(unitMatrix, glm::vec3( 2.0f, 2.0f, 0.0f)),  
        glm::translate(unitMatrix, glm::vec3( 0.0f, 0.0f, 2.0f)),  
        glm::translate(unitMatrix, glm::vec3( 2.0f,  2.0f, 1.5f)), 
        glm::translate(unitMatrix, glm::vec3(-1.5f,  0.5f, -1.0f)),
        glm::translate(unitMatrix, glm::vec3(2.0f,  -0.5f, -1.5f))
    };

    // 准备阴影渲染
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);

    unsigned int depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);        // https://registry.khronos.org/OpenGL-Refpages/es3.0/html/glTexImage2D.xhtml

    float borderColor[] = { 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);   // 设置边框颜色，解决超出深度图采样时的问题
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);   // 设置超出范围时 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);          // 因为没有 Color Attachment，所以需要这两个声明，这样帧缓冲才被认为是完整的
    glReadBuffer(GL_NONE);          // 因为没有 Color Attachment，所以需要这两个声明，这样帧缓冲才被认为是完整的
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    Shader shadowShader("./src/15_shadow_map/shader/shadow_vertex.glsl", "./src/15_shadow_map/shader/shadow_fragment.glsl");
    unsigned int shadowMatrices = glGetUniformBlockIndex(shadowShader.ID, "Matrices");
    glUniformBlockBinding(shadowShader.ID, shadowMatrices, 1);

    // 配置参数
    int shininess = 32;
    float rotateSpeed = 0.5f;
    bool rotateBox = false;
    bool rotateView = false;
    bool perspectiveLight = false;

    while (!glfwWindowShouldClose(window))
	{
        processInput(window);

        glClearColor(0.14f, 0.16f, 0.16f, 1.0f);

        // 更新实例化数组
        const glm::vec3 rotateAxis = glm::vec3(1.0, 0.5, 0.0);
        glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
        for (int i = 0; i < instanceNum; i++)
        {
            glm::mat4 model = rotateBox ? glm::rotate(cubeModelMatrixes[i], float(i + glfwGetTime()), rotateAxis) : cubeModelMatrixes[i];
            glBufferSubData(GL_ARRAY_BUFFER, i * sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(model)); 
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // 渲染阴影深度图
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        glCullFace(GL_FRONT);                   // 使用正面剔除，来解决封闭几何体阴影（失真或悬浮）和问题
    
        glm::mat lightProjection = perspectiveLight ? perspectiveLightProjection : orthoLightProjection;
        glm::mat4 lightVP = lightProjection * lightView;
        updateViewAndProjection(lightView, lightProjection, lightVP);

        shadowShader.use();
        glBindVertexArray(boxGeometry.VAO);
        glDrawElementsInstanced(GL_TRIANGLES, boxGeometry.indices.size(), GL_UNSIGNED_INT, 0, instanceNum);

        glCullFace(GL_BACK);                    // 回复背面剔除
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 开始渲染场景
        glViewport(0, 0, currentScrWidth, currentScrHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 更新 Uniform 缓冲
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)currentScrWidth / (float)currentScrHeight, 0.1f, 100.0f);
        glm::mat4 dynamicView = rotateView ? glm::rotate(view, float(glfwGetTime() * rotateSpeed), glm::vec3(0.0f, 1.0f, 0.0f)) : view;
        glm::vec3 lightVDir = glm::vec3(dynamicView * glm::vec4(lightDir, 0.0f));
        updateViewAndProjection(dynamicView, projection, lightVP);

        glBindBuffer(GL_UNIFORM_BUFFER, uboLightCamera);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(bool), &perspectiveLight);
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(float), sizeof(float), &perspectiveLightNear);
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(float) + sizeof(float), sizeof(float), &perspectiveLightFar);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // 渲染平面
        planeShader.use();
        glUniform3fv(pLightDirTransLoc, 1, glm::value_ptr(lightVDir));
        glUniform1i(pShininessTransLoc, shininess * 4.0f);

        glActiveTexture(GL_TEXTURE0); 
        glBindTexture(GL_TEXTURE_2D, planeTexture);
        glActiveTexture(GL_TEXTURE1); 
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glBindVertexArray(planeGeometry.VAO);
        glDrawElements(GL_TRIANGLES, planeGeometry.indices.size(), GL_UNSIGNED_INT, 0);

        // 渲染场景
        outShader.use();
        glActiveTexture(GL_TEXTURE0); 
        glBindTexture(GL_TEXTURE_2D, texture0);
        glActiveTexture(GL_TEXTURE1); 
        glBindTexture(GL_TEXTURE_2D, texture1);
        glActiveTexture(GL_TEXTURE2); 
        glBindTexture(GL_TEXTURE_2D, depthMap);

        glUniform3fv(lightDirTransLoc, 1, glm::value_ptr(lightVDir));
        glUniform1i(shininessTransLoc, shininess);
        glBindVertexArray(boxGeometry.VAO);
        glDrawElementsInstanced(GL_TRIANGLES, boxGeometry.indices.size(), GL_UNSIGNED_INT, 0, instanceNum);

        // 渲染天空盒
        skyboxShader.use();
        glActiveTexture(GL_TEXTURE0); 
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

        glBindVertexArray(skyboxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // GUI 渲染
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Settings");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::SliderInt("Shininess", &shininess, 8.0f, 128.0f);
        ImGui::SliderFloat("Rotate Speed", &rotateSpeed, 0.1f, 1.0f);
        ImGui::Checkbox("Rotate Box", &rotateBox);
        ImGui::Checkbox("Rotate View", &rotateView);
        ImGui::Checkbox("Perspective Light", &perspectiveLight);
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        // 交换缓冲并查询IO事件
		glfwSwapBuffers(window);
		glfwPollEvents();
    }

    glBindVertexArray(0);
    glDeleteTextures(1, &texture0);
    glDeleteTextures(1, &texture1);
    glDeleteTextures(1, &planeTexture);
    glDeleteTextures(1, &cubemapTexture);

    planeGeometry.dispose();
    boxGeometry.dispose();

    glfwTerminate();
    return 0;
}

void updateViewAndProjection(glm::mat4 view, glm::mat4 projection, glm::mat4 lightVP)
{
    glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(view), glm::value_ptr(view));
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(view), sizeof(projection), glm::value_ptr(projection));
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(view) + sizeof(projection), sizeof(lightVP), glm::value_ptr(lightVP));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
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
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glViewport(0, 0, width, height);
}

unsigned int createSkyBox()
{
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

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    return skyboxVAO;
}

unsigned int createInstanceBuffer(GLuint vao)
{
    // 实例化数组
    int instanceNum = 10;
    glm::mat4 unitMatrix = glm::mat4(1.0f);
    glm::mat4 *cubeModelMatrixes = new glm::mat4[instanceNum]{
        glm::translate(unitMatrix, glm::vec3( 0.0f,  0.0f,  0.0f)), 
        glm::translate(unitMatrix, glm::vec3( 2.0f,  5.0f, -15.0f)), 
        glm::translate(unitMatrix, glm::vec3(-1.5f, -2.2f, -2.5f)),  
        glm::translate(unitMatrix, glm::vec3(-3.8f, -2.0f, -12.3f)),  
        glm::translate(unitMatrix, glm::vec3( 2.4f, -0.4f, -3.5f)),  
        glm::translate(unitMatrix, glm::vec3(-1.7f,  3.0f, -7.5f)),  
        glm::translate(unitMatrix, glm::vec3( 1.3f, -2.0f, -2.5f)),  
        glm::translate(unitMatrix, glm::vec3( 1.5f,  2.0f, -2.5f)), 
        glm::translate(unitMatrix, glm::vec3( 1.5f,  0.2f, -1.5f)), 
        glm::translate(unitMatrix, glm::vec3(-1.3f,  1.0f, -1.5f))  
    };

    GLsizei vec4Size = sizeof(glm::vec4);
    GLsizei mat4Size = sizeof(glm::mat4);

    unsigned int instanceBuffer;
    glGenBuffers(1, &instanceBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
    glBufferData(GL_ARRAY_BUFFER, instanceNum * mat4Size, NULL, GL_DYNAMIC_DRAW);

    glBindVertexArray(vao);

    // 顶点属性最大允许的数据大小等于一个 vec4。因为一个 mat4 本质上是 4 个 vec4，我们需要为这个矩阵预留 4 个顶点属性。
    glEnableVertexAttribArray(3); 
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, mat4Size, (void*)0);
    glEnableVertexAttribArray(4); 
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, mat4Size, (void*)(vec4Size));
    glEnableVertexAttribArray(5); 
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, mat4Size, (void*)(2 * vec4Size));
    glEnableVertexAttribArray(6); 
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, mat4Size, (void*)(3 * vec4Size));
    glVertexAttribDivisor(3, 1);    // 默认情况下，属性除数是 0，顶点着色器的每次迭代时更新顶点属性。1，渲染一个新实例的时候更新顶点属性。2，每2个实例更新一次属性，以此类推。
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);
    glVertexAttribDivisor(6, 1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return instanceBuffer;
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

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

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return texture;
}

unsigned int loadTexture(const char* path, GLint internalFormat, GLenum format)
{
    // 生成纹理0
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // 为当前绑定的纹理对象设置环绕、过滤方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 加载贴图
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    unsigned char *imageData = stbi_load(path, &width, &height, &nrChannels, 0);
    if (imageData)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, imageData);
        glGenerateMipmap(GL_TEXTURE_2D);            // 手动开启多级渐远纹理，或者直接使用上面的 glTexImage2D(GL_TEXTURE_2D, n, ...)
    } 
    else 
    {
        std::cout << "Failed to load texture0" << std::endl;
    }
    stbi_image_free(imageData);

    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}