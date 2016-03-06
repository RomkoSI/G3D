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
   - flip BMP vertical axis and BGR -> RGB
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
    const int numIndices = sizeof(Cube::index) / sizeof(Cube::index[0]);

    /////////////////////////////////////////////////////////////////////
    // Create the main shader
    const GLuint shader = createShaderProgram(loadTextFile("min.vrt"), loadTextFile("min.pix"));

    // Binding points for attributes and uniforms discovered from the shader
    const GLint positionAttribute                = glGetAttribLocation(shader,  "position");
    const GLint normalAttribute                  = glGetAttribLocation(shader,  "normal");
    const GLint texCoordAttribute                = glGetAttribLocation(shader,  "texCoord");
    const GLint tangentAttribute                 = glGetAttribLocation(shader,  "tangent");
    const GLint modelViewProjectionMatrixUniform = glGetUniformLocation(shader, "modelViewProjectionMatrix");
    const GLint objectToWorldNormalMatrixUniform = glGetUniformLocation(shader, "objectToWorldNormalMatrix");
    const GLint colorTextureUniform              = glGetUniformLocation(shader, "colorTexture");    

    // Load a texture map
    GLuint colorTexture = GL_NONE;
    {
        int textureWidth, textureHeight, channels;
        std::vector<std::uint8_t> data;
        loadBMP("color.bmp", textureWidth, textureHeight, channels, data);

        glGenTextures(1, &colorTexture);
        glBindTexture(GL_TEXTURE_2D, colorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, textureWidth, textureHeight, 0, (channels == 3) ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    GLuint trilinearSampler = GL_NONE;
    {
        glGenSamplers(1, &trilinearSampler);
        glSamplerParameteri(trilinearSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glSamplerParameteri(trilinearSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glSamplerParameteri(trilinearSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(trilinearSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

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
        
        const Matrix3x3& objectToWorldNormalMatrix = Matrix3x3(objectToWorldMatrix).transpose().inverse();

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

        // in position
        glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
        glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(positionAttribute);

        // in normal
        glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
        glVertexAttribPointer(normalAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(normalAttribute);

        // in tangent
        glBindBuffer(GL_ARRAY_BUFFER, tangentBuffer);
        glVertexAttribPointer(tangentAttribute, 4, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(tangentAttribute);

        // in texCoord 
        glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
        glVertexAttribPointer(texCoordAttribute, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(texCoordAttribute);

        // indexBuffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

        // uniform modelViewProjectionMatrix
        const Matrix4x4& modelViewProjectionMatrix = projectionMatrix * cameraToWorldMatrix.inverse() * objectToWorldMatrix;
        glUniformMatrix4fv(modelViewProjectionMatrixUniform, 1, GL_TRUE, modelViewProjectionMatrix.data);

        // uniform objectToWorldNormalMatrix
        glUniformMatrix3fv(objectToWorldNormalMatrixUniform, 1, GL_TRUE, objectToWorldNormalMatrix.data);

        // uniform colorTexture
        glUniform1i(colorTextureUniform, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorTexture);
        glBindSampler(0, trilinearSampler);

        glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0);

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
