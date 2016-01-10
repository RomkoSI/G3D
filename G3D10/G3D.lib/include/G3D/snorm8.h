/**
  \file G3D/snorm8.h
 
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
  \created 2012-03-02 by Zina Cigolle
  \edited  2013-09-02

  Copyright 2000-2015, Morgan McGuire.
  All rights reserved.
 */
#ifndef G3D_snorm8_h
#define G3D_snorm8_h

#include "G3D/platform.h"
#include "G3D/g3dmath.h"

namespace G3D {


/** Represents numbers on [-1, 1] in 8 bits as a signed normalized
 0.8 fixed-point value using the same encoding scheme as OpenGL.  

 Note that arithmetic operations may over and under-flow, just like
 int8 arithmetic.

 OpenGL specifications can be found here: 
 <www.opengl.org/registry/specs/ARB/shading_language_packing.txt>

*/
G3D_BEGIN_PACKED_CLASS(1)
snorm8 {
private:
    int8    m_bits;

    /** Private to prevent illogical conversions without explicitly
     stating that the input should be treated as bits; see fromBits. */
    explicit snorm8(int8 b) : m_bits(b) {}

public:

    /** Equivalent to: \code snorm8 u = reinterpret_cast<const snorm8&>(b);\endcode */
    static snorm8 fromBits(int8 b) {
        return snorm8(b);
    }

    /** \copydoc fromBits */
    static snorm8 reinterpretFrom(int8 b) {
        return snorm8(b);
    }

    snorm8() : m_bits(0) {}
    
    snorm8(const snorm8& other) : m_bits(other.m_bits) {}

    /** Maps f to round(f * 127).*/
    explicit snorm8(float f) {
        m_bits = (int8)(round(clamp(f, -1.0f, 1.0f) * 127.0f));
    }

    /** Returns a number on [-1.0f, 1.0f] */
    operator float() const {
        return float(clamp(int(m_bits) * (1.0f / 127.0f), -1.0f, 1.0f));
    }

    // Needs to be changed?
    static snorm8 one() {
        return fromBits(127);
    }

    static snorm8 zero() {
        return fromBits(0);
    }

    /**\brief Returns the underlying bits in this representation. 
     Equivalent to:
    \code uint8 i = reinterpret_cast<const uint8&>(u); \endcode */
    uint8 bits() const {
        return m_bits;
    }

    /** \copydoc bits */
    int8 reinterpretAsInt8() const {
        return m_bits;
    }

    bool operator>(const snorm8 other) const {
        return m_bits > other.m_bits;
    }

    bool operator<(const snorm8 other) const {
        return m_bits < other.m_bits;
    }

    bool operator>=(const snorm8 other) const {
        return m_bits >= other.m_bits;
    }

    bool operator<=(const snorm8 other) const {
        return m_bits <= other.m_bits;
    }

    bool operator==(const snorm8 other) const {
        return m_bits <= other.m_bits;
    }

    bool operator!=(const snorm8 other) const {
        return m_bits != other.m_bits;
    }

    snorm8 operator+(const snorm8 other) const {
        return snorm8(int8(m_bits + other.m_bits));
    }

    snorm8& operator+=(const snorm8 other) {
        m_bits += other.m_bits;
        return *this;
    }

    snorm8 operator-(const snorm8 other) const {
        return snorm8(int8(m_bits - other.m_bits));
    }

    snorm8& operator-=(const snorm8 other) {
        m_bits -= other.m_bits;
        return *this;
    }

    snorm8 operator*(const int i) const {
        return snorm8(int8(m_bits * i));
    }

    snorm8& operator*=(const int i) {
        m_bits *= i;
        return *this;
    }

    snorm8 operator/(const int i) const {
        return snorm8(int8(m_bits / i));
    }

    snorm8& operator/=(const int i) {
        m_bits /= i;
        return *this;
    }

    snorm8 operator<<(const int i) const {
        return snorm8((int8)(m_bits << i));
    }

    snorm8& operator<<=(const int i) {
        m_bits <<= i;
        return *this;
    }

    snorm8 operator>>(const int i) const {
        return snorm8(int8(m_bits >> i));
    }

    snorm8& operator>>=(const int i) {
        m_bits >>= i;
        return *this;
    }
}
G3D_END_PACKED_CLASS(1)

} // namespace G3D

#endif // G3D_snorm8
