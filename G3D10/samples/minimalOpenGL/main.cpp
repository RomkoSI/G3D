/**
  \file minimalOpenGL/main.cpp
  \author Morgan McGuire, http://graphics.cs.williams.edu

  Features demonstrated:
   * Window, OpenGL, and extension initialization
   * Triangle mesh rendering (GL Vertex Array Buffer)
   * Texture map loading (GL Texture Object)
   * Shader loading (GL Program and Shader Objects)
   * Ray tracing
   * Procedural texture
   * Tiny vector math library
   * Mouse and keyboard handling

  This is a minimal example of an OpenGL 4 program using only
  GLFW and GLEW libraries to simplify initialization. It does
  not depend on G3D or any other external libraries at all. 
  You could use SDL or another thin library instead of those two.
  
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
   - Show how to load a texture map
*/

#include "minimalOpenGL.h"

GLFWwindow* window = nullptr;
const int windowWidth = 1280, windowHeight = 720;

int main(const int argc, const char* argv[]) {
    std::cout << "Minimal OpenGL 4.1 Example by Morgan McGuire\nW, A, S, D, C, Z keys to translate\nMouse click and drag to rotate\n";
    std::cout << std::fixed;

    window = initOpenGL(windowWidth, windowHeight, "minimalOpenGL");

    Vector3 cameraTranslation(0.0f, 1.5f, 5.0f);
    Vector3 cameraRotation;

    /////////////////////////////////////////////////////////////////
    // Load vertex array buffers
    GLuint positionBuffer = GL_NONE;
    glGenBuffers(1, &positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Cube::position), Cube::position, GL_STATIC_DRAW);

    GLuint texCoordBuffer = GL_NONE;
    glGenBuffers(1, &texCoordBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Cube::texCoord), Cube::texCoord, GL_STATIC_DRAW);

    GLuint normalBuffer = GL_NONE;
    glGenBuffers(1, &normalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Cube::normal), Cube::normal, GL_STATIC_DRAW);

    GLuint tangentBuffer = GL_NONE;
    glGenBuffers(1, &tangentBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, tangentBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Cube::tangent), Cube::tangent, GL_STATIC_DRAW);

    const int numVertices = sizeof(Cube::position) / sizeof(Cube::position[0]);

    GLuint indexBuffer = GL_NONE;
    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Cube::index), Cube::index, GL_STATIC_DRAW);

    /////////////////////////////////////////////////////////////////////
    // Create the main shader
    const GLuint shader = loadShaderProgram("min.vrt", "min.pix");

    // Binding points for attributes and uniforms discovered from the shader
    const GLint positionAttribute                = glGetAttribLocation(shader,  "position");
    const GLint normalAttribute                  = glGetAttribLocation(shader,  "normal");
    const GLint texCoordAttribute                = glGetAttribLocation(shader,  "texCoord");
    const GLint tangentAttribute                 = glGetAttribLocation(shader,  "tangent");
    const GLint modelViewProjectionMatrixUniform = glGetUniformLocation(shader, "modelViewProjectionMatrix");


    // Main loop:
    int timer = 0;
    while (! glfwWindowShouldClose(window)) {
        const float nearPlaneZ = -0.1f;
        const float farPlaneZ = -100.0f;
        const float verticalFieldOfView = 45.0f * PI / 180.0f;

        glClearColor(0.1f, 0.2f, 0.3f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const Matrix4x4& objectToWorldMatrix = Matrix4x4::translate(0.0f, 0.5f, 0.0f) * Matrix4x4::yaw(PI / 4.0f);

        const Matrix4x4& cameraToWorldMatrix =
            Matrix4x4::translate(cameraTranslation) *
            Matrix4x4::roll(cameraRotation.z) *
            Matrix4x4::yaw(cameraRotation.y) *
            Matrix4x4::pitch(cameraRotation.x);
        
        const Matrix4x4& projectionMatrix = Matrix4x4::perspective(float(windowWidth), float(windowHeight), nearPlaneZ, farPlaneZ, verticalFieldOfView);

        // Draw the background
        drawSky(windowWidth, windowHeight, nearPlaneZ, farPlaneZ, verticalFieldOfView, cameraToWorldMatrix);

        ////////////////////////////////////////////////////////////////////////
        // Draw a mesh
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_TRUE);
        
        glUseProgram(shader);

        const Matrix4x4& modelViewProjectionMatrix = projectionMatrix * cameraToWorldMatrix.inverse() * objectToWorldMatrix;
        glUniformMatrix4fv(modelViewProjectionMatrixUniform, 1, GL_TRUE, modelViewProjectionMatrix.data);

        glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
        glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(positionAttribute);

        glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
        glVertexAttribPointer(normalAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(normalAttribute);

        glBindBuffer(GL_ARRAY_BUFFER, tangentBuffer);
        glVertexAttribPointer(tangentAttribute, 4, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(tangentAttribute);

        glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
        glVertexAttribPointer(texCoordAttribute, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(texCoordAttribute);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

        glDrawElements(GL_TRIANGLES, sizeof(Cube::index) / sizeof(Cube::index[0]), GL_UNSIGNED_INT, 0);

        ////////////////////////////////////////////////////////////////////////

        // Check for events
        glfwPollEvents();

        // Display what has been drawn
        glfwSwapBuffers(window);

        // Handle events
        if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, 1);
        }

        // WASD keyboard movement
        const float cameraMoveSpeed = 0.01f;
        if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_W)) { cameraTranslation += Vector3(cameraToWorldMatrix * Vector4(0, 0, -cameraMoveSpeed, 0)); }
        if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_S)) { cameraTranslation += Vector3(cameraToWorldMatrix * Vector4(0, 0, +cameraMoveSpeed, 0)); }
        if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_A)) { cameraTranslation += Vector3(cameraToWorldMatrix * Vector4(-cameraMoveSpeed, 0, 0, 0)); }
        if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_D)) { cameraTranslation += Vector3(cameraToWorldMatrix * Vector4(+cameraMoveSpeed, 0, 0, 0)); }
        if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_C)) { cameraTranslation.y -= cameraMoveSpeed; }
        if ((GLFW_PRESS == glfwGetKey(window, GLFW_KEY_SPACE)) || (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_Z))) { cameraTranslation.y += cameraMoveSpeed; }

        // Keep the camera above the ground
        if (cameraTranslation.y < 0.01f) { cameraTranslation.y = 0.01f; }

        static bool inDrag = false;
        const float cameraTurnSpeed = 0.005f;
        if (GLFW_PRESS == glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)) {
            static double startX, startY;
            double currentX, currentY;

            glfwGetCursorPos(window, &currentX, &currentY);
            if (inDrag) {
                cameraRotation.y -= float(currentX - startX) * cameraTurnSpeed;
                cameraRotation.x -= float(currentY - startY) * cameraTurnSpeed;
            }
            inDrag = true; startX = currentX; startY = currentY;
        } else {
            inDrag = false;
        }

        ++timer;
    }

    // Close the GL context and release all resources
    glfwTerminate();
    return 0;
}


#ifdef _WINDOWS
    int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw) {
        return main(0, nullptr);
    }
#endif



#if 0

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
#endif