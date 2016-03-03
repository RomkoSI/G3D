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

 */

#include "minGL4.h"

int main(const int argc, const char* argv[]) {
    // Start GL context and O/S window using the GLFW helper library
    if (! glfwInit()) {
        fprintf (stderr, "ERROR: could not start GLFW3\n");
        return 1;
    } 

#   ifdef G3D_OSX
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,    3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,    2);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,    GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE,           GLFW_OPENGL_CORE_PROFILE);
#   endif

    GLFWwindow* window = glfwCreateWindow(1280, 720, "minGL4", NULL, NULL);
    if (! window) {
        fprintf(stderr, "ERROR: could not open window with GLFW3\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
                                  
    // Start GLEW extension handler, with improved support for new features
    glewExperimental = GL_TRUE;
    glewInit();

    printf("Renderer:       %s\n", glGetString(GL_RENDERER));
    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    /* OTHER STUFF GOES HERE NEXT */
  
    // close GL context and any other GLFW resources
    glfwTerminate();
    return 0;
}
