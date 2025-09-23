// ===========================
// Base OpenGL Project (C++)
// Using: GLFW, GLEW, GLM, custom Shader class
// Renders a rectangular prism matching screen coords
// ===========================

#include <iostream>

// GLEW
#define GLEW_STATIC
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Custom Shader loader
#include "Shader.h"

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// Window dimensions
const GLuint WIDTH = 702, HEIGHT = 1062;

// Camera globals
glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

int main()
{
    // --- Init GLFW ---
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Rectangular Prism", nullptr, nullptr);
    if (!window)
    {
        std::cout << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cout << "Failed to initialize GLEW!" << std::endl;
        return -1;
    }

    glViewport(0, 0, WIDTH, HEIGHT);
    glEnable(GL_DEPTH_TEST);

    Shader shader("basic.vs", "basic.frag");

    // --- Rectangular prism vertices ---
    GLfloat vertices[] = {
        // Front face
        0.114f, -0.141f, -0.5f,
        0.364f, -0.141f, -0.5f,
        0.364f, -0.092f, -0.5f,
        0.364f, -0.092f, -0.5f,
        0.114f, -0.092f, -0.5f,
        0.114f, -0.141f, -0.5f,

        // Back face
        0.114f, -0.141f, -1.0f,
        0.364f, -0.141f, -1.0f,
        0.364f, -0.092f, -1.0f,
        0.364f, -0.092f, -1.0f,
        0.114f, -0.092f, -1.0f,
        0.114f, -0.141f, -1.0f,

        // Left face
        0.114f, -0.092f, -0.5f,
        0.114f, -0.141f, -0.5f,
        0.114f, -0.141f, -1.0f,
        0.114f, -0.141f, -1.0f,
        0.114f, -0.092f, -1.0f,
        0.114f, -0.092f, -0.5f,

        // Right face
        0.364f, -0.092f, -0.5f,
        0.364f, -0.141f, -0.5f,
        0.364f, -0.141f, -1.0f,
        0.364f, -0.141f, -1.0f,
        0.364f, -0.092f, -1.0f,
        0.364f, -0.092f, -0.5f,

        // Bottom face
        0.114f, -0.141f, -0.5f,
        0.364f, -0.141f, -0.5f,
        0.364f, -0.141f, -1.0f,
        0.364f, -0.141f, -1.0f,
        0.114f, -0.141f, -1.0f,
        0.114f, -0.141f, -0.5f,

        // Top face
        0.114f, -0.092f, -0.5f,
        0.364f, -0.092f, -0.5f,
        0.364f, -0.092f, -1.0f,
        0.364f, -0.092f, -1.0f,
        0.114f, -0.092f, -1.0f,
        0.114f, -0.092f, -0.5f
    };

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // --- Render Loop ---
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        const float cameraSpeed = 0.05f;
        const float cameraStrafeSpeed = 0.02f;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            cameraPos += cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            cameraPos -= cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraStrafeSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraStrafeSpeed;

        cameraPos.y = glm::clamp(cameraPos.y, -5.0f, 5.0f);

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        // Clear
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.Use();
        // Transformation for Glasses Case
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f);

        GLint modelLoc = glGetUniformLocation(shader.Program, "model");
        GLint viewLoc  = glGetUniformLocation(shader.Program, "view");
        GLint projLoc  = glGetUniformLocation(shader.Program, "projection");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    return 0;
}

// ===========================
// Callbacks
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    cameraPos.y += static_cast<float>(yoffset) * 0.2f;
}
