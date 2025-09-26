// ===========================
// OpenGL Rectangular Prism from Screen Coordinates
// Using: GLFW, GLEW, GLM, custom Shader class
// ===========================

#include <iostream>
#include <vector>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Shader.h"

const GLuint WIDTH = 702, HEIGHT = 1062;

glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

glm::vec3 initialCameraPos   = cameraPos;
glm::vec3 initialCameraFront = cameraFront;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

float screenToNDC_X(float x) { return (2.0f * x / WIDTH) - 1.0f; }
float screenToNDC_Y(float y) { return 1.0f - (2.0f * y / HEIGHT); }

std::vector<GLfloat> createPrismVertices(const std::vector<std::pair<int,int>>& corners, float zFront, float zBack)
{
    if(corners.size() != 4)
    {
        std::cerr << "Error: corners must have exactly 4 points." << std::endl;
        return {};
    }

    float x0 = screenToNDC_X(corners[0].first);
    float y0 = screenToNDC_Y(corners[0].second);
    float x1 = screenToNDC_X(corners[1].first);
    float y1 = screenToNDC_Y(corners[1].second);
    float x2 = screenToNDC_X(corners[2].first);
    float y2 = screenToNDC_Y(corners[2].second);
    float x3 = screenToNDC_X(corners[3].first);
    float y3 = screenToNDC_Y(corners[3].second);

    std::vector<GLfloat> vertices = {
        x0, y0, zFront,  x1, y1, zFront,  x2, y2, zFront,
        x2, y2, zFront,  x3, y3, zFront,  x0, y0, zFront,

        x0, y0, zBack,   x1, y1, zBack,   x2, y2, zBack,
        x2, y2, zBack,   x3, y3, zBack,   x0, y0, zBack,

        x0, y0, zFront,  x0, y0, zBack,   x3, y3, zBack,
        x3, y3, zBack,   x3, y3, zFront,  x0, y0, zFront,

        x1, y1, zFront,  x1, y1, zBack,   x2, y2, zBack,
        x2, y2, zBack,   x2, y2, zFront,  x1, y1, zFront,

        x0, y0, zFront,  x1, y1, zFront,  x1, y1, zBack,
        x1, y1, zBack,   x0, y0, zBack,   x0, y0, zFront,

        x3, y3, zFront,  x2, y2, zFront,  x2, y2, zBack,
        x2, y2, zBack,   x3, y3, zBack,   x3, y3, zFront
    };

    return vertices;
}

