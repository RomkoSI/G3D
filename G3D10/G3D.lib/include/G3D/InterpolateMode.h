/** 
  \file G3D/InterpolateMode.h
 
  \maintainer Michael Mara, http://www.illuminationcodified.com
 
  \created 2013-10-10
  \edited  2014-03-26

  G3D Innovation Engine
  Copyright 2000-2015, Morgan McGuire.
  All rights reserved.
 */

#ifndef G3D_InterpolateMode_h
#define G3D_InterpolateMode_h

#include "G3D/platform.h"
#include "G3D/enumclass.h"

namespace G3D {

/** 
    Trilinear mipmap is the best quality (and frequently fastest)
    mode.  The no-mipmap modes conserve memory.  Non-interpolating
    ("Nearest") modes are generally useful only when packing lookup
    tables into textures for shaders.
 */ 
class InterpolateMode {
public:
    /** Don't use this enum; use InterpolateMode instances instead. */
    enum Value {
        /** GL_LINEAR_MIPMAP_LINEAR */
        TRILINEAR_MIPMAP, 

        /** GL_LINEAR_MIPMAP_NEAREST */
        BILINEAR_MIPMAP,

        /** GL_NEAREST_MIPMAP_NEAREST */
        NEAREST_MIPMAP,

        /** GL_LINEAR */
        BILINEAR_NO_MIPMAP,

        /** GL_NEAREST */
        NEAREST_NO_MIPMAP,

        /** Choose the nearest MIP level and perform linear interpolation within it.*/
        LINEAR_MIPMAP_NEAREST,

        /** Linearly blend between nearest pixels in the two closest MIP levels.*/
        NEAREST_MIPMAP_LINEAR,
    } value;
    
    static const char* toString(int i, Value& v) {
        static const char* str[] = {"TRILINEAR_MIPMAP", "BILINEAR_MIPMAP", "NEAREST_MIPMAP", "BILINEAR_NO_MIPMAP", "NEAREST_NO_MIPMAP", "LINEAR_MIPMAP_NEAREST", "NEAREST_MIPMAP_LINEAR", NULL}; 
        static const Value val[] = {TRILINEAR_MIPMAP, BILINEAR_MIPMAP, NEAREST_MIPMAP, BILINEAR_NO_MIPMAP, NEAREST_NO_MIPMAP, LINEAR_MIPMAP_NEAREST, NEAREST_MIPMAP_LINEAR};
        const char* s = str[i];
        if (s) {
            v = val[i];
        }
        return s;
    }

    bool requiresMipMaps() const {
        return (value == TRILINEAR_MIPMAP) || 
            (value == BILINEAR_MIPMAP) || 
            (value == NEAREST_MIPMAP) ||
            (value == LINEAR_MIPMAP_NEAREST) ||
            (value == NEAREST_MIPMAP_LINEAR);
    }

    G3D_DECLARE_ENUM_CLASS_METHODS(InterpolateMode);
};


} // namespace G3D

G3D_DECLARE_ENUM_CLASS_HASHCODE(G3D::InterpolateMode);

#endif
