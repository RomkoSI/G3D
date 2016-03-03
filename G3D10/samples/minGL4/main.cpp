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

  See a SDL-based minimal OpenGL program at:
     https://gist.github.com/manpat/112f3f31c983ccddf044

  TODO:
    - Fix ortho projection
    - Render to HDR framebuffer and then post + gamma to the screen
    - Draw cube
    - Draw ground plane
    - Render background sphere
 */

#include "minGL4.h"
#include <iostream>

GLFWwindow* window = nullptr;

void APIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    if ((type == GL_DEBUG_TYPE_ERROR) || (type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)) {
        fprintf(stderr, "GL Debug: %s\n", message);
    }
}


int main(const int argc, const char* argv[]) {
    window = initOpenGL(1280, 720, "minGL4");

#   ifdef _DEBUG
       glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
       glEnable(GL_DEBUG_OUTPUT);
       // Causes a segmentation fault on OS X
       // glDebugMessageCallback(debugCallback, nullptr);
#   endif

    const Vector3 cpuPosition[] = {
        /*
        // Right triangle in pixel coords:
        Vector3(0, 0, 0),
        Vector3(1280, 0, 0),
        Vector3(0, 720, 0)
        */

        // Isoscoles triangle:
        Vector3( 0.0f,  1.0f, 0.0f),
        Vector3(-1.0f, -1.0f, 0.0f),
        Vector3( 1.0f, -1.0f, 0.0f)
       

        // Right triangle:
        /*
        Vector3( 0.0f,  0.0f, 0.0f),
        Vector3( 1.0f,  0.0f, 0.0f),
        Vector3( 0.0f,  1.0f, 0.0f)
        */
    };

    const Vector3 cpuColor[] = {
        Vector3( 1.0f, 0.0f, 0.0f),
        Vector3( 0.0f, 1.0f, 0.0f),
        Vector3( 0.0f, 0.0f, 1.0f),
    };

    // Bind a single global vertex array (done this way since OpenGL 3)
    {
        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
    }

    // Create separate vertex buffers for each attribute
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

    // Binding points for attributes and uniforms discovered from the shader
    const GLint positionAttribute                = glGetAttribLocation(shader,  "position");
    const GLint colorAttribute                   = glGetAttribLocation(shader,  "color");
    const GLint modelViewProjectionMatrixUniform = glGetUniformLocation(shader, "modelViewProjectionMatrix");

    int windowWidth = 1, windowHeight = 1;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    // Init framebuffer and targets
    GLuint framebuffer = GL_NONE;
    {
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        
        GLuint colorBuffer = GL_NONE;
        glGenTextures(1, &colorBuffer);
        glBindTexture(GL_TEXTURE_2D, colorBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, windowWidth, windowHeight, 0, GL_RGBA16F, GL_FLOAT, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);
        
        GLuint depthBuffer = GL_NONE;
        glGenTextures(1, &depthBuffer);
        glBindTexture(GL_TEXTURE_2D, depthBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, windowWidth, windowHeight, 0, GL_R32F, GL_FLOAT, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthBuffer, 0);
        
        // Restore the hardware framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Main loop:
    int timer = 0;
    while (! glfwWindowShouldClose(window)) {
        // Render to the off-screen HDR framebuffer
        // glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);

        const float nearPlaneZ = -0.1f;
        const float farPlaneZ = -100.0f;
        const float verticalFieldOfView = 45.0f * M_PI / 180.0f;

        glClearColor(0.1f, 0.2f, 0.3f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw the background
        drawSky(windowWidth, windowHeight, nearPlaneZ, farPlaneZ, verticalFieldOfView);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_TRUE);

        const Matrix4x4 objectToWorldMatrix;// = Matrix4x4::pitch(timer * 0.015f) * Matrix4x4::roll(timer * 0.01f);
        const Matrix4x4& worldToCameraMatrix = Matrix4x4::translate(0, 0, -3.0f);
        const Matrix4x4& projectionMatrix =
            Matrix4x4::perspective(windowWidth, windowHeight, nearPlaneZ, farPlaneZ, verticalFieldOfView);
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

        /*
        // Blit the HDR framebuffer to the LDR hardware framebuffer
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, windowWidth, windowHeight, 
                          0, 0, windowWidth, windowHeight, 
                          GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
                          GL_NEAREST);
        */

        // Display what has been drawn
        glfwSwapBuffers(window);

        // Handle events
        if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, 1);
        }

        ++timer;
    }

    // Close the GL context and release all resources
    glfwTerminate();
    return 0;
}
