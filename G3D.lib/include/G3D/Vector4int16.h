/**
  \file G3D/Vector4int16.h
  
  \maintainer Morgan McGuire, matrix@brown.edu

  \created 2011-06-19
  \edited  2011-06-19

  Copyright 2000-2011, Morgan McGuire.
  All rights reserved.
 */
#ifndef Vector4int16_h
#define Vector4int16_h

#include "G3D/platform.h"
#include "G3D/g3dmath.h"
#include "G3D/HashTrait.h"

namespace G3D {

G3D_BEGIN_PACKED_CLASS(2)
Vector4int16 {
private:
    // Hidden operators
    bool operator<(const Vector4int16&) const;
    bool operator>(const Vector4int16&) const;
    bool operator<=(const Vector4int16&) const;
    bool operator>=(const Vector4int16&) const;

public:

    G3D::int16 x, y, z, w;

    Vector4int16() : x(0), y(0), z(0), w(0) {}
    Vector4int16(G3D::int16 _x, G3D::int16 _y, G3D::int16 _z, G3D::int16 _w) : x(_x), y(_y), z(_z), w(_w) {}
    explicit Vector4int16(const class Vector4& v);
    explicit Vector4int16(class BinaryInput& bi);

    void serialize(class BinaryOutput& bo) const;
    void deserialize(class BinaryInput& bi);

    inline G3D::int16& operator[] (int i) {
        debugAssert(i <= 3);
        return ((G3D::int16*)this)[i];
    }

    inline const G3D::int16& operator[] (int i) const {
        debugAssert(i <= 3);
        return ((G3D::int16*)this)[i];
    }

    inline Vector4int16 operator+(const Vector4int16& other) const {
        return Vector4int16(x + other.x, y + other.y, z + other.z, w + other.w);
    }

    inline Vector4int16 operator-(const Vector4int16& other) const {
        return Vector4int16(x - other.x, y - other.y, z - other.z, w - other.w);
    }

    inline Vector4int16 operator*(const Vector4int16& other) const {
        return Vector4int16(x * other.x, y * other.y, z * other.z, w * other.w);
    }

    inline Vector4int16 operator*(const int s) const {
        return Vector4int16(int16(x * s), int16(y * s), int16(z * s), int16(w * s));
    }

    inline Vector4int16& operator+=(const Vector4int16& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }

    inline Vector4int16& operator-=(const Vector4int16& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }

    inline Vector4int16& operator*=(const Vector4int16& other) {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        w *= other.w;
        return *this;
    }

    inline bool operator== (const Vector4int16& rkVector) const {
        return ( x == rkVector.x && y == rkVector.y && z == rkVector.z && w == rkVector.w);
    }

    inline bool operator!= (const Vector4int16& rkVector) const {
        return ( x != rkVector.x || y != rkVector.y || z != rkVector.z || w != rkVector.w);
    }

    String toString() const;

} G3D_END_PACKED_CLASS(2)

} // namespace G3D

template <> struct HashTrait<G3D::Vector4int16> {
    static size_t hashCode(const G3D::Vector4int16& vertex) {
        return G3D::wangHash6432Shift(*(G3D::int64*)&vertex);
    }
};



#endif // Vector4int16_h

