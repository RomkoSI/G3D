/** 
  \file G3D/Color3unorm8.h
 
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
  \created 2003-04-07
  \edited  2011-08-11

  Copyright 2000-2015, Morgan McGuire.
  All rights reserved.
 */

#ifndef G3D_Color3unorm8_h
#define G3D_Color3unorm8_h

#include "G3D/platform.h"
#include "G3D/unorm8.h"
#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

namespace G3D {

/**
 Represents a Color3 as a packed integer.  Convenient
 for creating unsigned int vertex arrays.  Used by
 G3D::GImage as the underlying format.

 <B>WARNING</B>: Integer color formats are different than
 integer vertex formats.  The color channels are automatically
 scaled by 255 (because OpenGL automatically scales integer
 colors back by this factor).  So Color3(1,1,1) == Color3unorm8(255,255,255)
 but Vector3(1,1,1) == Vector3int16(1,1,1).
 */

G3D_BEGIN_PACKED_CLASS(1)
Color3unorm8 {
private:
    // Hidden operators
    bool operator<(const Color3unorm8&) const;
    bool operator>(const Color3unorm8&) const;
    bool operator<=(const Color3unorm8&) const;
    bool operator>=(const Color3unorm8&) const;

public:
    unorm8       r;
    unorm8       g;
    unorm8       b;

    Color3unorm8() : r(unorm8::fromBits(0)), g(unorm8::fromBits(0)), b(unorm8::fromBits(0)) {}

    Color3unorm8(const unorm8 _r, const unorm8 _g, const unorm8 _b) : r(_r), g(_g), b(_b) {}

    static Color3unorm8 zero() {
        return Color3unorm8(unorm8::zero(), unorm8::zero(), unorm8::zero());
    }

    static Color3unorm8 one() {
        return Color3unorm8(unorm8::one(), unorm8::one(), unorm8::one());
    }

    explicit Color3unorm8(const class Color3& c);

    explicit Color3unorm8(class BinaryInput& bi);

    static Color3unorm8 fromARGB(uint32 i) {
        Color3unorm8 c;
        c.r = unorm8::fromBits((i >> 16) & 0xFF);
        c.g = unorm8::fromBits((i >> 8) & 0xFF);
        c.b = unorm8::fromBits(i & 0xFF);
        return c;
    }

    Color3unorm8 bgr() const {
        return Color3unorm8(b, g, r);
    }

    Color3unorm8 max(const Color3unorm8 x) const {
        return Color3unorm8(G3D::max(r, x.r), G3D::max(g, x.g), G3D::max(b, x.b));
    }

    Color3unorm8 min(const Color3unorm8 x) const {
        return Color3unorm8(G3D::min(r, x.r), G3D::min(g, x.g), G3D::min(b, x.b));
    }

    /**
     Returns the color packed into a uint32
     (the upper byte is 0xFF)
     */
    uint32 asUInt32() const {
        return (255U << 24) + (uint32(r.bits()) << 16) + (uint32(g.bits()) << 8) + uint32(b.bits());
    }

    void serialize(class BinaryOutput& bo) const;

    void deserialize(class BinaryInput& bi);

    // access vector V as V[0] = V.r, V[1] = V.g, V[2] = V.b
    //
    // WARNING.  These member functions rely on
    // (1) Color3 not having virtual functions
    // (2) the data packed in a 3*sizeof(unorm8) memory block
    unorm8& operator[] (int i) const {
        debugAssert((unsigned int)i < 3);
        return ((unorm8*)this)[i];
    }

    operator unorm8* () {
        return (G3D::unorm8*)this;
    }

    operator const unorm8* () const {
        return (unorm8*)this;
    }

    bool operator==(const Color3unorm8 other) const {
        return (other.r == r) && (other.g == g) && (other.b == b);
    }

    bool operator!=(const Color3unorm8 other) const {
        return ! (*this == other);
    }
}
G3D_END_PACKED_CLASS(1)

} // namespace G3D

#endif
