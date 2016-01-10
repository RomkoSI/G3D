/**
  \file getOpenGLState.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
  \cite       Created by Morgan McGuire & Seth Block
  \created 2001-08-05
  \edited  2013-12-31
*/

#include "GLG3D/glcalls.h"
#include "GLG3D/getOpenGLState.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/RenderDevice.h"

namespace G3D {

int sizeOfGLFormat(GLenum format) {
    switch (format) {
    case GL_2_BYTES:
        return 2;

    case GL_3_BYTES:
        return 3;

    case GL_4_BYTES:
        return 4;

    case GL_DOUBLE:
        return sizeof(double);

    case GL_FLOAT:
        return sizeof(float);

    case GL_FLOAT_VEC2:
        return sizeof(float) * 2;

    case GL_FLOAT_VEC3:
        return sizeof(float) * 3;

    case GL_FLOAT_VEC4:
        return sizeof(float) * 4;

    case GL_FLOAT_MAT2:
        return sizeof(float) * 4;

    case GL_FLOAT_MAT3:
        return sizeof(float) * 9;

    case GL_FLOAT_MAT4:
        return sizeof(float) * 16;

    case GL_UNSIGNED_SHORT:
        return sizeof(unsigned short);
        
    case GL_SHORT:
        return sizeof(short);
        
    case GL_UNSIGNED_INT:
        return sizeof(unsigned int);
        
    case GL_INT:
        return sizeof(int);

    case GL_INT_VEC2_ARB:
        return sizeof(int) * 2;

    case GL_INT_VEC3_ARB:
        return sizeof(int) * 3;

    case GL_INT_VEC4_ARB:
        return sizeof(int) * 4;
        
    case GL_UNSIGNED_BYTE:
        return 1;

    case GL_BYTE:
        return 1;

    default:
        return 0;
    }
}


GLint glGetTexParameteri(GLenum target, GLenum pname) {
    GLint result;
    glGetTexParameteriv(target, pname, &result);
    return result;
}


GLint glGetInteger(GLenum which) {
    // We allocate an array in case the caller accidentally
    // invoked on a value that will return more than just
    // one integer.
    GLint result[32];
    glGetIntegerv(which, result);
    return result[0];
}


GLfloat glGetFloat(GLenum which) {
    // We allocate an array in case the caller accidentally
    // invoked on a value that will return more than just
    // one float.
    GLfloat result[32];
    glGetFloatv(which, result);
    return result[0];
}


GLboolean glGetBoolean(GLenum which) {
    // We allocate an array in case the caller accidentally
    // invoked on a value that will return more than just
    // one bool.
    GLboolean result[32];
    glGetBooleanv(which, result);
    return result[0];
}


GLdouble glGetDouble(GLenum which) {
    // We allocate an array in case the caller accidentally
    // invoked on a value that will return more than just
    // one double.
    GLdouble result[32];
    glGetDoublev(which, result);
    return result[0];
}

static String getGenericAttributeState(GLuint index) {
    GLuint enabledUI;
    glGetVertexAttribIuiv(index, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabledUI);
    bool enabled = enabledUI != 0;
    String result = enabled ? "glEnableVertexAttributeArray" : "glDisableVertexAttributeArray";
    result += format("(%d);\n", index);

    if (enabled) {
	GLint size;
	glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_SIZE, &size);
	GLenum type;
	glGetVertexAttribIuiv(index, GL_VERTEX_ATTRIB_ARRAY_TYPE, &type);
	GLint normalized;
	glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &normalized);
	GLint stride;
	glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &stride);
	GLvoid* ptr;
        glGetVertexAttribPointerv(index, GL_VERTEX_ATTRIB_ARRAY_POINTER, &ptr);	
	result += format("glVertexAttribPointer(%d, %d, %s, %s, %d, %p);\n",
			 index,
			 size,
			 GLenumToString(type),
			 normalized != 0 ? "true" : "false",
			 stride,
			 ptr);
    }
    return result;
}

