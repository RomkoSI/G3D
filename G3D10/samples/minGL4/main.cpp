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

int main(const int argc, const char* argv[]) {
    window = initOpenGL(1280, 720, "minGL4");

    float points[] = {
        0.0f,  0.5f,  0.0f,
        0.5f, -0.5f,  0.0f,
        -0.5f, -0.5f,  0.0f
    };

    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 9 * sizeof (float), points, GL_STATIC_DRAW);

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glEnableVertexAttribArray (0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    GLuint shader = loadShader("min.vrt", "min.pix");

    Matrix4x4 objectToWorldMatrix;
    Matrix4x4 worldToCameraMatrix;
    Matrix4x4 projectionMatrix;

    // Main loop:
    while (! glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        
        glUseProgram(shader);
        glBindVertexArray(vao);

        glDrawArrays(GL_TRIANGLES, 0, 3);

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
