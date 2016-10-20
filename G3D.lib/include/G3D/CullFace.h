/** 
  \file G3D/CullFace.h
 
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
  \created 2012-07-26
  \edited  2012-07-26

  Copyright 2000-2015, Morgan McGuire.
  All rights reserved.
 */

#ifndef G3D_CullFace_h
#define G3D_CullFace_h

#include "G3D/platform.h"
#include "G3D/Any.h"
#include "G3D/HashTrait.h"

namespace G3D {

class CullFace {
public:
    /** Don't use this enum; use CullFace instances instead. */
    enum Value {
        NONE = 0, // GL_NONE,
        FRONT =  0x0404, // GL_FRONT,
        BACK = 0x0405, //GL_BACK,
        FRONT_AND_BACK = 0x0408, //GL_FRONT_AND_BACK,
        CURRENT
    };

    Value value;
private:

    void fromString(const String& x) {
        if (x == "NONE") {
            value = NONE;
        } else if (x == "FRONT") {
            value = FRONT;
        } else if (x == "BACK") {
            value = BACK;
        } else if (x == "FRONT_AND_BACK") {
            value = FRONT_AND_BACK;
        } else if (x == "CURRENT") {
            value = CURRENT;
        }
    }

public:

    explicit CullFace(const String& x) : value((Value)0) {
        fromString(x);
    }

    explicit CullFace(const G3D::Any& a) : value((Value)0) {
        fromString(a.string());
    }
    
    const char* toString() const {
        switch (value) {
        case NONE:
            return "NONE";
        case FRONT:
            return "FRONT";
        case BACK:
            return "BACK";
        case FRONT_AND_BACK:
            return "FRONT_AND_BACK";
        default:
            return "CURRENT";
        }
    }

    Any toAny() const {
        return G3D::Any(toString());
    }

    CullFace(char v) : value((Value)v) {}

    CullFace() : value((Value)0) {}

    CullFace(const Value v) : value(v) {}

    CullFace& operator=(const Any& a) {
        value = CullFace(a).value;
        return *this;
    }

    bool operator== (const CullFace other) const {\
        return value == other.value;
    }

    bool operator== (const CullFace::Value other) const {\
        return value == other;
    }

    bool operator!= (const CullFace other) const {\
        return value != other.value;
    }

    bool operator!= (const CullFace::Value other) const {\
        return value != other;
    }


};

} // namespace


template <> struct HashTrait<G3D::CullFace::Value> {
    static size_t hashCode(G3D::CullFace::Value key) { return static_cast<size_t>(key); }
};                                                                                          
                                                                                            
template <> struct HashTrait<G3D::CullFace> { 
    static size_t hashCode(G3D::CullFace key) { return static_cast<size_t>(key.value); }
};

#endif