static String getClippingState() {
    String result;

    int numPlanes = glGetInteger(GL_MAX_CLIP_PLANES);
    int C;

    for(C = 0; C < numPlanes; ++C) {
        result += format("// Clip plane %d\n", C);
        
        result += format("%s(GL_CLIP_PLANE0 + %d);\n", glIsEnabled(GL_CLIP_PLANE0 + C) ? "glEnable" : "glDisable", C);

        double x[4];
        glGetClipPlane(GL_CLIP_PLANE0 + C, x);
        result += format("{double coefficients[]={%4.4e, %4.4e, %4.4e, %4.4e};\n glClipPlane(GL_CLIP_PLANE0 + %d, coefficients);}\n",
            x[0], x[1], x[2], x[3], C);
    }

    return result;
}

static String getLightingState(bool showDisabled) {
    String result;
    int L;

    if (glIsEnabled(GL_LIGHTING)) {
        result += "glEnable(GL_LIGHTING);\n";
    } else {
        result += "glDisable(GL_LIGHTING);\n";
        if (! showDisabled) {
            return result;
        }
    }

    result += "\n";


    for (L = 0; L < 8; ++L) {
        result += format("// Light %d\n", L);
        if (glIsEnabled(GL_LIGHT0 + L)) {
            result += format("glEnable(GL_LIGHT0 + %d);\n", L);
        } else {
            result += format("glDisable(GL_LIGHT0 + %d);\n", L);
        }

        if (showDisabled || glIsEnabled(GL_LIGHT0 + L)) {
            float x[4];
            glGetLightfv(GL_LIGHT0 + L, GL_POSITION, x);
            result += format("{float pos[]={%4.4ff, %4.4ff, %4.4ff, %4.4ff};\nglLightfv(GL_LIGHT0 + %d, GL_POSITION, pos);}\n",
                  x[0], x[1], x[2], x[3], L);

            glGetLightfv(GL_LIGHT0 + L, GL_AMBIENT, x);
            result += format("{float col[]={%4.4ff, %4.4ff, %4.4ff, %4.4ff};\nglLightfv(GL_LIGHT0 + %d, GL_AMBIENT, col);}\n",
                  x[0], x[1], x[2], x[3], L);

            glGetLightfv(GL_LIGHT0 + L, GL_DIFFUSE, x);
            result += format("{float col[]={%4.4ff, %4.4ff, %4.4ff, %4.4ff};\nglLightfv(GL_LIGHT0 + %d, GL_DIFFUSE, col);}\n",
                  x[0], x[1], x[2], x[3], L);

            glGetLightfv(GL_LIGHT0 + L, GL_SPECULAR, x);
            result += format("{float col[]={%4.4ff, %4.4ff, %4.4ff, %4.4ff};\nglLightfv(GL_LIGHT0 + %d, GL_SPECULAR, col);}\n",
                  x[0], x[1], x[2], x[3], L);

            glGetLightfv(GL_LIGHT0 + L, GL_CONSTANT_ATTENUATION, x);
            result += format("glLightf (GL_LIGHT0 + %d, GL_CONSTANT_ATTENUATION,  %ff);\n",
                  L, x[0]);

            glGetLightfv(GL_LIGHT0 + L, GL_LINEAR_ATTENUATION, x);
            result += format("glLightf (GL_LIGHT0 + %d, GL_LINEAR_ATTENUATION,    %ff);\n",
                  L, x[0]);

            glGetLightfv(GL_LIGHT0 + L, GL_QUADRATIC_ATTENUATION, x);
            result += format("glLightf (GL_LIGHT0 + %d, GL_QUADRATIC_ATTENUATION, %ff);\n",
                  L, x[0]);
        }

        result += "\n";
    }

    // Ambient
    result += "// Ambient\n";
    float x[4];
    glGetFloatv(GL_LIGHT_MODEL_AMBIENT, x);
    result += format("{float col[] = {%ff, %ff, %ff, %ff};\n glLightModelfv(GL_LIGHT_MODEL_AMBIENT, col);}\n",
        x[0], x[1], x[2], x[3]);

    result += "\n";

    return result;
}


