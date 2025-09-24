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
        {400, 590}, {480, 590}, {480, 610}, {400, 610}
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
        {376,610}, {505,610}, {505,632}, {376,632}
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
        {340,712}, {438,712}, {438,729}, {340,729}
    };
    glm::vec4 color4 = rgb255(43, 43, 41); // light gray
    std::vector<GLfloat> vertices4 = createPrismVertices(corners4,-0.5f,-1.0f);

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
        {438,712}, {471,712}, {471,729}, {438,729}
    };
    glm::vec4 color5 = rgb255(10, 10, 10); // near black
    std::vector<GLfloat> vertices5 = createPrismVertices(corners5,-0.5f,-1.0f);

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
        {0, 632}, {702,632}, {702,644}, {0,644}
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