// Helper to convert 0-255 RGB to glm::vec4 (RGBA)
glm::vec4 rgb255(int r, int g, int b, int a = 255) {
    return glm::vec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Prisms", nullptr, nullptr);
    if(!window){ std::cout<<"Failed to create window\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window,key_callback);
    glfwSetScrollCallback(window,scroll_callback);

    glewExperimental = GL_TRUE;
    if(glewInit()!=GLEW_OK){ std::cout<<"Failed to initialize GLEW\n"; return -1; }

    glViewport(0,0,WIDTH,HEIGHT);
    glEnable(GL_DEPTH_TEST);

    Shader shader("basic.vs","basic.frag");

    // --- Glasses Case (on top of Bible) ---
    std::vector<std::pair<int,int>> corners1 = {
    {400, 528}, {480, 528}, {480, 560}, {400, 560}
    };

    glm::vec4 color1 = rgb255(105,105,107); // gray
    std::vector<GLfloat> vertices1 = createPrismVertices(corners1,-0.5f,-0.6f);

    GLuint VAO1,VBO1;
    glGenVertexArrays(1,&VAO1);
    glGenBuffers(1,&VBO1);
    glBindVertexArray(VAO1);
    glBindBuffer(GL_ARRAY_BUFFER,VBO1);
    glBufferData(GL_ARRAY_BUFFER,vertices1.size()*sizeof(GLfloat),vertices1.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(GLfloat),(GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // --- Bible ---
    std::vector<std::pair<int,int>> corners2 = {
    {380, 560}, {505, 560}, {505, 582}, {380, 582}
    };

    glm::vec4 color2 = rgb255(150,153,149);// light gray
    std::vector<GLfloat> vertices2 = createPrismVertices(corners2,-0.5f,-0.8f);

    GLuint VAO2,VBO2;
    glGenVertexArrays(1,&VAO2);
    glGenBuffers(1,&VBO2);
    glBindVertexArray(VAO2);
    glBindBuffer(GL_ARRAY_BUFFER,VBO2);
    glBufferData(GL_ARRAY_BUFFER,vertices2.size()*sizeof(GLfloat),vertices2.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(GLfloat),(GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);


    // --- Wall ---
    glm::vec4 color3 = rgb255(233, 227, 213); // cream
    std::vector<GLfloat> vertices3 = {
        -1.0f, -1.0f, 0.0f,  // bottom-left
        1.0f, -1.0f, 0.0f,   // bottom-right
        1.0f,  1.0f, 0.0f,   // top-right

        1.0f,  1.0f, 0.0f,   // top-right
        -1.0f,  1.0f, 0.0f,  // top-left
        -1.0f, -1.0f, 0.0f   // bottom-left
    };

    GLuint VAO3, VBO3;
    glGenVertexArrays(1, &VAO3);
    glGenBuffers(1, &VBO3);
    glBindVertexArray(VAO3);
    glBindBuffer(GL_ARRAY_BUFFER, VBO3);
    glBufferData(GL_ARRAY_BUFFER, vertices3.size() * sizeof(GLfloat), vertices3.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // --- DVD Player Pt1 ---
    std::vector<std::pair<int,int>> corners4 = {
    {356, 662}, {454, 662}, {454, 695}, {356, 695}
    };


    glm::vec4 color4 = rgb255(43, 43, 41); // light gray
    std::vector<GLfloat> vertices4 = createPrismVertices(corners4,-0.6f,-1.0f);

    GLuint VAO4,VBO4;
    glGenVertexArrays(1,&VAO4);
    glGenBuffers(1,&VBO4);
    glBindVertexArray(VAO4);
    glBindBuffer(GL_ARRAY_BUFFER,VBO4);
    glBufferData(GL_ARRAY_BUFFER,vertices4.size()*sizeof(GLfloat),vertices4.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(GLfloat),(GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // --- DVD Player Pt2 (flush with Pt1) ---
    std::vector<std::pair<int,int>> corners5 = {
        {454, 662}, {487, 662}, {487, 695}, {454, 695}
    };

    glm::vec4 color5 = rgb255(10, 10, 10); // near black
    std::vector<GLfloat> vertices5 = createPrismVertices(corners5,-0.6f,-1.0f);

    GLuint VAO5,VBO5;
    glGenVertexArrays(1,&VAO5);
    glGenBuffers(1,&VBO5);
    glBindVertexArray(VAO5);
    glBindBuffer(GL_ARRAY_BUFFER,VBO5);
    glBufferData(GL_ARRAY_BUFFER,vertices5.size()*sizeof(GLfloat),vertices5.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(GLfloat),(GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Cabinet Top ---
    std::vector<std::pair<int,int>> corners6 = {
    {0, 582}, {702, 582}, {702, 594}, {0, 594}
    };

    glm::vec4 color6 = rgb255(70, 46, 29); // brown (will add texture)
    std::vector<GLfloat> vertices6 = createPrismVertices(corners6,-0.5f,-1.0f);

    GLuint VAO6,VBO6;
    glGenVertexArrays(1,&VAO6);
    glGenBuffers(1,&VBO6);
    glBindVertexArray(VAO6);
    glBindBuffer(GL_ARRAY_BUFFER,VBO6);
    glBufferData(GL_ARRAY_BUFFER,vertices6.size()*sizeof(GLfloat),vertices6.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(GLfloat),(GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Cabinet Bottom Base ---
    std::vector<std::pair<int,int>> corners7 = {
        {0, 880}, {702,880}, {702,906}, {0,906}
    };
    glm::vec4 color7 = rgb255(27, 26, 24); // black base
    std::vector<GLfloat> vertices7 = createPrismVertices(corners7,-0.5f,-1.0f);

    GLuint VAO7,VBO7;
    glGenVertexArrays(1,&VAO7);
    glGenBuffers(1,&VBO7);
    glBindVertexArray(VAO7);
    glBindBuffer(GL_ARRAY_BUFFER,VBO7);
    glBufferData(GL_ARRAY_BUFFER,vertices7.size()*sizeof(GLfloat),vertices7.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(GLfloat),(GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Cabinet Base Support 1 ---
    std::vector<std::pair<int,int>> corners8 = {
    {201, 594}, {215, 594}, {215, 880}, {201, 880}
    };

    glm::vec4 color8 = rgb255(27, 26, 24); // black base
    std::vector<GLfloat> vertices8 = createPrismVertices(corners8,-0.5f,-1.0f);

    GLuint VAO8,VBO8;
    glGenVertexArrays(1,&VAO8);
    glGenBuffers(1,&VBO8);
    glBindVertexArray(VAO8);
    glBindBuffer(GL_ARRAY_BUFFER,VBO8);
    glBufferData(GL_ARRAY_BUFFER,vertices8.size()*sizeof(GLfloat),vertices8.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(GLfloat),(GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Cabinet Base Support 2 (mirrored on right side)
    std::vector<std::pair<int,int>> corners9 = {
        {487, 594}, {501, 594}, {501, 880}, {487, 880}
    };

    glm::vec4 color9 = rgb255(27, 26, 24); // same black base
    std::vector<GLfloat> vertices9 = createPrismVertices(corners9,-0.5f,-1.0f);

    GLuint VAO9,VBO9;
    glGenVertexArrays(1,&VAO9);
    glGenBuffers(1,&VBO9);
    glBindVertexArray(VAO9);
    glBindBuffer(GL_ARRAY_BUFFER,VBO9);
    glBufferData(GL_ARRAY_BUFFER,vertices9.size()*sizeof(GLfloat),vertices9.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(GLfloat),(GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Cabinet Back
    std::vector<std::pair<int,int>> corners10 = {
        {0, 594}, {702, 594}, {702, 906}, {0, 906}
    };

    glm::vec4 color10 = rgb255(6, 6, 6); // same black base
    std::vector<GLfloat> vertices10 = createPrismVertices(corners10,-0.9f,-1.0f);

    GLuint VAO10,VBO10;
    glGenVertexArrays(1,&VAO10);
    glGenBuffers(1,&VBO10);
    glBindVertexArray(VAO10);
    glBindBuffer(GL_ARRAY_BUFFER,VBO10);
    glBufferData(GL_ARRAY_BUFFER,vertices10.size()*sizeof(GLfloat),vertices10.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(GLfloat),(GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // --- Shelf under DVD player ---
    std::vector<std::pair<int,int>> corners11 = {
        {201, 695}, {501, 695}, {501, 705}, {201, 705}
    };

    glm::vec4 color11 = rgb255(21, 18, 18); // dark shelf
    std::vector<GLfloat> vertices11 = createPrismVertices(corners11, -0.5f, -1.0f);

    GLuint VAO11, VBO11;
    glGenVertexArrays(1, &VAO11);
    glGenBuffers(1, &VBO11);
    glBindVertexArray(VAO11);
    glBindBuffer(GL_ARRAY_BUFFER, VBO11);
    glBufferData(GL_ARRAY_BUFFER, vertices11.size() * sizeof(GLfloat), vertices11.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // --- TV ---
    std::vector<std::pair<int,int>> corners12 = {
        {143,180}, {625,180}, {625,554}, {143,554}
    };

    glm::vec4 color12 = rgb255(21, 18, 18); // TV Screen 
    std::vector<GLfloat> vertices12 = createPrismVertices(corners12, -0.9f, -1.0f);

    GLuint VAO12, VBO12;
    glGenVertexArrays(1, &VAO12);
    glGenBuffers(1, &VBO12);
    glBindVertexArray(VAO12);
    glBindBuffer(GL_ARRAY_BUFFER, VBO12);
    glBufferData(GL_ARRAY_BUFFER, vertices12.size() * sizeof(GLfloat), vertices12.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // --- TV boarder ---
    std::vector<std::pair<int,int>> corners13 = {
        {143,554}, {625,554}, {625,544}, {143,544}
    };

    glm::vec4 color13 = rgb255(41, 40, 38); // TV Boarder
    std::vector<GLfloat> vertices13 = createPrismVertices(corners13, -0.88, -0.9f);

    GLuint VAO13, VBO13;
    glGenVertexArrays(1, &VAO13);
    glGenBuffers(1, &VBO13);
    glBindVertexArray(VAO13);
    glBindBuffer(GL_ARRAY_BUFFER, VBO13);
    glBufferData(GL_ARRAY_BUFFER, vertices13.size() * sizeof(GLfloat), vertices13.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // --- Switch case ---
    std::vector<std::pair<int,int>> corners14 = {
        {11,518}, {171,518}, {171,582}, {11,582}
    };

    glm::vec4 color14 = rgb255(125, 122, 113); // Switch Case
    std::vector<GLfloat> vertices14 = createPrismVertices(corners14, -0.6, -0.8f);

    GLuint VAO14, VBO14;
    glGenVertexArrays(1, &VAO14);
    glGenBuffers(1, &VBO14);
    glBindVertexArray(VAO14);
    glBindBuffer(GL_ARRAY_BUFFER, VBO14);
    glBufferData(GL_ARRAY_BUFFER, vertices14.size() * sizeof(GLfloat), vertices14.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // --- Switch case zipper ---
    std::vector<std::pair<int,int>> corners15 = {
        {7,545}, {175,545}, {175,559}, {7,559}
    };

    glm::vec4 color15 = rgb255(39, 35, 34); // Zipper Color
    std::vector<GLfloat> vertices15 = createPrismVertices(corners15, -0.55, -0.81f);

    GLuint VAO15, VBO15;
    glGenVertexArrays(1, &VAO15);
    glGenBuffers(1, &VBO15);
    glBindVertexArray(VAO15);
    glBindBuffer(GL_ARRAY_BUFFER, VBO15);
    glBufferData(GL_ARRAY_BUFFER, vertices15.size() * sizeof(GLfloat), vertices15.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // --- Waterbottle ---
    std::vector<std::pair<int,int>> corners16 = {
        {67,451}, {113,451}, {113,582}, {67,582}
    };

    glm::vec4 color16 = rgb255(80, 52, 125); // Purple
    std::vector<GLfloat> vertices16 = createPrismVertices(corners16, -0.8, -0.9f);

    GLuint VAO16, VBO16;
    glGenVertexArrays(1, &VAO16);
    glGenBuffers(1, &VBO16);
    glBindVertexArray(VAO16);
    glBindBuffer(GL_ARRAY_BUFFER, VBO16);
    glBufferData(GL_ARRAY_BUFFER, vertices16.size() * sizeof(GLfloat), vertices16.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // --- Waterbottle neck ---
    std::vector<std::pair<int,int>> corners17 = {
        {72,431}, {108,431}, {108,451}, {72,451}
    };

    glm::vec4 color17 = rgb255(80, 52, 125); // Purple
    std::vector<GLfloat> vertices17 = createPrismVertices(corners17, -0.8, -0.9f);

    GLuint VAO17, VBO17;
    glGenVertexArrays(1, &VAO17);
    glGenBuffers(1, &VBO17);
    glBindVertexArray(VAO17);
    glBindBuffer(GL_ARRAY_BUFFER, VBO17);
    glBufferData(GL_ARRAY_BUFFER, vertices17.size() * sizeof(GLfloat), vertices17.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // --- Waterbottle silver ring ---
    std::vector<std::pair<int,int>> corners18 = {
        {72,426}, {108,426}, {108,431}, {72,431}
    };

    glm::vec4 color18 = rgb255(135, 127, 133); // silver
    std::vector<GLfloat> vertices18 = createPrismVertices(corners18, -0.8, -0.9f);

    GLuint VAO18, VBO18;
    glGenVertexArrays(1, &VAO18);
    glGenBuffers(1, &VBO18);
    glBindVertexArray(VAO18);
    glBindBuffer(GL_ARRAY_BUFFER, VBO18);
    glBufferData(GL_ARRAY_BUFFER, vertices18.size() * sizeof(GLfloat), vertices18.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // --- Waterbottle Lid ---
    std::vector<std::pair<int,int>> corners19 = {
        {72,396}, {108,396}, {108,426}, {72,426}
    };

    glm::vec4 color19 = rgb255(30, 27, 28); // black
    std::vector<GLfloat> vertices19 = createPrismVertices(corners19, -0.8, -0.9f);

    GLuint VAO19, VBO19;
    glGenVertexArrays(1, &VAO19);
    glGenBuffers(1, &VBO19);
    glBindVertexArray(VAO19);
    glBindBuffer(GL_ARRAY_BUFFER, VBO19);
    glBufferData(GL_ARRAY_BUFFER, vertices19.size() * sizeof(GLfloat), vertices19.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Cabinet Base Support 3 ---
    std::vector<std::pair<int,int>> corners20 = {
    {0, 594}, {14, 594}, {14, 880}, {0, 880}
    };

    glm::vec4 color20 = rgb255(27, 26, 24); // black base
    std::vector<GLfloat> vertices20 = createPrismVertices(corners20,-0.5f,-1.0f);

    GLuint VAO20,VBO20;
    glGenVertexArrays(1,&VAO20);
    glGenBuffers(1,&VBO20);
    glBindVertexArray(VAO20);
    glBindBuffer(GL_ARRAY_BUFFER,VBO20);
    glBufferData(GL_ARRAY_BUFFER,vertices20.size()*sizeof(GLfloat),vertices20.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(GLfloat),(GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Cabinet Base Support 4 ---
    std::vector<std::pair<int,int>> corners21 = {
    {688, 594}, {702, 594}, {702, 880}, {688, 880}
    };

    glm::vec4 color21 = rgb255(27, 26, 24); // black base
    std::vector<GLfloat> vertices21 = createPrismVertices(corners21,-0.5f,-1.0f);

    GLuint VAO21,VBO21;
    glGenVertexArrays(1,&VAO21);
    glGenBuffers(1,&VBO21);
    glBindVertexArray(VAO21);
    glBindBuffer(GL_ARRAY_BUFFER,VBO21);
    glBufferData(GL_ARRAY_BUFFER,vertices21.size()*sizeof(GLfloat),vertices21.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(GLfloat),(GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // switch dock ---
    std::vector<std::pair<int,int>> corners22 = {
    {562, 529}, {658, 529}, {658, 582}, {562, 582}
    };

    glm::vec4 color22 = rgb255(227, 224, 215); // white
    std::vector<GLfloat> vertices22 = createPrismVertices(corners22,-0.5f,-0.7f);

    GLuint VAO22,VBO22;
    glGenVertexArrays(1,&VAO22);
    glGenBuffers(1,&VBO22);
    glBindVertexArray(VAO22);
    glBindBuffer(GL_ARRAY_BUFFER,VBO22);
    glBufferData(GL_ARRAY_BUFFER,vertices22.size()*sizeof(GLfloat),vertices22.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(GLfloat),(GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // switch ---
    std::vector<std::pair<int,int>> corners23 = {
    {562, 514}, {658, 514}, {658, 572}, {562, 572}
    };

    glm::vec4 color23 = rgb255(33, 33, 33); // black
    std::vector<GLfloat> vertices23 = createPrismVertices(corners23,-0.55f,-0.65f);

    GLuint VAO23,VBO23;
    glGenVertexArrays(1,&VAO23);
    glGenBuffers(1,&VBO23);
    glBindVertexArray(VAO23);
    glBindBuffer(GL_ARRAY_BUFFER,VBO23);
    glBufferData(GL_ARRAY_BUFFER,vertices23.size()*sizeof(GLfloat),vertices23.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(GLfloat),(GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Joy Con L ---
    std::vector<std::pair<int,int>> corners24 = {
    {547, 514}, {562, 514}, {562, 572}, {547, 572}
    };

    glm::vec4 color24 = rgb255(253, 58, 65); // red
    std::vector<GLfloat> vertices24 = createPrismVertices(corners24,-0.55f,-0.65f);

    GLuint VAO24,VBO24;
    glGenVertexArrays(1,&VAO24);
    glGenBuffers(1,&VBO24);
    glBindVertexArray(VAO24);
    glBindBuffer(GL_ARRAY_BUFFER,VBO24);
    glBufferData(GL_ARRAY_BUFFER,vertices24.size()*sizeof(GLfloat),vertices24.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(GLfloat),(GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Joy Con R ---
    std::vector<std::pair<int,int>> corners25 = {
    {658, 514}, {673, 514}, {673, 572}, {658, 572}
    };

    glm::vec4 color25 = rgb255(4, 135, 183); // blue
    std::vector<GLfloat> vertices25 = createPrismVertices(corners25,-0.55f,-0.65f);

    GLuint VAO25,VBO25;
    glGenVertexArrays(1,&VAO25);
    glGenBuffers(1,&VBO25);
    glBindVertexArray(VAO25);
    glBindBuffer(GL_ARRAY_BUFFER,VBO25);
    glBufferData(GL_ARRAY_BUFFER,vertices25.size()*sizeof(GLfloat),vertices25.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(GLfloat),(GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // tv leg 1 ---
    std::vector<std::pair<int,int>> corners26 = {
    {203, 544}, {215, 544}, {199, 582}, {193, 582},
    };

    glm::vec4 color26 = rgb255(33, 33, 33); // black
    std::vector<GLfloat> vertices26 = createPrismVertices(corners26,-0.9f,-1.0f);

    GLuint VAO26,VBO26;
    glGenVertexArrays(1,&VAO26);
    glGenBuffers(1,&VBO26);
    glBindVertexArray(VAO26);
    glBindBuffer(GL_ARRAY_BUFFER,VBO26);
    glBufferData(GL_ARRAY_BUFFER,vertices26.size()*sizeof(GLfloat),vertices26.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(GLfloat),(GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // tv leg 2 ---
    std::vector<std::pair<int,int>> corners27 = {
    {203, 544}, {215, 544}, {224, 582}, {219, 582}
    };

    glm::vec4 color27 = rgb255(33, 33, 33); // black
    std::vector<GLfloat> vertices27 = createPrismVertices(corners27,-0.9f,-1.0f);

    GLuint VAO27,VBO27;
    glGenVertexArrays(1,&VAO27);
    glGenBuffers(1,&VBO27);
    glBindVertexArray(VAO27);
    glBindBuffer(GL_ARRAY_BUFFER,VBO27);
    glBufferData(GL_ARRAY_BUFFER,vertices27.size()*sizeof(GLfloat),vertices27.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(GLfloat),(GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);


    // --- Render loop ---
    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Camera movement
        float cameraSpeed = 0.01f;      // forward/backward speed
        float strafeSpeed = cameraSpeed * 0.5f;  // left/right speed is half

        glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));

        if(glfwGetKey(window,GLFW_KEY_W)==GLFW_PRESS) cameraPos += cameraSpeed * cameraFront;
        if(glfwGetKey(window,GLFW_KEY_S)==GLFW_PRESS) cameraPos -= cameraSpeed * cameraFront;
        if(glfwGetKey(window,GLFW_KEY_A)==GLFW_PRESS) cameraPos -= strafeSpeed * right;
        if(glfwGetKey(window,GLFW_KEY_D)==GLFW_PRESS) cameraPos += strafeSpeed * right;

        cameraPos.y = glm::clamp(cameraPos.y, -5.0f, 5.0f);

        // Clear screen
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.Use();

        // --- Draw wall first ---
        glDisable(GL_DEPTH_TEST); // ensure wall is always in back
        glm::mat4 identity = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "model"), 1, GL_FALSE, glm::value_ptr(identity));
        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "view"), 1, GL_FALSE, glm::value_ptr(identity));
        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(identity));

        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color3));
        glBindVertexArray(VAO3);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST); // re-enable for 3D objects

        // --- Draw 3D objects ---
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "model"), 1, GL_FALSE, glm::value_ptr(identity));

        // Draw glasses case
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color1));
        glBindVertexArray(VAO1);
        glDrawArrays(GL_TRIANGLES, 0, vertices1.size()/3);
        glBindVertexArray(0);

        // Draw bible
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color2));
        glBindVertexArray(VAO2);
        glDrawArrays(GL_TRIANGLES, 0, vertices2.size()/3);
        glBindVertexArray(0);

        // Draw dvd player pt1
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color4));
        glBindVertexArray(VAO4);
        glDrawArrays(GL_TRIANGLES, 0, vertices4.size()/3);
        glBindVertexArray(0);

        // Draw dvd player pt2
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color5));
        glBindVertexArray(VAO5);
        glDrawArrays(GL_TRIANGLES, 0, vertices5.size()/3);
        glBindVertexArray(0);

        // Draw Cabinet Top
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color6));
        glBindVertexArray(VAO6);
        glDrawArrays(GL_TRIANGLES, 0, vertices6.size()/3);
        glBindVertexArray(0);

        // Draw Cabinet Base
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color7));
        glBindVertexArray(VAO7);
        glDrawArrays(GL_TRIANGLES, 0, vertices7.size()/3);
        glBindVertexArray(0);

        // Draw Cabinet Base Support 1
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color8));
        glBindVertexArray(VAO8);
        glDrawArrays(GL_TRIANGLES, 0, vertices8.size()/3);
        glBindVertexArray(0);

        // Draw Cabinet Base Support 2
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color9));
        glBindVertexArray(VAO9);
        glDrawArrays(GL_TRIANGLES, 0, vertices9.size()/3);
        glBindVertexArray(0);

        // Draw Cabinet Base Support 2
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color10));
        glBindVertexArray(VAO10);
        glDrawArrays(GL_TRIANGLES, 0, vertices10.size()/3);
        glBindVertexArray(0);

        // Draw Shelf
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color11));
        glBindVertexArray(VAO11);
        glDrawArrays(GL_TRIANGLES, 0, vertices11.size()/3);
        glBindVertexArray(0);

        // Draw TV
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color12));
        glBindVertexArray(VAO12);
        glDrawArrays(GL_TRIANGLES, 0, vertices12.size()/3);
        glBindVertexArray(0);

        // Draw TV boarder
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color13));
        glBindVertexArray(VAO13);
        glDrawArrays(GL_TRIANGLES, 0, vertices13.size()/3);
        glBindVertexArray(0);

        // Draw Switch case
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color14));
        glBindVertexArray(VAO14);
        glDrawArrays(GL_TRIANGLES, 0, vertices14.size()/3);
        glBindVertexArray(0);

        // Draw Switch case zipper
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color15));
        glBindVertexArray(VAO15);
        glDrawArrays(GL_TRIANGLES, 0, vertices15.size()/3);
        glBindVertexArray(0);

        // Draw waterbottle
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color16));
        glBindVertexArray(VAO16);
        glDrawArrays(GL_TRIANGLES, 0, vertices16.size()/3);
        glBindVertexArray(0);

        // Draw waterbottle neck
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color17));
        glBindVertexArray(VAO17);
        glDrawArrays(GL_TRIANGLES, 0, vertices17.size()/3);
        glBindVertexArray(0);

        // Draw waterbottle silver ring
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color18));
        glBindVertexArray(VAO18);
        glDrawArrays(GL_TRIANGLES, 0, vertices18.size()/3);
        glBindVertexArray(0);

        // Draw waterbottle Lid
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color19));
        glBindVertexArray(VAO19);
        glDrawArrays(GL_TRIANGLES, 0, vertices19.size()/3);
        glBindVertexArray(0);

        // Draw Cabinet Base Support 3
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color20));
        glBindVertexArray(VAO20);
        glDrawArrays(GL_TRIANGLES, 0, vertices20.size()/3);
        glBindVertexArray(0);

        // Draw Cabinet Base Support 4
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color21));
        glBindVertexArray(VAO21);
        glDrawArrays(GL_TRIANGLES, 0, vertices21.size()/3);
        glBindVertexArray(0);

        // Draw Switch Dock
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color22));
        glBindVertexArray(VAO22);
        glDrawArrays(GL_TRIANGLES, 0, vertices22.size()/3);
        glBindVertexArray(0);

        // Draw Switch
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color23));
        glBindVertexArray(VAO23);
        glDrawArrays(GL_TRIANGLES, 0, vertices23.size()/3);
        glBindVertexArray(0);

        // Draw Joy Con L
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color24));
        glBindVertexArray(VAO24);
        glDrawArrays(GL_TRIANGLES, 0, vertices24.size()/3);
        glBindVertexArray(0);

        // Draw Joy Con R
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color25));
        glBindVertexArray(VAO25);
        glDrawArrays(GL_TRIANGLES, 0, vertices25.size()/3);
        glBindVertexArray(0);

        // Draw TV Stand 1
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color26));
        glBindVertexArray(VAO26);
        glDrawArrays(GL_TRIANGLES, 0, vertices26.size()/3);
        glBindVertexArray(0);

        // Draw TV Stand 2
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color27));
        glBindVertexArray(VAO27);
        glDrawArrays(GL_TRIANGLES, 0, vertices27.size()/3);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1,&VAO1); glDeleteBuffers(1,&VBO1);
    glDeleteVertexArrays(1,&VAO2); glDeleteBuffers(1,&VBO2);
    glDeleteVertexArrays(1,&VAO3); glDeleteBuffers(1,&VBO3);
    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow* window,int key,int scancode,int action,int mode)
{
    if(key==GLFW_KEY_ESCAPE && action==GLFW_PRESS)
        glfwSetWindowShouldClose(window,GL_TRUE);

    // Control + C resets camera
    if(key==GLFW_KEY_C && (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                           glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS))
    {
        cameraPos = initialCameraPos;
        cameraFront = initialCameraFront;
        std::cout << "Camera reset to initial position.\n";
    }
}

void scroll_callback(GLFWwindow* window,double xoffset,double yoffset)
{
    cameraPos.y += static_cast<float>(yoffset)*0.2f;
}
