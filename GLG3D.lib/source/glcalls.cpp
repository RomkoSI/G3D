/**
  @file glcalls.cpp

  @maintainer Morgan McGuire, http://graphics.cs.williams.edu
  @created 2002-08-07
  @edited  2009-06-01
*/

#include "G3D/Matrix3.h"
#include "G3D/Matrix4.h"
#include "G3D/AABox.h"
#include "G3D/CoordinateFrame.h"
#include "GLG3D/glcalls.h"
#include "GLG3D/getOpenGLState.h"
#include "GLG3D/GLCaps.h"

#if defined(G3D_OSX)
#include <mach-o/dyld.h>
// Implemented in glew.c
extern "C" void* NSGLGetProcAddress(const GLubyte*);
#endif

namespace G3D {

#if defined(G3D_LINUX) || defined(G3D_FREEBSD)
    Display* OpenGLDisplay = NULL;
    GLXDrawable OpenGLDrawable;
#elif defined(G3D_WINDOWS)
    HDC OpenGLWindowHDC;
#endif


/**
 Sets up matrix m from rot and trans
 */
static void _getGLMatrix(GLfloat* m, const Matrix3& rot, const Vector3& trans) {
    // GL wants a column major matrix
    m[0] = rot[0][0];
    m[1] = rot[1][0];
    m[2] = rot[2][0];
    m[3] = 0.0f;

    m[4] = rot[0][1];
    m[5] = rot[1][1];
    m[6] = rot[2][1];
    m[7] = 0.0f;

    m[8] = rot[0][2];
    m[9] = rot[1][2];
    m[10] = rot[2][2];
    m[11] = 0.0f;

    m[12] = trans[0];
    m[13] = trans[1];
    m[14] = trans[2];
    m[15] = 1.0f;
}


void glGetMatrix(GLenum name, Matrix4& m) {
    float f[16];
    glGetFloatv(name, f);
    debugAssertGLOk();
    m = Matrix4(f).transpose();
}


Matrix4 glGetMatrix(GLenum name) {
    Matrix4 m;
    glGetMatrix(name, m);
    return m;
}


void glLoadMatrix(const CoordinateFrame &cf) {
    GLfloat matrix[16];
    _getGLMatrix(matrix, cf.rotation, cf.translation);
    glLoadMatrixf(matrix);
}


void glLoadMatrix(const Matrix4& m) {
    GLfloat matrix[16];
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            // Transpose
            matrix[c * 4 + r] = m[r][c];
        }
    }

    glLoadMatrixf(matrix);
}


void glLoadInvMatrix(const CoordinateFrame &cf) {
    Matrix3 rotInv = cf.rotation.transpose();

    GLfloat matrix[16];
    _getGLMatrix(matrix, rotInv, rotInv * -cf.translation);
    glLoadMatrixf(matrix);
}

void glMultInvMatrix(const CoordinateFrame &cf) {
    Matrix3 rotInv = cf.rotation.transpose();

    GLfloat matrix[16];
    _getGLMatrix(matrix, rotInv, rotInv * -cf.translation);
    glMultMatrixf(matrix);
}

void glMultMatrix(const CoordinateFrame &cf) {
    GLfloat matrix[16];
    _getGLMatrix(matrix, cf.rotation, cf.translation);
    glMultMatrixf(matrix);
}


Vector2 glGetVector2(GLenum which) {
    float v[4];
    glGetFloatv(which, v);
    return Vector2(v[0], v[1]);
}


Vector3 glGetVector3(GLenum which) {
    float v[4];
    glGetFloatv(which, v);
    return Vector3(v[0], v[1], v[2]);
}


Vector4 glGetVector4(GLenum which) {
    float v[4];
    glGetFloatv(which, v);
    return Vector4(v[0], v[1], v[2], v[3]);
}


/**
 Takes an object space point to screen space using the current MODELVIEW and
 PROJECTION matrices. The resulting xy values are in <B>pixels</B> and
 are relative to the viewport origin, the z 
 value is on the glDepthRange scale, and the w value contains rhw (-1/z for
 camera space z), which is useful for scaling line and point size.
 */
Vector4 glToScreen(const Vector4& v) {
    
    // Get the matrices and viewport
    double modelView[16];
    double projection[16];
    double viewport[4];
    double depthRange[2];

    glGetDoublev(GL_MODELVIEW_MATRIX, modelView);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetDoublev(GL_VIEWPORT, viewport);
    glGetDoublev(GL_DEPTH_RANGE, depthRange);

    // Compose the matrices into a single row-major transformation
    Vector4 T[4];
    int r, c, i;
    for (r = 0; r < 4; ++r) {
        for (c = 0; c < 4; ++c) {
            T[r][c] = 0;
            for (i = 0; i < 4; ++i) {
                // OpenGL matrices are column major
                T[r][c] += float(projection[r + i * 4] * modelView[i + c * 4]);
            }
        }
    }

    // Transform the vertex
    Vector4 result;
    for (r = 0; r < 4; ++r) {
        result[r] = T[r].dot(v);
    }

    // Homogeneous divide
    const double rhw = 1 / result.w;

    return Vector4(
        float((1 + result.x * rhw) * viewport[2] / 2 + viewport[0]),
        float((1 - result.y * rhw) * viewport[3] / 2 + viewport[1]),
        float((result.z * rhw) * (depthRange[1] - depthRange[0]) + depthRange[0]),
        (float)rhw);
}

void glDisableAllTextures() {
    glDisable(GL_TEXTURE_2D);
    if (GLCaps::supports_GL_EXT_texture3D()) {
        glDisable(GL_TEXTURE_3D);
    }
    if (GLCaps::supports_GL_EXT_texture_cube_map()) {
        glDisable(GL_TEXTURE_CUBE_MAP_ARB);
    }
    glDisable(GL_TEXTURE_1D);
    
    if (GLCaps::supports_GL_EXT_texture_rectangle()) {
        glDisable(GL_TEXTURE_RECTANGLE_ARB);
    }
}

} // namespace

extern void (*glXGetProcAddress (const GLubyte *))( void );
extern void (*glXGetProcAddressARB (const GLubyte *))( void );

namespace G3D {
void* glGetProcAddress(const char * name) {
    #if defined(G3D_WINDOWS)
        return (void *)wglGetProcAddress(name);
    #elif defined(G3D_LINUX)
        //#ifndef GLX_VERSION_1_4
            return (void *)glXGetProcAddressARB((const GLubyte*)name);
        //#else
            //return (void *)glXGetProcAddress((const GLubyte*)name);
        //#endif
    #elif defined(G3D_OSX)
        return ::NSGLGetProcAddress((const GLubyte*)name);
    #else
        return NULL;
    #endif
}


void glClipToBox(const AABox& box) {
    // Clip to bounds cube                                                                                   
    double eq[4];
    for (int i = 0; i < 6; ++i) {
        // Compute the equation of one of the box planes. The normal is an axis vector:                      
        const int axis = i % 3;
        eq[0] = eq[1] = eq[2] = 0.0;
        eq[axis] = sign(i - 2.5);
        
        // The offset is the position along that axis                                                        
        eq[3] = -(eq[axis] * box.center()[axis] - box.extent()[axis] / 2);
        
        // Clipping planes are specified in object space                                                     
        glClipPlane(i + GL_CLIP_PLANE0, eq);
        glEnable(i + GL_CLIP_PLANE0);
    }
}
    

void glDisableAllClipping() {
    for (int i = GL_CLIP_PLANE0; i <= GL_CLIP_PLANE5; ++i) {
        glDisable(i);
    }
}

} // namespace 

