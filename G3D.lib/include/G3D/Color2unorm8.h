/** 
  \file G3D/Color2unorm8.h
 
  \maintainer Morgan McGuire
 
  \created 2011-06-23
  \edited  2011-01-23

  Copyright 2000-2015, Morgan McGuire.
  All rights reserved.
 */

#ifndef G3D_Color2unorm8_h
#define G3D_Color2unorm8_h

#include "G3D/platform.h"
#include "G3D/unorm8.h"

namespace G3D {

/** Matches OpenGL GL_RG8 format.  

Can be used to accessdata in GL_RA8 or GL_LA8 format as well.
*/
G3D_BEGIN_PACKED_CLASS(2)
Color2unorm8 {
private:
    // Hidden operators
    bool operator<(const Color2unorm8&) const;
    bool operator>(const Color2unorm8&) const;
    bool operator<=(const Color2unorm8&) const;
    bool operator>=(const Color2unorm8&) const;

public:

    unorm8       r;
    unorm8       g;

    Color2unorm8() : r(unorm8::fromBits(0)), g(unorm8::fromBits(0)) {}

    explicit Color2unorm8(const unorm8 _r, const unorm8 _g) : r(_r), g(_g) {}

    inline bool operator==(const Color2unorm8& other) const {
        return r == other.r && g == other.g;
    }

    inline bool operator!=(const Color2unorm8& other) const {
        return r != other.r || g != other.g;
    }

}
G3D_END_PACKED_CLASS(2)
}
#endif
