/**
  \file G3D/snorm16.h
 
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
  \created 2012-03-02 by Zina Cigolle
  \edited  2012-03-02

  Copyright 2000-2015, Morgan McGuire.
  All rights reserved.
 */
#ifndef G3D_snorm16_h
#define G3D_snorm16_h

#include "G3D/platform.h"
#include "G3D/g3dmath.h"

namespace G3D {


/** Represents numbers on [-1, 1] in 16 bits as an unsigned normalized
 0.8 fixed-point value using the same encoding scheme as OpenGL.  

 Note that arithmetic operations may over and under-flow, just like
 int16 arithmetic.

 OpenGL specifications can be found here: 
 <www.opengl.org/registry/specs/ARB/shading_language_packing.txt>

*/
G3D_BEGIN_PACKED_CLASS(1)
snorm16 {
private:
    int16    m_bits;

    /** Private to prevent illogical conversions without explicitly
     stating that the input should be treated as bits; see fromBits. */
    explicit snorm16(int16 b) : m_bits(b) {}

public:

    /** Equivalent to: \code snorm16 u = reinterpret_cast<const snorm16&>(b);\endcode */
    static snorm16 fromBits(int16 b) {
        return snorm16(b);
    }

    /** \copydoc fromBits */
    static snorm16 reinterpretFrom(int16 b) {
        return snorm16(b);
    }

    snorm16() : m_bits(0) {}
    
    snorm16(const snorm16& other) : m_bits(other.m_bits) {}

    /** Maps f to round(f * 32767).*/
    explicit snorm16(float f) {
        m_bits = (int)round(clamp(f, -1.0f, 1.0f) * 32767.0f);
    }

    /** Returns a number on [-1.0f, 1.0f] */
    operator float() const {
        return clamp(int(m_bits) * (1.0f / 32767.0f), -1.0f, 1.0f);
    }

    static snorm16 one() {
        return fromBits(32767);
    }

    static snorm16 zero() {
        return fromBits(0);
    }

    /**\brief Returns the underlying bits in this representation. 
     Equivalent to:
    \code int16 i = reinterpret_cast<const int16&>(u); \endcode */
    int16 bits() const {
        return m_bits;
    }

    /** \copydoc bits */
    int16 reinterpretAsInt16() const {
        return m_bits;
    }

    bool operator>(const snorm16 other) const {
        return m_bits > other.m_bits;
    }

    bool operator<(const snorm16 other) const {
        return m_bits < other.m_bits;
    }

    bool operator>=(const snorm16 other) const {
        return m_bits >= other.m_bits;
    }

    bool operator<=(const snorm16 other) const {
        return m_bits <= other.m_bits;
    }

    bool operator==(const snorm16 other) const {
        return m_bits <= other.m_bits;
    }

    bool operator!=(const snorm16 other) const {
        return m_bits != other.m_bits;
    }

    snorm16 operator+(const snorm16 other) const {
        return snorm16(int16(m_bits + other.m_bits));
    }

    snorm16& operator+=(const snorm16 other) {
        m_bits += other.m_bits;
        return *this;
    }

    snorm16 operator-(const snorm16 other) const {
        return snorm16(int16(m_bits - other.m_bits));
    }

    snorm16& operator-=(const snorm16 other) {
        m_bits -= other.m_bits;
        return *this;
    }

    snorm16 operator*(const int i) const {
        return snorm16(int16(m_bits * i));
    }

    snorm16& operator*=(const int i) {
        m_bits *= i;
        return *this;
    }

    snorm16 operator/(const int i) const {
        return snorm16(int16(m_bits / i));
    }

    snorm16& operator/=(const int i) {
        m_bits /= i;
        return *this;
    }

    snorm16 operator<<(const int i) const {
        return snorm16((int16)(m_bits << i));
    }

    snorm16& operator<<=(const int i) {
        m_bits <<= i;
        return *this;
    }

    snorm16 operator>>(const int i) const {
        return snorm16(int16(m_bits >> i));
    }

    snorm16& operator>>=(const int i) {
        m_bits >>= i;
        return *this;
    }
}
G3D_END_PACKED_CLASS(1)

} // namespace G3D

#endif // G3D_snorm16
