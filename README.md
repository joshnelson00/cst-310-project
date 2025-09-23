# OpenGL 3D Entertainment Scene

This project is an early prototype of a 3D scene rendered in OpenGL. The scene represents an entertainment system with various objects. The current implementation uses rectangular prisms derived from screen coordinates and supports basic camera movement.

## Features

- Render 3D rectangular prisms from 2D screen coordinates.
- Simple color-based objects (textures to be added in future versions).
- Movable camera with forward, backward, and strafing controls.
- Camera height adjustment via mouse scroll.
- Reset camera to initial position with `Ctrl + C`.
- Depth testing and background wall rendering.

## Technologies Used

- **OpenGL** – Core rendering.
- **GLFW** – Window and input handling.
- **GLEW** – OpenGL extension management.
- **GLM** – Math library for vectors and matrices.
- **C++** – Core language.
- **Custom Shader Class** – Encapsulates vertex and fragment shaders.

## Controls

- `W` – Move forward
- `S` – Move backward
- `A` – Strafe left
- `D` – Strafe right
- Scroll mouse wheel – Move camera up/down
- `Ctrl + C` – Reset camera to initial position
- `ESC` – Close the application


## Setup Instructions

1. **Clone the repository:**
   ```bash
   git clone https://github.com/joshnelson00/cst-310-project.git
   cd cst-310-project
   ```
2. **Install dependencies**:
* GLFW
* GLEW
* GLM
* A C++17 compatible compiler
* CMake Build Compatible

3. Compile
```bash
g++ <FILENAME>.cpp -o <Binary Output Name> -I/usr/local/include -L/usr/local/lib -lGLEW -lglfw -lGL -lGLU /usr/local/lib/libSOIL.a
```

4. Run
```bash
./<Binary Output Name>
```

