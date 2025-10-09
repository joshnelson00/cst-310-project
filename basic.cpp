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
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Shader.h"

// Joycon shader sources
static const char* joyconVertexShaderSrc = R"glsl(
#version 330 core
layout(location = 0) in vec2 aPos; // clip-space quad positions (-1..1)

out vec2 v_uv; // 0..1 normalized coordinates across the screen

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    // map clip-space (-1..1) to uv (0..1)
    v_uv = (aPos * 0.5) + 0.5;
}
)glsl";

static const char* joyconFragmentShaderSrc = R"glsl(
#version 330 core
in vec2 v_uv;
out vec4 FragColor;

uniform vec2 u_center;
uniform vec2 u_size;
uniform float u_radius;
uniform vec3 u_color;
uniform float u_edgeSoftness;

// Signed distance to rounded rectangle
float sdRoundRect(vec2 p, vec2 halfSize, float r) {
    vec2 d = abs(p) - halfSize + vec2(r);
    vec2 d_clamped = max(d, vec2(0.0));
    float outside = length(d_clamped) - r;
    float inside = min(max(d.x, d.y), 0.0) - r;
    return outside + inside;
}

void main() {
    vec2 p = v_uv - u_center;
    vec2 halfSize = u_size * 0.5;

    float dist = sdRoundRect(p, halfSize, u_radius);
    float alpha = 1.0 - smoothstep(0.0, u_edgeSoftness, dist);

    // Cut the joycon vertically in half
    if (p.x > 0.0)
        discard;

    if (alpha <= 0.001) discard;

    FragColor = vec4(u_color, alpha);
}
)glsl";


const GLuint WIDTH = 702, HEIGHT = 1062;

glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

// Camera rotation
float yaw   = -90.0f; // start facing -Z
float pitch = 0.0f;


glm::vec3 initialCameraPos   = cameraPos;
glm::vec3 initialCameraFront = cameraFront;
float initialYaw   = -90.0f; 
float initialPitch = 0.0f;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
GLuint loadTexture(const char* path);

float screenToNDC_X(float x) { return (2.0f * x / WIDTH) - 1.0f; }
float screenToNDC_Y(float y) { return 1.0f - (2.0f * y / HEIGHT); }

std::vector<GLfloat> createPrismVertices(const std::vector<std::pair<int,int>>& corners, float zFront, float zBack, bool withTexCoords = false)
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

    if (withTexCoords) {
        // With texture coordinates (5 components per vertex: x, y, z, s, t)
        return {
            // Front face
            x0, y0, zFront,  0.0f, 0.0f,
            x1, y1, zFront,  1.0f, 0.0f,
            x2, y2, zFront,  1.0f, 1.0f,
            x2, y2, zFront,  1.0f, 1.0f,
            x3, y3, zFront,  0.0f, 1.0f,
            x0, y0, zFront,  0.0f, 0.0f,

            // Back face
            x1, y1, zBack,   1.0f, 0.0f,
            x0, y0, zBack,   0.0f, 0.0f,
            x3, y3, zBack,   0.0f, 1.0f,
            x3, y3, zBack,   0.0f, 1.0f,
            x2, y2, zBack,   1.0f, 1.0f,
            x1, y1, zBack,   1.0f, 0.0f,

            // Left face
            x0, y0, zBack,   1.0f, 0.0f,
            x0, y0, zFront,  1.0f, 1.0f,
            x3, y3, zFront,  0.0f, 1.0f,
            x3, y3, zFront,  0.0f, 1.0f,
            x3, y3, zBack,   0.0f, 0.0f,
            x0, y0, zBack,   1.0f, 0.0f,

            // Right face
            x1, y1, zFront,  1.0f, 1.0f,
            x1, y1, zBack,   1.0f, 0.0f,
            x2, y2, zBack,   0.0f, 0.0f,
            x2, y2, zBack,   0.0f, 0.0f,
            x2, y2, zFront,  0.0f, 1.0f,
            x1, y1, zFront,  1.0f, 1.0f,

            // Top face (corrected)
            x0, y0, zFront,  0.0f, 0.0f,  // bottom-left
            x1, y1, zFront,  1.0f, 0.0f,  // bottom-right
            x2, y2, zFront,  1.0f, 1.0f,  // top-right
            x0, y0, zFront,  0.0f, 0.0f,  // bottom-left
            x2, y2, zFront,  1.0f, 1.0f,  // top-right
            x3, y3, zFront,  0.0f, 1.0f,  // top-left

            // Bottom face
            x0, y0, zBack,   0.0f, 0.0f,
            x1, y1, zBack,   1.0f, 0.0f,
            x3, y3, zBack,   0.0f, 1.0f,
            x3, y3, zBack,   0.0f, 1.0f,
            x1, y1, zBack,   1.0f, 0.0f,
            x2, y2, zBack,   1.0f, 1.0f
        };
    } else {
        // Without texture coordinates (3 components per vertex: x, y, z)
        return {
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
    }
}

