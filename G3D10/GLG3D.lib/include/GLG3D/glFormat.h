/**
 \file GLG3D/glFormat.h

 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2002-08-07
 \edited  2013-01-14

 Copyright 2002-2013, Morgan McGuire.
 All rights reserved.
*/
#ifndef glFormat_h
#define glFormat_h

#include "glheaders.h"
#include "G3D/g3dmath.h"
#include "G3D/unorm8.h"
#include "G3D/unorm16.h"
#include "G3D/snorm8.h"
#include "G3D/snorm16.h"
#include "G3D/Vector2.h"
#include "G3D/Vector2int16.h"
#include "G3D/Vector2int32.h"
#include "G3D/Vector3.h"
#include "G3D/Vector3int16.h"
#include "G3D/Vector3int32.h"
#include "G3D/Vector4int16.h"
#include "G3D/Vector4uint16.h"
#include "G3D/Vector4int32.h"
#include "G3D/Vector4.h"
#include "G3D/Vector2unorm16.h"

// This implementation is designed to meet the following constraints:
//   1. Work around the many MSVC++ partial template bugs
//   2. Work for primitive types (e.g. int)

/** \def glFormatOf

    \brief A macro that maps G3D types to OpenGL formats
    (e.g. <code>glFormat(Vector3) == GL_FLOAT</code>).

    Use <code>DECLARE_GLFORMATOF(MyType, GLType, bool)</code> at top-level to define
    glFormatOf values for your own classes.

    Used by the vertex array infrastructure. */
#define glFormatOf(T) (G3D::_internal::_GLFormat<T>::type())

/** \def glCanBeIndexType

    \brief True if the C-type passed as an argument can be used by an OpenGL index buffer as the data type.
    e.g., glCanBeIndexType(float) = false.
 */
#define glCanBeIndexType(T) (G3D::_internal::_GLFormat<T>::canBeIndex())

/** \def glIsNormalizedFixedPoint

    \brief True if the C-type passed as an argument is stored in OpenGL normalized fixed point format.
    e.g., glIsNormalizedFixedPoint(Vector2unorm16) = true.
 */
#define glIsNormalizedFixedPoint(T) (G3D::_internal::_GLFormat<T>::isNormalizedFixedPoint())

namespace G3D {
namespace _internal {


template<class T> class _GLFormat {
public:
    static GLenum type() {
        return GL_NONE;
    }

    static bool canBeIndex() {            
        return false;
    }                                     
    static bool isNormalizedFixedPoint() {
        return false;
    }
};

} // namespace _internal
} // namespace G3D

/**
   \def DECLARE_GLFORMATOF

 \brief Macro to declare the underlying format (as will be returned by glFormatOf)
 of a type.  

 For example,

 \code
    DECLARE_GLFORMATOF(Vector4, GL_FLOAT, false, false)
  \endcode

  Use this so you can make vertex arrays of your own classes and not just 
  the standard ones.

  \param _isIndex True for types that can be used as indices.
 */
#define DECLARE_GLFORMATOF(G3DType, GLType, _isIndex, _isNormalizedFixedPoint)  \
namespace G3D {                                      \
    namespace _internal {                            \
        template<> class _GLFormat<G3DType> {        \
        public:                                      \
            static GLenum type()  {                  \
                return GLType;                       \
            }                                        \
            static bool canBeIndex() {               \
                return _isIndex;                     \
            }                                        \
            static bool isNormalizedFixedPoint() {   \
                return _isNormalizedFixedPoint;       \
            }                                        \
        };                                           \
    }                                                \
}

DECLARE_GLFORMATOF( Vector2,       GL_FLOAT,          false, false)
DECLARE_GLFORMATOF( Vector2int16,  GL_SHORT,          false, false)
DECLARE_GLFORMATOF( Vector2int32,  GL_INT,            false, false)
DECLARE_GLFORMATOF( Vector2unorm16,GL_UNSIGNED_SHORT, false, true)

DECLARE_GLFORMATOF( Vector3,       GL_FLOAT,          false, false)
DECLARE_GLFORMATOF( Vector3int16,  GL_SHORT,          false, false)
DECLARE_GLFORMATOF( Vector3int32,  GL_INT,            false, false)

DECLARE_GLFORMATOF( Vector4,       GL_FLOAT,          false, false)
DECLARE_GLFORMATOF( Vector4int16,  GL_SHORT,          false, false)
DECLARE_GLFORMATOF( Vector4uint16, GL_UNSIGNED_SHORT, false, false)
DECLARE_GLFORMATOF( Vector4int8,   GL_BYTE,           false, false)
DECLARE_GLFORMATOF( Vector4int32,  GL_INT,            false, false)

DECLARE_GLFORMATOF( Color3unorm8,  GL_UNSIGNED_BYTE,  false, true)
DECLARE_GLFORMATOF( Color3,        GL_FLOAT,          false, false)
DECLARE_GLFORMATOF( Color4,        GL_FLOAT,          false, false)
DECLARE_GLFORMATOF( Color4unorm8,  GL_UNSIGNED_BYTE,  false, false)

DECLARE_GLFORMATOF( snorm8,        GL_BYTE,           false, true)
DECLARE_GLFORMATOF( snorm16,       GL_SHORT,          false, true)

DECLARE_GLFORMATOF( unorm8,        GL_UNSIGNED_BYTE,  false, true)
DECLARE_GLFORMATOF( unorm16,       GL_UNSIGNED_SHORT, false, true)

DECLARE_GLFORMATOF( uint8,         GL_UNSIGNED_BYTE,  true,  false)
DECLARE_GLFORMATOF( uint16,        GL_UNSIGNED_SHORT, true,  false)
DECLARE_GLFORMATOF( uint32,        GL_UNSIGNED_INT,   true,  false)

DECLARE_GLFORMATOF( int8,          GL_BYTE,           true,  false)
DECLARE_GLFORMATOF( int16,         GL_SHORT,          true,  false)
DECLARE_GLFORMATOF( int32,         GL_INT,            true,  false)

DECLARE_GLFORMATOF( float,         GL_FLOAT,          false, false)
DECLARE_GLFORMATOF( double,        GL_DOUBLE,         false, false)

#endif