static String getOneMatrixState(GLenum getWhich, GLenum which) {

    String matrixName = GLenumToString(which);

    GLdouble m[16];
    glGetDoublev(getWhich, m);

    String result = String("{glMatrixMode(") + matrixName + ");\n";

    result +=
        format(" GLdouble m[16] = {%3.3f, %3.3f, %3.3f, %3.3f,\n"
               "                   %3.3f, %3.3f, %3.3f, %3.3f,\n"  
               "                   %3.3f, %3.3f, %3.3f, %3.3f,\n" 
               "                   %3.3f, %3.3f, %3.3f, %3.3f};\n",
               m[0], m[1], m[2], m[3],
               m[4], m[5], m[6], m[7],
               m[8], m[9], m[10], m[11],
               m[12], m[13], m[14], m[15]);
    result += " glLoadMatrixd(m);}\n\n";

    return result;
}


static String getMatrixState() {
    String result;

    result += getOneMatrixState(GL_MODELVIEW_MATRIX, GL_MODELVIEW);
    result += getOneMatrixState(GL_PROJECTION_MATRIX, GL_PROJECTION);

    // Get the matrix mode
    result += String("glMatrixMode(") + GLenumToString(glGetInteger(GL_MATRIX_MODE)) + ");\n\n";

    return result;
}

static String getTextureState(bool showDisabled) {

    String result;
    GLenum pname[]  = {GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R};
    GLenum tname[]  = {GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D};
    GLenum tname2[] = {GL_TEXTURE_BINDING_1D, GL_TEXTURE_BINDING_2D, GL_TEXTURE_BINDING_3D};

    // Let's check the multitexture availability
    if (!GLCaps::supports_GL_ARB_multitexture()) {

        result += format("//NO MULTITEXTURE\n");

        // See if the texture unit is enabled
        bool enabled = false;
        for (int tt = 0; tt < 3; ++tt) 
            enabled = enabled || (glGetBoolean(tname[tt]) ? true : false);

        if (!enabled && !showDisabled) {
            for (int tt = 0; tt < 3; ++tt) 
                result += format("glDisable(%s); ", GLenumToString(tname[tt]));
            result += "\n";
        } else {
            for (int tt = 0; tt < 3; ++tt) {
        
                bool on = glGetBoolean(tname[tt]) ? true : false;
                result += format("%s(%s);\n",
                    on ? "glEnable" : "glDisable",
                    GLenumToString(tname[tt]));
                        
                if (showDisabled || on) {
                    result += format("glBindTexture(%s, %d);\n", 
                        GLenumToString(tname[tt]),
                        glGetInteger(tname2[tt]));

                    for (int p = 0; p < 3; ++p) {
                        result += format("glTexParameteri(%s, %s, %s);\n",
                            GLenumToString(tname[tt]),
                            GLenumToString(pname[p]),
                            GLenumToString(glGetTexParameteri(tname[tt], pname[p])));
                    }

                    result += "\n";
                }
            }

            // TODO: texture environment

            GLdouble v[4];
            glGetDoublev(GL_CURRENT_TEXTURE_COORDS, v);
            result += format("glTexCoord4dARB(%g, %g, %g, %g);\n", v[0], v[1], v[2], v[3]);

            result += getOneMatrixState(GL_TEXTURE_MATRIX, GL_TEXTURE);
            result += "\n";
        }
        
        return result;
    }

    //=============
    //multitexture
    //=============
    int numUnits = glGetInteger(GL_MAX_TEXTURE_UNITS_ARB);
    int active = glGetInteger(GL_ACTIVE_TEXTURE_ARB);

    // Iterate over all of the texture units
    for (int t = 0; t < numUnits; ++t) {
    
        result += format("// Texture Unit %d\n", t);
        result += format("glActiveTextureARB(GL_TEXTURE0_ARB + %d);\n", t);
        glActiveTextureARB(GL_TEXTURE0_ARB + t);

        // See if this unit is on
        bool enabled = false;
        for (int tt = 0; tt < 3; ++tt) {
            enabled = enabled || (glGetBoolean(tname[tt]) ? true : false);
        }

        if (! enabled && ! showDisabled) {
            for (int tt = 0; tt < 3; ++tt) {
                result += format("glDisable(%s); ",
                    GLenumToString(tname[tt]));
            }

            result += "\n";
        } else {
            for (int tt = 0; tt < 3; ++tt) {
        
                bool on = glGetBoolean(tname[tt]) ? true : false;
                result += format("%s(%s);\n",
                    on ? "glEnable" : "glDisable",
                    GLenumToString(tname[tt]));
                        
                if (showDisabled || on) {
                    result += format("glBindTexture(%s, %d);\n", 
                        GLenumToString(tname[tt]),
                        glGetInteger(tname2[tt]));

                    for (int p = 0; p < 3; ++p) {
                        result += format("glTexParameteri(%s, %s, %s);\n",
                            GLenumToString(tname[tt]),
                            GLenumToString(pname[p]),
                            GLenumToString(glGetTexParameteri(tname[tt], pname[p])));
                    }

                    result += "\n";
                }
            }

            // TODO: texture environment

            GLdouble v[4];
            glGetDoublev(GL_CURRENT_TEXTURE_COORDS, v);
            result += format("glMultiTexCoord4dARB(GL_TEXTURE0_ARB + %d, %g, %g, %g, %g);\n", t, v[0], v[1], v[2], v[3]);

            result += getOneMatrixState(GL_TEXTURE_MATRIX, GL_TEXTURE);
            result += "\n";
        }
    }

    // Restore the active texture unit
    glActiveTexture(active);
    result += format("glActiveTextureARB(GL_TEXTURE0 + %d);\n\n", active - GL_TEXTURE0);

    return result;
}