// Helper to convert 0-255 RGB to glm::vec4 (RGBA)
glm::vec4 rgb255(int r, int g, int b, int a = 255) {
    return glm::vec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}


std::vector<GLfloat> createCircleVertices(float centerX, float centerY, float z, float radius, int segments = 32)
{
    std::vector<GLfloat> vertices;
    for(int i = 0; i < segments; ++i)
    {
        float theta1 = 2.0f * 3.1415926f * float(i) / float(segments);
        float theta2 = 2.0f * 3.1415926f * float(i+1) / float(segments);

        float x1 = centerX + radius * cos(theta1);
        float y1 = centerY + radius * sin(theta1);
        float x2 = centerX + radius * cos(theta2);
        float y2 = centerY + radius * sin(theta2);

        vertices.push_back(screenToNDC_X(centerX)); // Triangle center
        vertices.push_back(screenToNDC_Y(centerY));
        vertices.push_back(z);

        vertices.push_back(screenToNDC_X(x1)); // Edge point 1
        vertices.push_back(screenToNDC_Y(y1));
        vertices.push_back(z);

        vertices.push_back(screenToNDC_X(x2)); // Edge point 2
        vertices.push_back(screenToNDC_Y(y2));
        vertices.push_back(z);
    }
    return vertices;
}

GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint logLen;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen, '\0');
        glGetShaderInfoLog(shader, logLen, nullptr, &log[0]);
        std::cerr << "Shader compile error: " << log << "\n";
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint createProgram(const char* vsrc, const char* fsrc) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vsrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsrc);
    if (!vs || !fs) return 0;
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint logLen;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen, '\0');
        glGetProgramInfoLog(prog, logLen, nullptr, &log[0]);
        std::cerr << "Program link error: " << log << "\n";
        glDeleteProgram(prog);
        return 0;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

