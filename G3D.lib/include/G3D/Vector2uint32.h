/**
  \file G3D/Vector2uint32.h
  
 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  @created 2003-08-09
  @edited  2015-07-10

     G3D Innovation Engine
     Copyright 2002-2016, Morgan McGuire.
     All rights reserved.

 */

#ifndef G3D_Vector2uint32_h
#define G3D_Vector2uint32_h

#include "G3D/platform.h"
#include "G3D/G3DString.h"
#include "G3D/g3dmath.h"
#include "G3D/format.h"
#include "G3D/HashTrait.h"

namespace G3D {

/**
 \class Vector2uint32 
 A Vector2 that packs its fields into int32s.
 */
G3D_BEGIN_PACKED_CLASS(2)
Vector2uint32 {
private:
    // Hidden operators
    bool operator<(const Vector2uint32&) const;
    bool operator>(const Vector2uint32&) const;
    bool operator<=(const Vector2uint32&) const;
    bool operator>=(const Vector2uint32&) const;

public:
    G3D::uint32              x;
    G3D::uint32              y;

    Vector2uint32() : x(0), y(0) {}
    Vector2uint32(G3D::uint32 _x, G3D::uint32 _y) : x(_x), y(_y){}
    explicit Vector2uint32(const class Vector2& v);
    explicit Vector2uint32(class BinaryInput& bi);
    Vector2uint32(const class Vector2int16& v);
    Vector2uint32(const class Any& a);

    inline G3D::uint32& operator[] (int i) {
        debugAssert(((unsigned int)i) <= 1);
        return ((G3D::uint32*)this)[i];
    }

    inline const G3D::uint32& operator[] (int i) const {
        debugAssert(((unsigned int)i) <= 1);
        return ((G3D::uint32*)this)[i];
    }

    inline Vector2uint32 operator+(const Vector2uint32& other) const {
        return Vector2uint32(x + other.x, y + other.y);
    }

    inline Vector2uint32 operator-(const Vector2uint32& other) const {
        return Vector2uint32(x - other.x, y - other.y);
    }

    inline Vector2uint32 operator*(const Vector2uint32& other) const {
        return Vector2uint32(x * other.x, y * other.y);
    }

    inline Vector2uint32 operator*(const int s) const {
        return Vector2uint32(x * s, y * s);
    }

    inline Vector2uint32& operator+=(const Vector2uint32& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    /** Shifts both x and y */
    inline Vector2uint32 operator>>(const int s) const {
        return Vector2uint32(x >> s, y >> s);
    }

    /** Shifts both x and y */
    inline Vector2uint32 operator<<(const int s) const {
        return Vector2uint32(x << s, y << s);
    }

    inline Vector2uint32& operator-=(const Vector2uint32& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    inline Vector2uint32& operator*=(const Vector2uint32& other) {
        x *= other.x;
        y *= other.y;
        return *this;
    }

    Vector2uint32 clamp(const Vector2uint32& lo, const Vector2uint32& hi);

    inline bool operator==(const Vector2uint32& other) const {
        return (x == other.x) && (y == other.y);
    }

    inline bool operator!= (const Vector2uint32& other) const {
        return !(*this == other);
    }

    Vector2uint32 max(const Vector2uint32& v) const {
        return Vector2uint32(iMax(x, v.x), iMax(y, v.y));
    }

    Vector2uint32 min(const Vector2uint32& v) const {
        return Vector2uint32(iMin(x, v.x), iMin(y, v.y));
    }

    String toString() const;

    void serialize(class BinaryOutput& bo) const;
    void deserialize(class BinaryInput& bi);
}
G3D_END_PACKED_CLASS(2)

typedef Vector2uint32 Point2uint32;

} // namespace G3D

template<> struct HashTrait<G3D::Vector2uint32> {
    static size_t hashCode(const G3D::Vector2uint32& key) { return static_cast<size_t>(key.x ^ ((int)key.y << 1)); }
};

#endif
