/**
   This code sample shows how to use GLUT to initialize OpenGL.  It contains
   no G3D code.  This is primarily useful for tracking down GPU driver bugs
   or other issues for which you would like to rule out G3D's interactions.

   See also the "minGlut" G3D sample, which contains a small amount of 
   G3D code for support classes.

   Note that this project requires linking to GLUT, which is not
   formally distributed with G3D but is part of OS X and most Linux
   distributions.  The Windows version of GLUT is included in this
   directory.
 */
#include <stdio.h>
#include <string>
#include "glew.h"
#include "glut.h"
#include "G3D/Log.h"
#ifdef _MSC_VER
#   include <windows.h>
#   pragma comment(lib, "glut32.lib")
#endif

static const int WIDTH = 640;
static const int HEIGHT = 400;

static void quitOnEscape(unsigned char key, int x, int y) {
    if (key == 27) { ::exit(0); }
}

/* Creates a shader object of the specified type using the specified text
 */
static GLuint make_shader(GLenum type, const char* shader_src) {
    GLuint shader;
    GLint shader_ok;
    GLsizei log_length;
    char info_log[8192];

    shader = glCreateShader(type);
    if (shader != 0) {
        glShaderSource(shader, 1, (const GLchar**)&shader_src, NULL);
        glCompileShader(shader);
        glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_ok);
        if (shader_ok != GL_TRUE) {
            logPrintf("Failed shader source code:\n %s\n", shader_src);
            fprintf(stderr, "ERROR: Failed to compile %s shader\n", (type == GL_FRAGMENT_SHADER) ? "fragment" : "vertex" );
            glGetShaderInfoLog(shader, 8192, &log_length, info_log);
            fprintf(stderr, "ERROR: \n%s\n\n", info_log);
            glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}

/* Creates a program object using the specified vertex and fragment text
 */
static GLuint make_shader_program(const char* vertex_shader_src, const char* fragment_shader_src) {
    GLuint program = 0u;
    GLint program_ok;
    GLuint vertex_shader = 0u;
    GLuint fragment_shader = 0u;
    GLsizei log_length;
    char info_log[8192];

    vertex_shader = make_shader(GL_VERTEX_SHADER, vertex_shader_src);
    if (vertex_shader != 0u)
    {
        fragment_shader = make_shader(GL_FRAGMENT_SHADER, fragment_shader_src);
        if (fragment_shader != 0u)
        {
            /* make the program that connect the two shader and link it */
            program = glCreateProgram();
            if (program != 0u)
            {
                /* attach both shader and link */
                glAttachShader(program, vertex_shader);
                glAttachShader(program, fragment_shader);
                glLinkProgram(program);
                glGetProgramiv(program, GL_LINK_STATUS, &program_ok);

                if (program_ok != GL_TRUE)
                {
                    fprintf(stderr, "ERROR, failed to link shader program\n");
                    glGetProgramInfoLog(program, 8192, &log_length, info_log);
                    fprintf(stderr, "ERROR: \n%s\n\n", info_log);
                    glDeleteProgram(program);
                    glDeleteShader(fragment_shader);
                    glDeleteShader(vertex_shader);
                    program = 0u;
                }
                glDetachShader(program, vertex_shader);
                glDetachShader(program, fragment_shader);
            }
        }
        else
        {
            fprintf(stderr, "ERROR: Unable to load fragment shader\n");
            glDeleteShader(vertex_shader);
        }
    }
    else
    {
        fprintf(stderr, "ERROR: Unable to load vertex shader\n");
    }
    if (program == 0u) {
        fprintf(stderr, "ERROR: Unable to create shader\n");
    }
    return program;
}

static void generateGeometryAndBindBuffers(GLuint program) {
    GLuint attrloc;

    static GLuint mesh;
    static GLuint mesh_vbo;
    static GLuint mesh_ibo;
    static GLuint mesh_indices[6] = {   0u, 2u, 1u,
                                        1u, 2u, 3u};
    static GLfloat mesh_vertices[16] = {    -1.0f, -1.0f, 0.0f, 1.0f,
                                            -1.0f, 1.0f, 0.0f, 1.0f,
                                            1.0f, -1.0f, 0.0f, 1.0f,
                                            1.0f, 1.0f, 0.0f, 1.0f  };
    glGenVertexArrays(1, &mesh);
    glGenBuffers(1, &mesh_vbo);
    glGenBuffers(1, &mesh_ibo);
    glBindVertexArray(mesh);
    /* Prepare the data for drawing through a buffer inidices */
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * 6, mesh_indices, GL_STATIC_DRAW);

    /* Prepare the attributes for rendering */
    attrloc = glGetAttribLocation(program, "position");
    //fprintf(stderr, "attrLoc: %d\n",attrloc);
    glBindBuffer(GL_ARRAY_BUFFER, mesh_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16, mesh_vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(attrloc);
    glVertexAttribPointer(attrloc, 4, GL_FLOAT, GL_FALSE, 0, 0);
}

static void render() {
    static const char* vertex_shader =
        "#version 330\n"
        "in vec4 position;\n"
        "\n"
        "void main()\n"
        "{\n"
        "   gl_Position = position;\n"
        "}\n";

    static const char* fragment_shader =
        "#version 330\n"
        "void main()\n"
        "{\n"
        "    float y  = gl_FragCoord.y / 400.0;\n"
        "    float d_y = dFdy(gl_FragCoord.y);\n"
        "    float d_x = dFdx(gl_FragCoord.y);\n"
        "    gl_FragColor = vec4(d_y, d_y, y, 1.0); \n"
        "}\n";

    static GLuint program   = make_shader_program(vertex_shader, fragment_shader);
    static bool initialized = false;
    static GLuint fb        = 0;
    static GLuint tex       = 0;
    
    if (!initialized) {
        glGenFramebuffers(1, &fb);
        glBindFramebuffer(GL_FRAMEBUFFER, fb);

        glGenTextures(1, &tex);
        // "Bind" the newly created texture : all future texture functions will modify this texture
        glBindTexture(GL_TEXTURE_2D, tex);
        // Give an empty image to OpenGL ( the last "0" )
        glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, WIDTH, HEIGHT, 0,GL_RGB, GL_UNSIGNED_BYTE, 0);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex, 0);
 
        static GLenum drawBuffer = GL_COLOR_ATTACHMENT0;

        glDrawBuffers(1, &drawBuffer);
        // Always check that our framebuffer is ok
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            fprintf(stderr, "ERROR: Unable to setup framebuffer\n");
        }

        glUseProgram(program);
        generateGeometryAndBindBuffers(program);
        glViewport(0, 0, WIDTH, HEIGHT);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        initialized = true;
    }
    glUseProgram(program);
    // Render to our framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fb);
    glBlitFramebuffer(0,0,WIDTH,HEIGHT,0,0,WIDTH,HEIGHT,GL_COLOR_BUFFER_BIT, GL_NEAREST);
    // Put your rendering code here

    glutSwapBuffers();
}

int main(int argc, char** argv) {
    
    // Initialize OpenGL
    glutInit(&argc, argv);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutCreateWindow("OpenGL");

    // Initialize OpenGL extensions
    glewInit();

    // Set GLUT callbacks
    glutKeyboardFunc(&quitOnEscape);
    glutDisplayFunc(&render);

    // Never returns
    glutMainLoop();

    return 0;
}


#ifdef _MSC_VER

// Make Microsoft Windows programs start from the main() entry point
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw) {
    const int argc = 1;
    char** argv = new char*[argc];
    argv[0] = "OpenGL";
    
    int r = main(argc, argv);
    delete[] argv;

    return r;
}

#endif