int main()
{
    // Water bottle mesh variables (declared at function scope)
    GLuint waterBottleVAO = 0, waterBottleVBO = 0, waterBottleEBO = 0;
    std::vector<GLfloat> waterBottleVertices;
    std::vector<GLuint> waterBottleIndices;
    glm::vec3 bottlePosition(-0.7f, 0.05f, -0.9f);  // x, y, z position

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

        // ===== Initialize Joycon 3D =====
    // Create a half-pill shaped joycon
    float joyconLength = 0.115f;      // Length of the joycon
    float joyconRadius = 0.05f;      // Radius of the half-cylinder
    int segments = 32;              // Number of segments for the half-cylinder
    
    // Create vertices for the half-pill joycon
    std::vector<GLfloat> joyconVertices;
    
    // Function to add a vertex with normal for lighting
    auto addVertex = [&](float x, float y, float z, float nx, float ny, float nz) {
        joyconVertices.push_back(x);
        joyconVertices.push_back(y);
        joyconVertices.push_back(z);
        joyconVertices.push_back(nx);
        joyconVertices.push_back(ny);
        joyconVertices.push_back(nz);
    };
    
    // Create the half-cylinder part
    float halfLength = joyconLength * 0.5f;
    
    // Create the curved part (half-cylinder)
    for (int i = 0; i <= segments; ++i) {
        float theta = i * (M_PI / segments);  // Only half the circle (0 to 180 degrees)
        float x = cosf(theta) * joyconRadius;
        float z = sinf(theta) * joyconRadius;
        
        // Add vertices for the front face
        addVertex(x, halfLength, z, x, 0, z);
        addVertex(x, -halfLength, z, x, 0, z);
        
        // Add flat face (the cut part)
        if (i < segments) {
            // Add two triangles to form a quad for the flat face
            addVertex(0, -halfLength, 0, 0, 0, -1);
            addVertex(0, halfLength, 0, 0, 0, -1);
            addVertex(x, halfLength, 0, 0, 0, -1);
            
            addVertex(0, -halfLength, 0, 0, 0, -1);
            addVertex(x, halfLength, 0, 0, 0, -1);
            addVertex(x, -halfLength, 0, 0, 0, -1);
        }
    }
    
    // Add the semi-circular caps
    for (int cap = 0; cap < 2; ++cap) {
        float y = (cap == 0) ? halfLength : -halfLength;
        
        // Center point
        addVertex(0, y, 0, 0, (cap == 0) ? 1.0f : -1.0f, 0);
        
        // Create semi-circle of vertices
        for (int i = 0; i <= segments; ++i) {
            float theta = i * (M_PI / segments);  // 0 to 180 degrees
            float x = cosf(theta) * joyconRadius;
            float z = sinf(theta) * joyconRadius;
            
            addVertex(x, y, z, 0, (cap == 0) ? 1.0f : -1.0f, 0);
            
            // Add triangle fan for the cap
            if (i > 0) {
                addVertex(0, y, 0, 0, (cap == 0) ? 1.0f : -1.0f, 0);
                addVertex(x, y, z, 0, (cap == 0) ? 1.0f : -1.0f, 0);
            }
        }
    }
    
    // Create VAO/VBO for the joycon
    GLuint joyconVAO, joyconVBO;
    glGenVertexArrays(1, &joyconVAO);
    glGenBuffers(1, &joyconVBO);
    glBindVertexArray(joyconVAO);
    glBindBuffer(GL_ARRAY_BUFFER, joyconVBO);
    glBufferData(GL_ARRAY_BUFFER, joyconVertices.size() * sizeof(GLfloat), joyconVertices.data(), GL_STATIC_DRAW);
    
    // Position attribute (3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute (3 floats)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    
    // Joycon positions and colors
    glm::vec3 joyconPosition(0.874f, -0.025f, -0.6f);    // Blue joycon position (right side)
    glm::vec3 joyconPositionR(0.6f, -0.025f, -0.6f);  // Red joycon position (left side)
    glm::vec4 joyconColor = rgb255(10, 185, 230);      // Blue color
    glm::vec4 joyconColorR = rgb255(230, 30, 30);      // Red color

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

    // Cabinet Base ---
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
    std::vector<GLfloat> vertices8 = createPrismVertices(corners8,-0.47f,-1.0f);

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
    std::vector<GLfloat> vertices9 = createPrismVertices(corners9,-0.47f,-1.0f);

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
    std::vector<GLfloat> vertices10 = createPrismVertices(corners10,-0.9f,-1.1f);

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
    std::vector<GLfloat> vertices15 = createPrismVertices(corners15, -0.59, -0.81f);

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

    // tv leg 3 ---
    std::vector<std::pair<int,int>> corners30 = {
    {553, 544}, {565, 544}, {554, 582}, {549, 582},
    };

    glm::vec4 color30 = rgb255(33, 33, 33); // black
    std::vector<GLfloat> vertices30 = createPrismVertices(corners30,-0.9f,-1.0f);

    GLuint VAO30,VBO30;
    glGenVertexArrays(1,&VAO30);
    glGenBuffers(1,&VBO30);
    glBindVertexArray(VAO30);
    glBindBuffer(GL_ARRAY_BUFFER,VBO30);
    glBufferData(GL_ARRAY_BUFFER,vertices30.size()*sizeof(GLfloat),vertices30.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(GLfloat),(GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // tv leg 4 ---
    std::vector<std::pair<int,int>> corners31 = {
    {553, 544}, {565, 544}, {575, 582}, {569, 582}
    };

    glm::vec4 color31 = rgb255(33, 33, 33); // black
    std::vector<GLfloat> vertices31 = createPrismVertices(corners31,-0.9f,-1.0f);

    GLuint VAO31,VBO31;
    glGenVertexArrays(1,&VAO31);
    glGenBuffers(1,&VBO31);
    glBindVertexArray(VAO31);
    glBindBuffer(GL_ARRAY_BUFFER,VBO31);
    glBufferData(GL_ARRAY_BUFFER,vertices31.size()*sizeof(GLfloat),vertices31.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(GLfloat),(GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Kleenex Box ---
    std::vector<std::pair<int,int>> corners28 = {
        {442, 788}, {486, 788}, {486, 880}, {442, 880}
    };

    glm::vec4 color28 = rgb255(240, 234, 235); // white
    std::vector<GLfloat> vertices28 = createPrismVertices(corners28, -0.6f, -0.8f, true);  // true means include texture coords

    // Load Kleenex box texture
    GLuint kleenexTexture = loadTexture("kleenex-box.jpg");
    if (kleenexTexture == 0) {
        std::cerr << "Failed to load Kleenex box texture" << std::endl;
        return -1;
    }

    GLuint VAO28, VBO28;
    glGenVertexArrays(1, &VAO28);
    glGenBuffers(1, &VBO28);
    glBindVertexArray(VAO28);
    glBindBuffer(GL_ARRAY_BUFFER, VBO28);
    glBufferData(GL_ARRAY_BUFFER, vertices28.size() * sizeof(GLfloat), vertices28.data(), GL_STATIC_DRAW);

    // Position attribute (3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute (2 floats)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // Carpet ---
    std::vector<std::pair<int,int>> corners29 = {
    {0, 906}, {702, 906}, {702, 908}, {0, 908}
    };

    glm::vec4 color29 = rgb255(163, 150, 133); // carpet color
    std::vector<GLfloat> vertices29 = createPrismVertices(corners29,-0.5f,1.0f);

    GLuint VAO29,VBO29;
    glGenVertexArrays(1,&VAO29);
    glGenBuffers(1,&VBO29);
    glBindVertexArray(VAO29);
    glBindBuffer(GL_ARRAY_BUFFER,VBO29);
    glBufferData(GL_ARRAY_BUFFER,vertices29.size()*sizeof(GLfloat),vertices29.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(GLfloat),(GLvoid*)0);  
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Barn Doors --
    // --- Left Barn Door ---
    std::vector<std::pair<int,int>> cornersLeftDoor = {
        {0, 594}, {201, 594}, {201, 880}, {0, 880}  // extend to very left edge
    };

    // --- Right Barn Door (covers support 2 and 4) ---
    std::vector<std::pair<int,int>> cornersRightDoor = {
        {501, 594}, {702, 594}, {702, 880}, {501, 880}  // extend to very right edge
    };

    // Color for doors
    glm::vec4 colorDoor = rgb255(24, 23, 21);

    // Left Door vertices
    std::vector<GLfloat> verticesLeftDoor = createPrismVertices(cornersLeftDoor, -0.47f, -0.5f);
    GLuint VAOLeftDoor, VBOLeftDoor;
    glGenVertexArrays(1, &VAOLeftDoor);
    glGenBuffers(1, &VBOLeftDoor);
    glBindVertexArray(VAOLeftDoor);
    glBindBuffer(GL_ARRAY_BUFFER, VBOLeftDoor);
    glBufferData(GL_ARRAY_BUFFER, verticesLeftDoor.size() * sizeof(GLfloat), verticesLeftDoor.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Right Door vertices
    std::vector<GLfloat> verticesRightDoor = createPrismVertices(cornersRightDoor, -0.47f, -0.5f);
    GLuint VAORightDoor, VBORightDoor;
    glGenVertexArrays(1, &VAORightDoor);
    glGenBuffers(1, &VBORightDoor);
    glBindVertexArray(VAORightDoor);
    glBindBuffer(GL_ARRAY_BUFFER, VBORightDoor);
    glBufferData(GL_ARRAY_BUFFER, verticesRightDoor.size() * sizeof(GLfloat), verticesRightDoor.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // --- X on Left Door ---
    std::vector<GLfloat> verticesLeftX = {
        // diagonal from top-left to bottom-right
        screenToNDC_X(14),  screenToNDC_Y(594), -0.46f,
        screenToNDC_X(201), screenToNDC_Y(880), -0.46f,

        // diagonal from bottom-left to top-right
        screenToNDC_X(14),  screenToNDC_Y(880), -0.46f,
        screenToNDC_X(201), screenToNDC_Y(594), -0.46f
    };

    GLuint VAOLX, VBOLX;
    glGenVertexArrays(1, &VAOLX);
    glGenBuffers(1, &VBOLX);
    glBindVertexArray(VAOLX);
    glBindBuffer(GL_ARRAY_BUFFER, VBOLX);
    glBufferData(GL_ARRAY_BUFFER, verticesLeftX.size() * sizeof(GLfloat), verticesLeftX.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // --- X on Right Door ---
    std::vector<GLfloat> verticesRightX = {
        screenToNDC_X(501), screenToNDC_Y(594), -0.46f,
        screenToNDC_X(688), screenToNDC_Y(880), -0.46f,

        screenToNDC_X(501), screenToNDC_Y(880), -0.46f,
        screenToNDC_X(688), screenToNDC_Y(594), -0.46f
    };

    GLuint VAORX, VBORX;
    glGenVertexArrays(1, &VAORX);
    glGenBuffers(1, &VBORX);
    glBindVertexArray(VAORX);
    glBindBuffer(GL_ARRAY_BUFFER, VBORX);
    glBufferData(GL_ARRAY_BUFFER, verticesRightX.size() * sizeof(GLfloat), verticesRightX.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // --- Barn Door Knobs ---
    // Left door knob (right side, near center)
    float leftKnobX = 201.0f - 8.0f; // slightly inset from center edge
    float leftKnobY = (594 + 880) / 2.0f; // vertical center
    float knobRadius = 5.0f;

    std::vector<GLfloat> verticesLeftKnob = createCircleVertices(leftKnobX, leftKnobY, -0.46f, knobRadius);
    GLuint VAOLeftKnob, VBOLefKnob;
    glGenVertexArrays(1,&VAOLeftKnob);
    glGenBuffers(1,&VBOLefKnob);
    glBindVertexArray(VAOLeftKnob);
    glBindBuffer(GL_ARRAY_BUFFER, VBOLefKnob);
    glBufferData(GL_ARRAY_BUFFER, verticesLeftKnob.size()*sizeof(GLfloat), verticesLeftKnob.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(GLfloat),(GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Right door knob (left side, near center)
    float rightKnobX = 501.0f + 8.0f; // slightly inset from center edge
    float rightKnobY = (594 + 880) / 2.0f;

    std::vector<GLfloat> verticesRightKnob = createCircleVertices(rightKnobX, rightKnobY, -0.46f, knobRadius);
    GLuint VAORightKnob, VBORightKnob;
    glGenVertexArrays(1,&VAORightKnob);
    glGenBuffers(1,&VBORightKnob);
    glBindVertexArray(VAORightKnob);
    glBindBuffer(GL_ARRAY_BUFFER, VBORightKnob);
    glBufferData(GL_ARRAY_BUFFER, verticesRightKnob.size()*sizeof(GLfloat), verticesRightKnob.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(GLfloat),(GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // --- Circle Hole ---
    const int holeSegments = 32;
    float holeRadius = 10.0f; // pixels
    float holeX = 356.0f;     // pixel X
    float holeY = 644.0f;     // pixel Y
    float holeZ = -0.8f;     // adjustable Z-depth

    float aspect = (float)HEIGHT / (float)WIDTH; // H / W
    float cx = screenToNDC_X(holeX);
    float cy = screenToNDC_Y(holeY);

    // Generate vertices
    std::vector<GLfloat> verticesHole;
    for(int i = 0; i <= holeSegments; ++i) {
        float theta = 2.0f * 3.1415926f * float(i) / float(holeSegments);
        float x = cx + (screenToNDC_X(holeRadius) - screenToNDC_X(0)) * cos(theta);
        float y = cy + (screenToNDC_Y(holeRadius) - screenToNDC_Y(0)) * sin(theta); // NO / aspect

        verticesHole.push_back(x);
        verticesHole.push_back(y);
        verticesHole.push_back(holeZ); // use adjustable Z
    }

    // Create VAO/VBO
    GLuint VAOHole, VBOHole;
    glGenVertexArrays(1, &VAOHole);
    glGenBuffers(1, &VBOHole);
    glBindVertexArray(VAOHole);
    glBindBuffer(GL_ARRAY_BUFFER, VBOHole);
    glBufferData(GL_ARRAY_BUFFER, verticesHole.size() * sizeof(GLfloat), verticesHole.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);



    // --- Waterbottle Mesh ---
    // Make the bottle much smaller (reduced from original)
    float scale = 0.4f;
    float H = 18.0f * scale;            // total height (scaled down to 40% of original)
    float R_base = 3.0f * scale;       // wide cylinder base (scaled down)
    float R_neck = 2.6f * scale;        // narrow neck (scaled down)
    float H_shoulder_start = 15.0f * scale; // height where taper begins (scaled)
    float H_shoulder_end = 16.0f * scale;;   // height where taper ends (scaled)
    int N_theta = 60;                    // number of angular segments
    int N_z = 50;                        // number of vertical segments

    // Clear any existing data
    waterBottleVertices.clear();
    waterBottleIndices.clear();

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

        // Calculate color based on height - solid colors only
        glm::vec3 color;
        if (z < H_shoulder_start) {
            color = glm::vec3(0.3137f, 0.2039f, 0.4902f); // Purple
        } else if (z < H_shoulder_end + 0.05f) {  // Silver band
            color = glm::vec3(0.5294f, 0.4980f, 0.5608f); // Silver
        } else {
            color = glm::vec3(0.1176f, 0.1059f, 0.1098f); // Black lid
        }

        for (int j = 0; j <= N_theta; ++j) {
            float theta = j * 2.0f * M_PI / N_theta;
            float x = R * cos(theta);
            float y = R * sin(theta);

            // Position (centered at origin, will be transformed by model matrix)
            waterBottleVertices.push_back(x * 0.04f);     // x position (scaled down)
            waterBottleVertices.push_back((z - H/2.0f) * 0.04f); // y position (centered vertically)
            waterBottleVertices.push_back(y * 0.04f);     // z position (centered)
            
            // Color
            waterBottleVertices.push_back(color.r);
            waterBottleVertices.push_back(color.g);
            waterBottleVertices.push_back(color.b);
        }
    }

    // Generate indices for triangle strips
    for (int i = 0; i < N_z; ++i) {
        for (int j = 0; j <= N_theta; ++j) {
            waterBottleIndices.push_back(i * (N_theta + 1) + j);
            waterBottleIndices.push_back((i + 1) * (N_theta + 1) + j);
        }
        // Add degenerate triangle for strip connection
        if (i < N_z - 1) {
            waterBottleIndices.push_back((i + 1) * (N_theta + 1) + N_theta);
            waterBottleIndices.push_back((i + 1) * (N_theta + 1));
        }
    }

    // Create and bind VAO/VBO for water bottle if not already created
    if (waterBottleVAO == 0) {
        glGenVertexArrays(1, &waterBottleVAO);
        glGenBuffers(1, &waterBottleVBO);
        glGenBuffers(1, &waterBottleEBO);
    }

    glBindVertexArray(waterBottleVAO);

    // Bind and set vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, waterBottleVBO);
    glBufferData(GL_ARRAY_BUFFER, waterBottleVertices.size() * sizeof(GLfloat), waterBottleVertices.data(), GL_STATIC_DRAW);

    // Bind and set element buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterBottleEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, waterBottleIndices.size() * sizeof(GLuint), waterBottleIndices.data(), GL_STATIC_DRAW);

    // Position attribute (3 floats) - location 0 in shader
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0);
    
    // Color attribute (3 floats) - location 2 in shader (matches shader's layout)
    glEnableVertexAttribArray(2);  // Changed from 1 to 2 to match shader
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

    glBindVertexArray(0);

    // --- Render loop ---
    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Camera movement
    float cameraSpeed = 0.01f;                // forward/backward speed
    float strafeSpeed = cameraSpeed * 0.5f;   // left/right speed is half
    float angleSpeed  = 0.25f;                 // degrees per key press/frame

    glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));

    // Position controls (WASD)
    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += cameraSpeed * cameraUp;
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= cameraSpeed * cameraUp;
    if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) cameraPos += cameraSpeed * cameraFront;
    if(glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) cameraPos -= cameraSpeed * cameraFront;
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= strafeSpeed * right;
    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += strafeSpeed * right;

    // Clamp vertical movement
    cameraPos.y = glm::clamp(cameraPos.y, -5.0f, 5.0f);

    // Rotation controls (Arrow Keys)
    if(glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  yaw   -= angleSpeed;
    if(glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) yaw   += angleSpeed;
    if(glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    pitch += angleSpeed;
    if(glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  pitch -= angleSpeed;

    // Constrain pitch to avoid flipping
    if(pitch > 89.0f) pitch = 89.0f;
    if(pitch < -89.0f) pitch = -89.0f;

    // Recalculate cameraFront from yaw/pitch
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);


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

        // Draw waterbottle mesh with position transformation
        shader.Use();
        glm::mat4 bottleModel = glm::mat4(1.0f);
        bottleModel = glm::translate(bottleModel, bottlePosition);
        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "model"), 1, GL_FALSE, glm::value_ptr(bottleModel));
        
        // Enable vertex attributes for the water bottle
        glBindVertexArray(waterBottleVAO);
        glEnableVertexAttribArray(0); // position
        glEnableVertexAttribArray(2); // color
        
        // Draw the water bottle
        glDrawElements(GL_TRIANGLE_STRIP, (GLsizei)waterBottleIndices.size(), GL_UNSIGNED_INT, 0);
        
        // Create and draw the disk cap
        {
            static GLuint diskVAO = 0, diskVBO = 0, diskEBO = 0;
            static int diskNumIndices = 0;
            
            if (diskVAO == 0) {
                const float radius = 2.2f * 0.02f;  // Scaled neck radius (2.6 * 0.04)
                const float height = 0.2f;          // Thickness of the disk
                const int segments = 32;            // Number of segments for the disk
                
                std::vector<GLfloat> vertices;
                std::vector<GLuint> indices;
                
                // Create top and bottom circles
                for (int cap = 0; cap <= 1; ++cap) {
                    float y = (cap == 0) ? -height/2.0f : height/2.0f;
                    
                    // Center point for this cap
                    vertices.push_back(0.0f);  // x
                    vertices.push_back(y);     // y
                    vertices.push_back(0.0f);  // z
                    
                    // Circle points for this cap
                    for (int i = 0; i <= segments; ++i) {
                        float theta = 2.0f * 3.1415926f * float(i) / float(segments);
                        float x = radius * cosf(theta);
                        float z = radius * sinf(theta);
                        vertices.push_back(x);
                        vertices.push_back(y);
                        vertices.push_back(z);
                    }
                }
                
                // Generate indices for the disk
                // Top cap (first circle)
                int centerTop = 0;
                int centerBottom = segments + 2;  // +2 for center and last vertex of first circle
                
                // Top and bottom caps
                for (int i = 0; i < segments; ++i) {
                    // Top cap
                    indices.push_back(centerTop);
                    indices.push_back(centerTop + i + 1);
                    indices.push_back(centerTop + ((i + 1) % segments) + 1);
                    
                    // Bottom cap (reverse winding)
                    indices.push_back(centerBottom);
                    indices.push_back(centerBottom + ((i + 1) % segments) + 1);
                    indices.push_back(centerBottom + i + 1);
                }
                
                // Sides
                for (int i = 0; i < segments; ++i) {
                    int next = (i + 1) % segments;
                    
                    // Triangle 1
                    indices.push_back(centerTop + i + 1);
                    indices.push_back(centerTop + next + 1);
                    indices.push_back(centerBottom + i + 1);
                    
                    // Triangle 2
                    indices.push_back(centerTop + next + 1);
                    indices.push_back(centerBottom + next + 1);
                    indices.push_back(centerBottom + i + 1);
                }
                
                // Create and bind VAO/VBO/EBO
                glGenVertexArrays(1, &diskVAO);
                glGenBuffers(1, &diskVBO);
                glGenBuffers(1, &diskEBO);
                
                glBindVertexArray(diskVAO);
                
                // Vertex buffer
                glBindBuffer(GL_ARRAY_BUFFER, diskVBO);
                glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
                
                // Element buffer
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, diskEBO);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
                
                // Position attribute
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
                glEnableVertexAttribArray(0);
                
                diskNumIndices = indices.size();
                glBindVertexArray(0);
            }
            
            // Draw the disk cap
            glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(rgb255(0, 0, 0))); // Black color
            
            // Position the disk at the top of the bottle
            glm::mat4 diskModel = glm::mat4(1.0f);
            // Position it at the top of the bottle (18.0 * 0.04 is the total height, 0.02 is half the disk height)
            diskModel = glm::translate(diskModel, bottlePosition + glm::vec3(0.0f, -2.0f * 0.04f + 0.1f, 0.0f));
            glUniformMatrix4fv(glGetUniformLocation(shader.Program, "model"), 1, GL_FALSE, glm::value_ptr(diskModel));
            
            glBindVertexArray(diskVAO);
            glDrawElements(GL_TRIANGLES, diskNumIndices, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        
        // Clean up
        glBindVertexArray(0);
        
        // Reset model matrix for other objects
        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "model"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

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

        // Draw TV Stand 3
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color30));
        glBindVertexArray(VAO30);
        glDrawArrays(GL_TRIANGLES, 0, vertices30.size()/3);
        glBindVertexArray(0);

        // Draw TV Stand 4
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color31));
        glBindVertexArray(VAO31);
        glDrawArrays(GL_TRIANGLES, 0, vertices31.size()/3);
        glBindVertexArray(0);

        // Draw Kleenex Box with texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, kleenexTexture);
        glUniform1i(glGetUniformLocation(shader.Program, "ourTexture"), 0);
        glUniform1i(glGetUniformLocation(shader.Program, "useTexture"), GL_TRUE);
        glBindVertexArray(VAO28);
        glDrawArrays(GL_TRIANGLES, 0, vertices28.size()/5); // Divide by 5 for x,y,z,s,t
        glBindVertexArray(0);
        glUniform1i(glGetUniformLocation(shader.Program, "useTexture"), GL_FALSE);

        // Draw Carpet
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(color29));
        glBindVertexArray(VAO29);
        glDrawArrays(GL_TRIANGLES, 0, vertices29.size()/3);
        glBindVertexArray(0);

        // Draw Left Barn Door
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(colorDoor));
        glBindVertexArray(VAOLeftDoor);
        glDrawArrays(GL_TRIANGLES, 0, verticesLeftDoor.size()/3);
        glBindVertexArray(0);

        // Draw Right Barn Door
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(colorDoor));
        glBindVertexArray(VAORightDoor);
        glDrawArrays(GL_TRIANGLES, 0, verticesRightDoor.size()/3);
        glBindVertexArray(0);

        // Draw X on Left Door
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(rgb255(40,39,36))); // grey
        glBindVertexArray(VAOLX);
        glDrawArrays(GL_LINES, 0, verticesLeftX.size()/3);
        glBindVertexArray(0);

        // Draw X on Right Door
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(rgb255(40,39,36))); // grey
        glBindVertexArray(VAORX);
        glDrawArrays(GL_LINES, 0, verticesRightX.size()/3);
        glBindVertexArray(0);

        // Draw Left Door Knob
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(rgb255(40,39,36)));
        glBindVertexArray(VAOLeftKnob);
        glDrawArrays(GL_TRIANGLES, 0, verticesLeftKnob.size()/3);
        glBindVertexArray(0);

        // Draw Right Door Knob
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(rgb255(40,39,36)));
        glBindVertexArray(VAORightKnob);
        glDrawArrays(GL_TRIANGLES, 0, verticesRightKnob.size()/3);
        glBindVertexArray(0);

        // Draw the circle hole
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(rgb255(42,39,32)));
        glBindVertexArray(VAOHole);
        glDrawArrays(GL_TRIANGLE_FAN, 0, verticesHole.size()/3);
        glBindVertexArray(0);

        // ===== Draw 3D Joycon =====
        shader.Use();
        
        // Draw the blue joycon (right side)
        glm::mat4 joyconModel = glm::mat4(1.0f);
        joyconModel = glm::translate(joyconModel, joyconPosition);
        joyconModel = glm::rotate(joyconModel, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        
        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "model"), 1, GL_FALSE, glm::value_ptr(joyconModel));
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(joyconColor));
        
        glBindVertexArray(joyconVAO);
        glDrawArrays(GL_TRIANGLES, 0, joyconVertices.size() / 6);
        
        // Draw the red joycon (left side, mirrored)
        glm::mat4 joyconModelR = glm::mat4(1.0f);
        joyconModelR = glm::translate(joyconModelR, joyconPositionR);
        joyconModelR = glm::rotate(joyconModelR, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Negative rotation for mirroring
        
        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "model"), 1, GL_FALSE, glm::value_ptr(joyconModelR));
        glUniform4fv(glGetUniformLocation(shader.Program, "prismColor"), 1, glm::value_ptr(joyconColorR));
        
        glDrawArrays(GL_TRIANGLES, 0, joyconVertices.size() / 6);
        glBindVertexArray(0);
        
        // Reset model matrix for other objects
        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "model"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

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
        yaw = initialYaw;
        pitch = initialPitch;
        std::cout << "Camera reset to initial position.\n";
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Adjust camera position based on scroll
    cameraPos += cameraFront * static_cast<float>(yoffset) * 0.1f;
}

// Load a texture from file
GLuint loadTexture(const char* path) {
    // Generate texture ID and load texture data
    GLuint textureID;
    glGenTextures(1, &textureID);
    
    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
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

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cerr << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
        return 0;
    }

    return textureID;
}
