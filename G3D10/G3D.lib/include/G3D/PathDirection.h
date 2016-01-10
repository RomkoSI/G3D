/**
  \file G3D/PathDirection.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
  \created 2012-07-20
  \edited  2012-07-20
*/
#ifndef G3D_PathDirection_h
#define G3D_PathDirection_h

#include "G3D/platform.h"
#include "G3D/enumclass.h"

namespace G3D {
/** 
    \brief Direction of light transport along a ray or path.
 */
class PathDirection {
public:
    enum Value {
        /** A path originating at a light source that travels in the
            direction of real photon propagation. Used in bidirectional
            path tracing and photon mapping.*/
        SOURCE_TO_EYE, 

        /** A path originating in the scene (often, at the "eye" or
            camera) and propagating backwards towards a light
            source. Used in path tracing and Whitted ray tracing.*/
        EYE_TO_SOURCE
    } value;


    static const char* toString(int i, Value& v) {
        static const char* str[] = {"SOURCE_TO_EYE", "EYE_TO_SOURCE", NULL};
        static const Value val[] = {SOURCE_TO_EYE, EYE_TO_SOURCE};
        const char* s = str[i];
        if (s) {
            v = val[i];
        }
        return s;
    }
    G3D_DECLARE_ENUM_CLASS_METHODS(PathDirection);
};


} // namespace G3D

G3D_DECLARE_ENUM_CLASS_HASHCODE(G3D::PathDirection);

#endif