static String enableEntry(GLenum which) {
    return format("%s(%s);\n",
        glGetBoolean(which) ? "glEnable" : "glDisable", GLenumToString(which));
}

// TODO: automatically generate bufferEnum from bindingEnum... right now redundant state
static String getBufferBinding(GLenum bindingEnum, GLenum bufferEnum) {
    int buffer;
    glGetIntegerv(bindingEnum, &buffer);
    String result = format("glBindBuffer(%s,%d);\n", GLenumToString(bufferEnum), buffer);
    if (buffer != 0) {
        GLint size;
        GLint usage;
        glGetBufferParameteriv(bufferEnum, GL_BUFFER_SIZE, &size);
        glGetBufferParameteriv(bufferEnum, GL_BUFFER_USAGE, &usage);
        result += format("glBufferData(%s, %d, NULL, %s)\n", GLenumToString(bufferEnum), size, GLenumToString(usage));
    } 
    return result;
}

String getOpenGLState(bool showDisabled) {
    {
        debugAssertGLOk();
        glGetInteger(GL_BLEND);
        debugAssertM(glGetError() != GL_INVALID_OPERATION, 
             "Can't call getOpenGLState between glBegin() and glEnd()");
    }

    // The implementation has to be careful not to disrupt any OpenGL state and
    // to produce output code that sets values that interact (e.g. lighting and modelview matrix) in an order
    // so they produce the same results as currently in memory.

    String result;

    result += "///////////////////////////////////////////////////////////////////\n";
    result += "//                         Matrices                              //\n\n";
    result += getMatrixState();


    result += "///////////////////////////////////////////////////////////////////\n";
    result += "//                         Lighting                              //\n\n";
    result += getLightingState(showDisabled);

    result += "///////////////////////////////////////////////////////////////////\n";
    result += "//                         Clipping                              //\n\n";
    result += getClippingState();

    result += "///////////////////////////////////////////////////////////////////\n";
    result += "//                         Textures                              //\n\n";

    result += getTextureState(showDisabled);

    result += "///////////////////////////////////////////////////////////////////\n";
    result += "//                          Other                                //\n\n";
    
    GLdouble d[4];
    GLboolean b[4];

    // Viewport
    glGetDoublev(GL_VIEWPORT, d);
    result += format("glViewport(%g, %g, %g, %g);\n\n", d[0], d[1], d[2], d[3]);

    //color
    result += enableEntry(GL_COLOR_ARRAY);
    result += enableEntry(GL_COLOR_LOGIC_OP);
    result += enableEntry(GL_COLOR_MATERIAL);

    glGetDoublev(GL_COLOR_CLEAR_VALUE, d);
    result += format("glClearColor(%g, %g, %g, %g);\n", d[0], d[1], d[2], d[3]);
    glGetDoublev(GL_CURRENT_COLOR, d);
    result += format("glColor4d(%g, %g, %g, %g);\n", d[0], d[1], d[2], d[3]);
    glGetBooleanv(GL_COLOR_WRITEMASK, b);
    result += format("glColorMask(%d, %d, %d, %d);\n", b[0], b[1], b[2], b[3]);

    result += format("\n");


    //blend
    result += enableEntry(GL_BLEND);

    if (showDisabled || glGetBoolean(GL_BLEND)) {
        result += format("glBlendFunc(%s, %s);\n", 
            GLenumToString(glGetInteger(GL_BLEND_DST)),
            GLenumToString(glGetInteger(GL_BLEND_SRC)));
        result += format("\n");
    }



    //alpha
    result += enableEntry(GL_ALPHA_TEST);

    if (showDisabled || glGetBoolean(GL_ALPHA_TEST)) {
        result += format("glAlphaFunc(%s, %g);\n", 
            GLenumToString(glGetInteger(GL_ALPHA_TEST_FUNC)),
            glGetDouble(GL_ALPHA_TEST_REF));
        result += format("\n");
    }


    //depth stuff
    result += "///////////////////////////////////////////////////////////////////\n";
    result += "//                      Depth Buffer                             //\n\n";
    result += enableEntry(GL_DEPTH_TEST);
    if (showDisabled || glGetBoolean(GL_DEPTH_TEST)) {
        result += format("glDepthFunc(%s);\n", 
            GLenumToString(glGetInteger(GL_DEPTH_FUNC)));
    }

    result += format("glClearDepth(%g);\n", glGetDouble(GL_DEPTH_CLEAR_VALUE));
    result += format("glDepthMask(%d);\n", glGetBoolean(GL_DEPTH_WRITEMASK));

    {
        Vector2 range = glGetVector2(GL_DEPTH_RANGE);
        result += format("glDepthRange(%g, %g);\n", range.x, range.y);
    }

    result += format("\n");


    //stencil stuff
    result += "///////////////////////////////////////////////////////////////////////\n";
    result += "// Stencil\n\n";

    result += enableEntry(GL_STENCIL_TEST);

    result += format("glClearStencil(0x%x);\n", glGetInteger(GL_STENCIL_CLEAR_VALUE));

    if (showDisabled || glGetBoolean(GL_STENCIL_TEST)) 
        result += format(
            "glStencilFunc(%s, %d, %d);\n",
            GLenumToString(glGetInteger(GL_STENCIL_FUNC)),
            glGetInteger(GL_STENCIL_REF),
            glGetInteger(GL_STENCIL_VALUE_MASK));

    result += format(
        "glStencilOp(%s, %s, %s);\n",
        GLenumToString(glGetInteger(GL_STENCIL_FAIL)),
        GLenumToString(glGetInteger(GL_STENCIL_PASS_DEPTH_FAIL)),
        GLenumToString(glGetInteger(GL_STENCIL_PASS_DEPTH_PASS)));

    result += format("glStencilMask(0x%x);\n", glGetInteger(GL_STENCIL_WRITEMASK));
    
    result += ("\n");

    //misc
    result += enableEntry(GL_NORMALIZE);

    glGetDoublev(GL_CURRENT_NORMAL, d);
    result += format("glNormal3d(%g, %g, %g);\n", d[0], d[1], d[2]);

    result += ("\n");

    result += format("glPixelZoom(%g, %g);\n", glGetDouble(GL_ZOOM_X), 
        glGetDouble(GL_ZOOM_Y));

    result += format("glReadBuffer(%s);\n", 
        GLenumToString(glGetInteger(GL_READ_BUFFER)));

    result += enableEntry(GL_POLYGON_SMOOTH);
    result += enableEntry(GL_POLYGON_STIPPLE);
    result += enableEntry(GL_LINE_SMOOTH);
    result += enableEntry(GL_LINE_STIPPLE);
    result += enableEntry(GL_POINT_SMOOTH);

    result += enableEntry(GL_AUTO_NORMAL);
    result += enableEntry(GL_CULL_FACE);

    result += enableEntry(GL_POLYGON_OFFSET_FILL);
    result += enableEntry(GL_POLYGON_OFFSET_LINE);
    result += enableEntry(GL_POLYGON_OFFSET_POINT);

    result += ("\n");

    result += enableEntry(GL_DITHER);
    result += enableEntry(GL_FOG);

    result += enableEntry(GL_COLOR_ARRAY);
    result += enableEntry(GL_TEXTURE_COORD_ARRAY);
    result += enableEntry(GL_NORMAL_ARRAY);
    result += enableEntry(GL_VERTEX_ARRAY);
    result += enableEntry(GL_INDEX_ARRAY);
    result += enableEntry(GL_INDEX_LOGIC_OP);

    result += getBufferBinding(GL_ARRAY_BUFFER_BINDING, GL_ARRAY_BUFFER);
    result += getBufferBinding(GL_ELEMENT_ARRAY_BUFFER_BINDING, GL_ELEMENT_ARRAY_BUFFER);
    result += getBufferBinding(GL_PIXEL_PACK_BUFFER_BINDING, GL_PIXEL_PACK_BUFFER);
    result += getBufferBinding(GL_PIXEL_UNPACK_BUFFER_BINDING, GL_PIXEL_UNPACK_BUFFER);

    GLint maxVertexAttributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttributes);
    for (GLuint i = 0; i < (GLuint)maxVertexAttributes; ++i) {
        result += getGenericAttributeState(i);
    }
    result += format("\n");

    

    result += format("\n");

    result += enableEntry(GL_MAP1_COLOR_4);
    result += enableEntry(GL_MAP1_INDEX);
    result += enableEntry(GL_MAP1_NORMAL);
    result += enableEntry(GL_MAP1_TEXTURE_COORD_1);
    result += enableEntry(GL_MAP1_TEXTURE_COORD_2);
    result += enableEntry(GL_MAP1_TEXTURE_COORD_3);
    result += enableEntry(GL_MAP1_TEXTURE_COORD_4);
    result += enableEntry(GL_MAP1_VERTEX_3);
    result += enableEntry(GL_MAP1_VERTEX_4);
    result += enableEntry(GL_MAP2_COLOR_4);
    result += enableEntry(GL_MAP2_INDEX);
    result += enableEntry(GL_MAP2_NORMAL);
    result += enableEntry(GL_MAP2_TEXTURE_COORD_1);
    result += enableEntry(GL_MAP2_TEXTURE_COORD_2);
    result += enableEntry(GL_MAP2_TEXTURE_COORD_3);
    result += enableEntry(GL_MAP2_TEXTURE_COORD_4);
    result += enableEntry(GL_MAP2_VERTEX_3);
    result += enableEntry(GL_MAP2_VERTEX_4);

    result += format("\n");

    result += enableEntry(GL_SCISSOR_TEST);

    return result;
}

} // namespace

