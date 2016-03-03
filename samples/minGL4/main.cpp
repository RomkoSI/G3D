/**
  \file minGL4/main.cpp
  \author Morgan McGuire, http://graphics.cs.williams.edu

  This is a minimal example of an OpenGL 4 program using only
  GLFW and GLEW libraries to simplify initialization. It does
  not depend on G3D at all. You could use SDL or another thin
  library instead of those two.
  
  This is useful as a testbed when isolating driver bugs and 
  seeking a minimal context. 

  It is also helpful if you're new to computer graphics and wish to
  see the underlying hardware API without the high-level features that
  G3D provides.

  I chose OpenGL 4.1 because it is the newest OpenGL available on OS
  X, and thus the newest OpenGL that can be used across the major PC
  operating systems of Windows, Linux, OS X, and Steam.

  See the stb libraries for single-header, dependency-free support
  for image loading, parsing, fonts, noise, etc.:

     https://github.com/nothings/stb
 */

#include "minGL4.h"

GLFWwindow* window = nullptr;

void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    fprintf(stderr, "GL Debug: %s\n", message);
}

int main(const int argc, const char* argv[]) {
    window = initOpenGL(1280, 720, "minGL4");

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(debugCallback, nullptr);

    const Vector3 cpuPosition[] = {
        /*
        Vector3(0, 0, -1),
        Vector3(1000, 0, -1),
        Vector3(0, 1000, -1)
        */
        /*
        Vector3( 0.0f,  2.5f, -10.0f),
        Vector3(-0.7f, -1.0f, -10.0f),
        Vector3( 0.7f, -1.0f, -10.0f)
        */
        Vector3( 0.0f,  0.0f, -2.0f),
        Vector3( 1.0f,  0.0f, -2.0f),
        Vector3( 0.0f,  1.0f, -2.0f)
    };

    const Vector3 cpuNormal[] = {
        Vector3( 1.0f, 0.0f, 0.0f),
        Vector3( 0.0f, 1.0f, 0.0f),
        Vector3( 0.0f, 0.0f, 1.0f),
    };

    // Bind a single vertex array (done this way since OpenGL 3)
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    const int N = 3;
    GLuint positionBuffer = GL_NONE;
    glGenBuffers(1, &positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, N * sizeof(Vector3), cpuPosition, GL_STATIC_DRAW);

    GLuint colorBuffer = GL_NONE;
    glGenBuffers(1, &colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, N * sizeof(Vector3), cpuColor, GL_STATIC_DRAW);

    const GLuint shader = loadShader("min.vrt", "min.pix");

    // Binding points for attributes and uniforms
    const GLint positionAttribute                = glGetAttribLocation(shader, "position");
    const GLint colorAttribute                  = glGetAttribLocation(shader, "color");
    const GLint modelViewProjectionMatrixUniform = glGetUniformLocation(shader, "modelViewProjectionMatrix");

    // Main loop:
    while (! glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDisable(GL_CULL_FACE);

        int windowWidth = 1, windowHeight = 1;
        const float nearPlaneZ = -0.1f;
        const float farPlaneZ = -100.0f;
        const float verticalFieldOfView = 60.0f * M_PI / 180.0f;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);

        Matrix4x4 objectToWorldMatrix;
        Matrix4x4 worldToCameraMatrix;
        Matrix4x4 projectionMatrix = Matrix4x4::perspective(windowWidth, windowHeight, nearPlaneZ, farPlaneZ, verticalFieldOfView);
            //Matrix4x4::ortho(windowWidth, windowHeight, nearPlaneZ, farPlaneZ);
       
        glUseProgram(shader);

        const Matrix4x4& modelViewProjectionMatrix = projectionMatrix * worldToCameraMatrix * objectToWorldMatrix;
        glUniformMatrix4fv(modelViewProjectionMatrixUniform, 1, GL_TRUE, modelViewProjectionMatrix.data);

        glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
        glVertexAttribPointer(positionAttribute, N, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(positionAttribute);

        glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
        glVertexAttribPointer(colorAttribute, N, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(colorAttribute);

        glDrawArrays(GL_TRIANGLES, 0, N);

        // Check for events
        glfwPollEvents();

        // Display what has been drawn
        glfwSwapBuffers(window);

        // Handle events
        if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, 1);
        }
    }

    // Close the GL context and release all resources
    glfwTerminate();
    return 0;
}
