/**
  @file Vector2uint16.h
  
  @maintainer Morgan McGuire

  @created 2003-08-09
  @edited  2016-10-05

  Copyright 2000-2016, Morgan McGuire.
  All rights reserved.
 */

#pragma once

#include "G3D/platform.h"
#include "G3D/g3dmath.h"
#include "G3D/HashTrait.h"

namespace G3D {

class Any;
/**
 \class Vector2uint16 
 A Vector2 that packs its fields into G3D::uint16 s.
 */
G3D_BEGIN_PACKED_CLASS(2)
Vector2uint16 {
private:
    // Hidden operators
    bool operator<(const Vector2uint16&) const;
    bool operator>(const Vector2uint16&) const;
    bool operator<=(const Vector2uint16&) const;
    bool operator>=(const Vector2uint16&) const;

public:
    G3D::uint16              x;
    G3D::uint16              y;

    Vector2uint16() : x(0), y(0) {}
    Vector2uint16(G3D::uint16 _x, G3D::uint16 _y) : x(_x), y(_y){}
    explicit Vector2uint16(const class Vector2& v);
    explicit Vector2uint16(class BinaryInput& bi);
    explicit Vector2uint16(const class Any& a);
    explicit Vector2uint16(const class Vector2int32& v);

    Any toAny() const;
    
    Vector2uint16& operator=(const Any& a);

    inline G3D::int16& operator[] (int i) {
        debugAssert(((unsigned int)i) <= 1);
        return ((G3D::int16*)this)[i];
    }

    inline const G3D::int16& operator[] (int i) const {
        debugAssert(((unsigned int)i) <= 1);
        return ((G3D::int16*)this)[i];
    }

    inline Vector2uint16 operator+(const Vector2uint16& other) const {
        return Vector2uint16(x + other.x, y + other.y);
    }

    inline Vector2uint16 operator-(const Vector2uint16& other) const {
        return Vector2uint16(x - other.x, y - other.y);
    }

    inline Vector2uint16 operator*(const Vector2uint16& other) const {
        return Vector2uint16(x * other.x, y * other.y);
    }


    inline Vector2uint16 operator*(const unsigned int s) const {
        return Vector2uint16(x * s, y * s);
    }

    inline Vector2uint16& operator+=(const Vector2uint16& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    bool isZero() const {
        return (x == 0) && (y == 0);
    }

    /** Shifts both x and y */
    inline Vector2uint16 operator>>(const int s) const {
        return Vector2uint16(x >> s, y >> s);
    }

    /** Shifts both x and y */
    inline Vector2uint16 operator<<(const int s) const {
        return Vector2uint16(x << s, y << s);
    }

    inline Vector2uint16& operator-=(const Vector2uint16& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    inline Vector2uint16& operator*=(const Vector2uint16& other) {
        x *= other.x;
        y *= other.y;
        return *this;
    }

    Vector2uint16 clamp(const Vector2uint16& lo, const Vector2uint16& hi);

    inline bool operator== (const Vector2uint16& rkVector) const {
        return ((int32*)this)[0] == ((int32*)&rkVector)[0];
    }

    inline bool operator!= (const Vector2uint16& rkVector) const {
        return ((int32*)this)[0] != ((int32*)&rkVector)[0];
    }

    Vector2uint16 max(const Vector2uint16& v) const {
        return Vector2uint16(iMax(x, v.x), iMax(y, v.y));
    }

    Vector2uint16 min(const Vector2uint16& v) const {
        return Vector2uint16(iMin(x, v.x), iMin(y, v.y));
    }

    void serialize(class BinaryOutput& bo) const;
    void deserialize(class BinaryInput& bi);
}
G3D_END_PACKED_CLASS(2)

typedef Vector2uint16 Point2uint16;    

}

template<> struct HashTrait<G3D::Vector2uint16> {
    static size_t hashCode(const G3D::Vector2uint16& key) { return static_cast<size_t>(key.x + ((int)key.y << 16)); }
};

