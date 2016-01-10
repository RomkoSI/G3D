/**
 @file tesselate.cpp
 
 @maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
 @created 2003-05-01
 @edited  2005-02-24
 */

#include "G3D/platform.h"
#include "GLG3D/tesselate.h"
#include "G3D/Vector3.h"
#include "G3D/Array.h"
#include "G3D/Triangle.h"
#include "GLG3D/glheaders.h"

namespace G3D {

/** Data passed to the _tesselate callbacks. Because the GLU tesselator
    generates both the triangles and the outline of a tesselated polygon.
    We only want the former, so we use a state bit to track which
    we are inside of. */
class TessData {
public:
    class Primitive {
    public:
        GLenum              primitiveType;
        Array<Vector3>      vertex;
        Primitive() : primitiveType(GL_NONE) {}
        Primitive(GLenum e) : primitiveType(e) {}
    };

    Array<Primitive>        primitive;

    /** Vertices used by the combine callback.*/
    Array<Vector3>          allocStack;

};

static  void __stdcall _tesselateBegin(GLenum e, TessData* data) {
    if (e == GL_TRIANGLES || e == GL_TRIANGLE_FAN || e == GL_TRIANGLE_STRIP) {
        data->primitive.append(TessData::Primitive(e));
    }
}

static void __stdcall _tesselateVertex(Vector3* v, TessData* data) {
    data->primitive.last().vertex.append(*v);
}

static void __stdcall _tesselateEnd(TessData* data) {
    (void)data;
}

/**
 Called by the GLU tesselator when an intersection is detected to
 synthesize a new vertex.
 */
static void __stdcall _tesselateCombine(
    GLdouble        coords[3],
    void*           dummy[4], 
    GLfloat         w[4],
    Vector3**       vertex,
    TessData*       data) {

    (void)w;
    (void)dummy;

    // Just copy data from the coordinates
    data->allocStack.append(Vector3((float)coords[0], (float)coords[1], (float)coords[2]));
    *vertex = &(data->allocStack[data->allocStack.size() - 1]);
}

static void __stdcall _tesselateError(GLenum e) {
    // This "should" never be called
    if (e == GLU_TESS_NEED_COMBINE_CALLBACK) {
        debugAssertM(false, "GLU_TESS_NEED_COMBINE_CALLBACK");
    } else {
        //debugAssertM(false, GLenumToString(e));
        debugAssert(false);
    }
}


void tesselateComplexPolygon(const Array<Vector3>& input, Array<Triangle>& output) {
    // Use the GLU triangulator to do the hard work.

    static GLUtriangulatorObj* tobj = NULL;

    if (tobj == NULL) {
        tobj = gluNewTess();
#if defined(G3D_OSX)
#       if (__GNUC__ < 4)
#           define CAST(x) reinterpret_cast<GLvoid (*)(...)>(x)
#       else
#           define CAST(x) reinterpret_cast<GLvoid (*)()>(x)
#       endif
#elif defined(G3D_LINUX)
        #define CAST(x) reinterpret_cast<void (*)()>(x)
#else
        #define CAST(x) reinterpret_cast<void (__stdcall *)(void)>(x) 
#endif
    gluTessCallback(tobj, GLU_TESS_BEGIN_DATA,    CAST(_tesselateBegin));
        gluTessCallback(tobj, GLU_TESS_VERTEX_DATA,   CAST(_tesselateVertex));
        gluTessCallback(tobj, GLU_TESS_END_DATA,      CAST(_tesselateEnd));
        gluTessCallback(tobj, GLU_TESS_COMBINE_DATA,  CAST(_tesselateCombine));
        gluTessCallback(tobj, GLU_TESS_ERROR,         CAST(_tesselateError));
        gluTessProperty(tobj, GLU_TESS_BOUNDARY_ONLY, GL_FALSE);
        gluTessProperty(tobj, GLU_TESS_WINDING_RULE,  GLU_TESS_WINDING_ODD);
        #undef CAST
    }

    double v[3];
    int i;
    TessData data;

    gluTessBeginPolygon(tobj, &data);
        gluTessBeginContour(tobj);
            for (i = 0; i < input.size(); ++i) {
                // Expand the input to double precision
                v[0] = input[i].x;
                v[1] = input[i].y;
                v[2] = input[i].z;
                gluTessVertex(tobj, v, const_cast<Vector3*>(&(input[i])));
            }
        gluTessEndContour(tobj);
    gluTessEndPolygon(tobj);


    for (int p = 0; p < data.primitive.size(); ++p) {
        const TessData::Primitive& primitive = data.primitive[p];

        // Turn the tesselated primitive into triangles
        switch (primitive.primitiveType) {
        case GL_TRIANGLES:
            // This is easy, just walk through them in order.
            for (i = 0; i < primitive.vertex.size(); i += 3) {
                output.append(Triangle(primitive.vertex[i], primitive.vertex[i + 1], primitive.vertex[i + 2]));
            }
            break;

        case GL_TRIANGLE_FAN:
            {
                // Make a triangle between every pair of vertices and the 1st vertex
                for (i = 1; i < primitive.vertex.size() - 1; ++i) {
                    output.append(Triangle(primitive.vertex[0], primitive.vertex[i], primitive.vertex[i + 1]));
                }
            }
            break;

        case GL_TRIANGLE_STRIP:
            for (i = 0; i < primitive.vertex.size() - 3; i += 2) {
                output.append(Triangle(primitive.vertex[i], primitive.vertex[i + 1], primitive.vertex[i + 2]));
                output.append(Triangle(primitive.vertex[i + 2], primitive.vertex[i + 1], primitive.vertex[i + 3]));
            }
            if (i < primitive.vertex.size() - 2) {
                output.append(Triangle(primitive.vertex[i], primitive.vertex[i + 1], primitive.vertex[i + 2]));
            }
            break;

        default:
            //debugAssertM(false, GLenumToString(primitive.primitiveType));
            // Ignore other primitives
            ;
        }
    }
}

}
