// joycon.cpp
// Minimal OpenGL program that draws a red rounded-rectangle "joycon" with no buttons.
// Uses GLFW and GLEW only. No extra libraries or plugins.
// Build (Linux): g++ joycon.cpp -o joycon -lGLEW -lglfw -lGL
// Build (Windows, MinGW): g++ joycon.cpp -o joycon.exe -lglew32 -lglfw3 -lopengl32
// Make sure GLFW and GLEW are installed and on include/library path.

#include <iostream>
#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

static const char* vertexShaderSrc = R"glsl(
#version 330 core
layout(location = 0) in vec2 aPos; // clip-space quad positions (-1..1)

out vec2 v_uv; // 0..1 normalized coordinates across the screen

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    // map clip-space (-1..1) to uv (0..1)
    v_uv = (aPos * 0.5) + 0.5;
}
)glsl";

static const char* fragmentShaderSrc = R"glsl(
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

    // ---- NEW SECTION: cut the joycon vertically in half ----
    // Discard pixels on the right half of the shape (relative to center)
    if (p.x > 0.0)
        discard;
    // --------------------------------------------------------

    if (alpha <= 0.001) discard;

    FragColor = vec4(u_color, alpha);
}
)glsl";



static void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error (" << error << "): " << description << "\n";
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

int main() {
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n";
        return -1;
    }

    // Request OpenGL 3.3 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const int WIN_W = 900;
    const int WIN_H = 600;

    GLFWwindow* window = glfwCreateWindow(WIN_W, WIN_H, "Red Joy-Con (rounded, no buttons)", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Init GLEW after context creation
    glewExperimental = GL_TRUE;
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        std::cerr << "GLEW init failed: " << glewGetErrorString(glewErr) << "\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Full-screen quad (two triangles) covering clip-space (-1..1)
    float quadVerts[] = {
        // x, y
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
        -1.0f,  1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f
    };

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    GLuint program = createProgram(vertexShaderSrc, fragmentShaderSrc);
    if (!program) {
        std::cerr << "Failed to create GL program\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Uniform locations
    GLint loc_center = glGetUniformLocation(program, "u_center");
    GLint loc_size = glGetUniformLocation(program, "u_size");
    GLint loc_radius = glGetUniformLocation(program, "u_radius");
    GLint loc_color = glGetUniformLocation(program, "u_color");
    GLint loc_edge = glGetUniformLocation(program, "u_edgeSoftness");

    // Clear background to a neutral color
    glClearColor(0.12f, 0.12f, 0.12f, 1.0f);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);

        // Parameters for the Joy-Con shape:
        // We'll place the joycon at the left side of the window, vertical oriented like a real Joy-Con.
        float aspect = (float)w / (float)h;

        // We'll define the joycon in normalized window coordinates [0,1].
        // Center: x=0.25 (left side), y=0.5 (center vertically)
        // Size: width ~0.18 of screen, height ~0.8 of screen (so it looks tall)
        // Radius: smaller than half the width to keep crisp rounded corners.

        // If you want to adjust proportions, change these numbers.
        float centerX = 0.25f;
        float centerY = 0.5f;
        float sizeW = 0.18f;
        float sizeH = 0.8f;
        float radius = 0.06f; // corner radius

        // Convert radius to normalized units that match how the shader interprets distances
        // (shader uses same normalized UV 0..1 for both axes). So this is fine.

        glUniform2f(loc_center, centerX, centerY);
        glUniform2f(loc_size, sizeW, sizeH);
        glUniform1f(loc_radius, radius);

        // Red color for Joy-Con (tweak to preferred red)
        glUniform3f(loc_color, 0.85f, 0.12f, 0.16f); // a bold red

        // Edge softness (antialias) in normalized units; smaller -> crisper
        glUniform1f(loc_edge, 0.0025f);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        // Swap buffers
        glfwSwapBuffers(window);

        // Close on ESC
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
