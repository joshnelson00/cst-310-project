#include <iostream>
#include <vector>
#include <cmath>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Window dimensions
const GLuint WIDTH = 800, HEIGHT = 600;

// Camera
glm::vec3 cameraPos = glm::vec3(0.0f, 12.0f, 30.0f);
glm::vec3 cameraTarget = glm::vec3(0.0f, 10.0f, 0.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// The MAIN function
int main() {
    // Water bottle parameters
    float H = 23.0f;                  // total height
    float R_base = 4.0f;              // wide cylinder base
    float R_neck = 3.5f;              // narrow neck
    float H_shoulder_start = 18.0f;   // height where taper begins
    float H_shoulder_end = 20.0f;     // height where taper ends
    int N_theta = 120;                // number of angular segments
    int N_z = 200;                    // number of vertical segments

    // Generate vertices
    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;

    // Generate vertices
    for (int i = 0; i <= N_z; ++i) {
        float z = i * H / N_z;

        // Compute radius using the smooth shoulder function
        float R;
        if (z < H_shoulder_start) {
            R = R_base;
        } else if (z <= H_shoulder_end) {
            R = R_neck + 0.5f * (R_base - R_neck) *
                (1.0f + cos(M_PI * (z - H_shoulder_start) / (H_shoulder_end - H_shoulder_start)));
        } else {
            R = R_neck;
        }

        for (int j = 0; j <= N_theta; ++j) {
            float theta = j * 2.0f * M_PI / N_theta;
            float x = R * cos(theta);
            float y = R * sin(theta);

            // Store vertex position
            vertices.push_back(x);
            vertices.push_back(z);  // Using z as up axis
            vertices.push_back(y);
        }
    }

    // Generate indices for triangle strips
    for (int i = 0; i < N_z; ++i) {
        for (int j = 0; j <= N_theta; ++j) {
            indices.push_back(i * (N_theta + 1) + j);
            indices.push_back((i + 1) * (N_theta + 1) + j);
        }
        // Add degenerate triangle for strip connection
        if (i < N_z - 1) {
            indices.push_back((i + 1) * (N_theta + 1) + N_theta);
            indices.push_back((i + 1) * (N_theta + 1));
        }
    }

    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // Create a GLFWwindow object
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Water Bottle Mesh", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Define the viewport dimensions
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Build and compile our shader program
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 position;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        out vec3 FragPos;
        
        void main()
        {
            gl_Position = projection * view * model * vec4(position, 1.0f);
            FragPos = vec3(model * vec4(position, 1.0f));
        }
    )";

    // In the fragment shader, replace the existing code with:
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec3 FragPos;  // Add this varying to get fragment position
        out vec4 FragColor;
        
        void main()
        {
            // If the y-coordinate is below the shoulder start, color it purple
            if (FragPos.y < 20.0f) {  // Match H_shoulder_start
                FragColor = vec4(0.5f, 0.0f, 0.8f, 1.0f);  // Purple color
            }
            else if (FragPos.y < 20.5f) {
                FragColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);  // light grey
            }
            else {
                FragColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);  // black
            }
        }
    )";

    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Link shaders
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Set up vertex data and attribute pointers
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0); // Unbind VAO

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Wireframe mode (comment out for solid)
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        // Check and call events
        glfwPollEvents();

        // Clear the color and depth buffer
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use the shader program
        glUseProgram(shaderProgram);

        // Create transformations
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
        glm::mat4 projection = glm::perspective(45.0f, (GLfloat)WIDTH / (GLfloat)HEIGHT, 0.1f, 100.0f);

        // Rotate the model for better viewing
        model = glm::rotate(model, (float)glfwGetTime() * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));

        // Get their uniform locations
        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
        GLint projLoc = glGetUniformLocation(shaderProgram, "projection");

        // Pass them to the shaders
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Draw the water bottle
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLE_STRIP, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Swap the screen buffers
        glfwSwapBuffers(window);
    }

    // Properly de-allocate all resources
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    // Terminate GLFW
    glfwTerminate();
    return 0;
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}