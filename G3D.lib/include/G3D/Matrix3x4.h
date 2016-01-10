/**
  \file Matrix3x4.h
 
  3x4 matrix class
 

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
  \created 2014-06-23
  \edited  2014-06-23
 */

#ifndef G3D_Matrix3x4_h
#define G3D_Matrix3x4_h

#ifdef _MSC_VER
// Disable conditional expression is constant, which occurs incorrectly on inlined functions
#   pragma warning (push)
#   pragma warning( disable : 4127 )
#endif

#include "G3D/platform.h"
#include "G3D/DoNotInitialize.h"
#include "G3D/debugAssert.h"
#include "G3D/Vector3.h"

namespace G3D {

class Any;
class Matrix2;

/**
  A 3x4 matrix.  Do not subclass.  Data is initialized to 0 when default constructed.

  \sa G3D::CoordinateFrame, G3D::Matrix3, G3D::Matrix4, G3D::Quat
 */
class Matrix3x4 {
private:

    float elt[3][4];

    /**
      Computes the determinant of the 3x3 matrix that lacks excludeRow
      and excludeCol. 
    */

    // Hidden operators
    bool operator<(const Matrix3x4&) const;
    bool operator>(const Matrix3x4&) const;
    bool operator<=(const Matrix3x4&) const;
    bool operator>=(const Matrix3x4&) const;

public:

    /** Must be in one of the following forms:
        - Matrix3x4(#, #, # .... #)
        - Matrix3x4::fromIdentity()
    */
    explicit Matrix3x4(const Any& any);

    Any toAny() const;

    Matrix3x4
       (float r1c1, float r1c2, float r1c3, float r1c4,
        float r2c1, float r2c2, float r2c3, float r2c4,
        float r3c1, float r3c2, float r3c3, float r3c4);

    Matrix3x4(DoNotInitialize dni) {}

    /**
     init should be <B>row major</B>.
     */
    Matrix3x4(const float* init);
    
    explicit Matrix3x4(const class Matrix3& m3x3);

    explicit Matrix3x4(const class Matrix4& m4x4);

    explicit Matrix3x4(const class CoordinateFrame& c);

    Matrix3x4(const double* init);

    /** Matrix3x4::zero() */
    Matrix3x4();

    // Special values.
    // Intentionally not inlined: see Matrix3::identity() for details.
    static const Matrix3x4& fromIdentity();

    static const Matrix3x4& zero();
        
    inline float* operator[](int r) {
        debugAssert(r >= 0);
        debugAssert(r < 3);
        return (float*)&elt[r];
    }

    inline const float* operator[](int r) const {
        debugAssert(r >= 0);
        debugAssert(r < 3);
        return (const float*)&elt[r];
    } 

    /** Returns a row-major pointer. */
    inline operator float* () {
        return (float*)&elt[0][0];
    }

    inline operator const float* () const {
        return (const float*)&elt[0][0];
    }

    Matrix3x4 operator*(const Matrix4& other) const;
    Vector3 operator*(const Vector4& other) const;

    Matrix3x4 operator+(const Matrix3x4& other) const;
    Matrix3x4 operator-(const Matrix3x4& other) const;

    Matrix3x4 operator*(const float s) const;
    Matrix3x4 operator/(const float s) const;

    bool operator!=(const Matrix3x4& other) const;
    bool operator==(const Matrix3x4& other) const;

    bool fuzzyEq(const Matrix3x4& b) const;

    String toString() const;

};

}

#ifdef _MSC_VER
#   pragma warning (pop)
#endif

#endif
