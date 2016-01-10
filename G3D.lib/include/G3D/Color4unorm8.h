/** 
  \file G3D/Color4unorm8.h
 
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
  \created 2003-04-07
  \edited  2011-08-24

  Copyright 2000-2015, Morgan McGuire.
  All rights reserved.
 */

#ifndef G3D_Color4unorm8_h
#define G3D_Color4unorm8_h

#include "G3D/g3dmath.h"
#include "G3D/platform.h"
#include "G3D/Color3unorm8.h"

namespace G3D {

/**
 Represents a Color4 as a packed integer.  Convenient
 for creating unsigned int vertex arrays.  Used by
 G3D::GImage as the underlying format.
 */
G3D_BEGIN_PACKED_CLASS(1)
Color4unorm8 {
private:
    // Hidden operators
    bool operator<(const Color4unorm8&) const;
    bool operator>(const Color4unorm8&) const;
    bool operator<=(const Color4unorm8&) const;
    bool operator>=(const Color4unorm8&) const;

public:
    unorm8       r;
    unorm8       g;
    unorm8       b;
    unorm8       a;

    Color4unorm8() : r(unorm8::zero()), g(unorm8::zero()), b(unorm8::zero()), a(unorm8::zero()) {}
    Color4unorm8(const unorm8 _r, const unorm8 _g, const unorm8 _b, const unorm8 _a) : r(_r), g(_g), b(_b), a(_a) {}

    inline static Color4unorm8 one() {
        return Color4unorm8(unorm8::one(), unorm8::one(), unorm8::one(), unorm8::one());
    }

    /** (0,0,0,0) */
    inline static Color4unorm8 zero() {
        return Color4unorm8(unorm8::zero(), unorm8::zero(), unorm8::zero(), unorm8::zero());
    }

    explicit Color4unorm8(const class Color4& c);

    Color4unorm8 max(const Color4unorm8 x) const {
        return Color4unorm8(G3D::max(r, x.r), G3D::max(g, x.g), G3D::max(b, x.b), G3D::max(a, x.a));
    }

    Color4unorm8 min(const Color4unorm8 x) const {
        return Color4unorm8(G3D::min(r, x.r), G3D::min(g, x.g), G3D::min(b, x.b), G3D::min(a, x.a));
    }

    Color4unorm8(const Color3unorm8& c, const unorm8 _a) : r(c.r), g(c.g), b(c.b), a(_a) {}

    Color4unorm8(class BinaryInput& bi);

    inline static Color4unorm8 fromARGB(uint32 i) {
        Color4unorm8 c(unorm8::fromBits((i >> 24) & 0xFF),
                       unorm8::fromBits((i >> 16) & 0xFF),
                       unorm8::fromBits((i >> 8) & 0xFF),
                       unorm8::fromBits(i & 0xFF));
        return c;
    }

    inline uint32 asUInt32() const {
        return ((uint32)a.bits() << 24) + ((uint32)r.bits() << 16) + ((uint32)g.bits() << 8) + b.bits();
    }

    // access vector V as V[0] = V.r, V[1] = V.g, V[2] = V.b
    //
    // WARNING.  These member functions rely on
    // (1) Color4unorm8 not having virtual functions
    // (2) the data packed in a 3*sizeof(unorm8) memory block
    unorm8& operator[] (int i) const {
        return ((unorm8*)this)[i];
    }

    operator unorm8* () {
        return (unorm8*)this;
    }

    operator const unorm8* () const {
        return (unorm8*)this;
    }


    inline Color3unorm8 bgr() const {
        return Color3unorm8(b, g, r);
    }

    void serialize(class BinaryOutput& bo) const;

    void deserialize(class BinaryInput& bi);

    inline Color3unorm8 rgb() const {
        return Color3unorm8(r, g, b);
    }

    bool operator==(const Color4unorm8& other) const {
        return *reinterpret_cast<const uint32*>(this) == *reinterpret_cast<const uint32*>(&other);
    }

    bool operator!=(const Color4unorm8& other) const {
        return *reinterpret_cast<const uint32*>(this) != *reinterpret_cast<const uint32*>(&other);
    }

}
G3D_END_PACKED_CLASS(1)

} // namespace G3D

#endif
