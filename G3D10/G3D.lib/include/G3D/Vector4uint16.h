/**
  \file G3D/Vector4uint16.h
  
  \maintainer Michael Mara, mmara@cs.stanford.edu

  \created 2015-09-15
  \edited  2015-09-15

  Copyright 2000-2015, Morgan McGuire.
  All rights reserved.
 */
#ifndef Vector4uint16_h
#define Vector4uint16_h

#include "G3D/platform.h"
#include "G3D/g3dmath.h"
#include "G3D/HashTrait.h"

namespace G3D {

G3D_BEGIN_PACKED_CLASS(2)
Vector4uint16 {
private:
    // Hidden operators
    bool operator<(const Vector4uint16&) const;
    bool operator>(const Vector4uint16&) const;
    bool operator<=(const Vector4uint16&) const;
    bool operator>=(const Vector4uint16&) const;

public:

    G3D::uint16 x, y, z, w;

    Vector4uint16() : x(0), y(0), z(0), w(0) {}
    Vector4uint16(G3D::uint16 _x, G3D::uint16 _y, G3D::uint16 _z, G3D::uint16 _w) : x(_x), y(_y), z(_z), w(_w) {}
    explicit Vector4uint16(const class Vector4& v);
    explicit Vector4uint16(class BinaryInput& bi);

    void serialize(class BinaryOutput& bo) const;
    void deserialize(class BinaryInput& bi);

    inline G3D::uint16& operator[] (int i) {
        debugAssert(i <= 3);
        return ((G3D::uint16*)this)[i];
    }

    inline const G3D::uint16& operator[] (int i) const {
        debugAssert(i <= 3);
        return ((G3D::uint16*)this)[i];
    }

    inline Vector4uint16 operator+(const Vector4uint16& other) const {
        return Vector4uint16(x + other.x, y + other.y, z + other.z, w + other.w);
    }

    inline Vector4uint16 operator-(const Vector4uint16& other) const {
        return Vector4uint16(x - other.x, y - other.y, z - other.z, w - other.w);
    }

    inline Vector4uint16 operator*(const Vector4uint16& other) const {
        return Vector4uint16(x * other.x, y * other.y, z * other.z, w * other.w);
    }

    inline Vector4uint16 operator*(const int s) const {
        return Vector4uint16(int16(x * s), int16(y * s), int16(z * s), int16(w * s));
    }

    inline Vector4uint16& operator+=(const Vector4uint16& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }

    inline Vector4uint16& operator-=(const Vector4uint16& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }

    inline Vector4uint16& operator*=(const Vector4uint16& other) {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        w *= other.w;
        return *this;
    }

    inline bool operator== (const Vector4uint16& rkVector) const {
        return ( x == rkVector.x && y == rkVector.y && z == rkVector.z && w == rkVector.w);
    }

    inline bool operator!= (const Vector4uint16& rkVector) const {
        return ( x != rkVector.x || y != rkVector.y || z != rkVector.z || w != rkVector.w);
    }

    String toString() const;

} G3D_END_PACKED_CLASS(2)

} // namespace G3D

template <> struct HashTrait<G3D::Vector4uint16> {
    static size_t hashCode(const G3D::Vector4uint16& vertex) {
        return G3D::wangHash6432Shift(*(G3D::int64*)&vertex);
    }
};



#endif // Vector4uint16_h

