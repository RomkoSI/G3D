/**
  \file minGL4/minGL4.h
  \author Morgan McGuire, http://graphics.cs.williams.edu

  Minimal headers emulating a basic set of 3D graphics classes.
*/
#pragma once

#ifdef __APPLE__
#   define G3D_OSX
#elif defined(_WIN64)
#   define G3D_WINDOWS
#elif defined(__linux__)
#   define G3D_LINUX
#endif

#include <GL/glew.h>
#ifdef G3D_WINDOWS
#   include <GL/wglew.h>
#elif defined(G3D_LINUX)
#   include <GL/xglew.h>
#endif
#include <GLFW/glfw3.h> 

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <fstream>
#include <sstream>

class Vector3 {
public:
    float x, y, z;
    /** initializes to zero */
    Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    explicit Vector3(const class Vector4&);
    float dot(const Vector3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }
    float& operator[](int i) {
        return (&x)[i];
    }
    float operator[](int i) const {
        return (&x)[i];
    }
};


class Vector4 {
public:
    float x, y, z, w;
    /** initializes to zero */
    Vector4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    Vector4(const Vector3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}
    float dot(const Vector4& other) const {
        return x * other.x + y * other.y + z * other.z + w * other.w;
    }
    float& operator[](int i) {
        return (&x)[i];
    }
    float operator[](int i) const {
        return (&x)[i];
    }
};


Vector3::Vector3(const Vector4& v) : x(v.x), y(v.y), z(v.z) {}

class Matrix4x4 {
public:
    /** row-major */
    float data[16];

    /** initializes to the identity matrix */
    Matrix4x4() {
        memset(data, 0, sizeof(float) * 16);
        data[0] = data[5] = data[10] = data[15] = 1.0f;
    }

    float& operator()(int r, int c) {
        return data[r * 4 + c];         
    }

    float operator()(int r, int c) const {
        return data[r * 4 + c];         
    }

    Vector4 row(int r) const {
        const int i = r * 4;
        return Vector4(data[i], data[i + 1], data[i + 2], data[i + 3]);
    }

    Vector4 col(int c) const {
        return Vector4(data[c], data[c + 4], data[c + 8], data[c + 12]);
    }

    Matrix4x4 operator*(const Matrix4x4& B) const {
        Matrix4x4 D;
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                D(r, c) = row(r).dot(B.col(c));
            }
        }
        return D;
    }

    Vector4 operator*(const Vector4& v) const {
        Vector4 d;
        for (int r = 0; r < 4; ++r) {
            d[r] = row(r).dot(v);
        }
        return d;
    }
};



GLFWwindow* initOpenGL(int width, int height, const std::string& title) {
    if (! glfwInit()) {
        fprintf(stderr, "ERROR: could not start GLFW\n");
        ::exit(1);
    } 

    // Without these, shaders actually won't initialize properly
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
    if (! window) {
        fprintf(stderr, "ERROR: could not open window with GLFW\n");
        glfwTerminate();
        ::exit(2);
    }
    glfwMakeContextCurrent(window);

                                  
    // Start GLEW extension handler, with improved support for new features
    glewExperimental = GL_TRUE;
    glewInit();

    printf("Renderer:       %s\n", glGetString(GL_RENDERER));
    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));

    return window;
}


std::string loadTextFile(const std::string& filename) {
    std::stringstream buffer;
    buffer << std::ifstream(filename.c_str()).rdbuf();
    return buffer.str();
}


GLuint loadShader(const std::string& vertexFilename, const std::string& pixelFilename) {
    GLuint shader = glCreateProgram();

    const std::string& vertexShaderSource = loadTextFile(vertexFilename);
    const std::string& pixelShaderSource  = loadTextFile(pixelFilename);

    const char* vSrcArray[] = {vertexShaderSource.c_str()};
    const char* pSrcArray[] = {pixelShaderSource.c_str()};
    
    GLuint vs = glCreateShader (GL_VERTEX_SHADER);
    glShaderSource(vs, 1, vSrcArray, NULL);
    glCompileShader(vs);
    
    GLuint ps = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(ps, 1, pSrcArray, NULL);
    glCompileShader(ps);
    
    glAttachShader(shader, ps);
    glAttachShader(shader, vs);
    glLinkProgram(shader);

    return shader;
}
