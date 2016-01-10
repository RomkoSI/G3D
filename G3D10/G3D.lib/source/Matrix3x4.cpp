/**
  \file G3D/source/Matrix3x4.cpp 
 
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2014-06-23
  \edited  2014-06-23
 */

#include "G3D/platform.h"
#include "G3D/Matrix3x4.h"
#include "G3D/Matrix4.h"
#include "G3D/Matrix3.h"
#include "G3D/Matrix2.h"
#include "G3D/Vector4.h"
#include "G3D/Vector3.h"
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"
#include "G3D/CoordinateFrame.h"
#include "G3D/Rect2D.h"
#include "G3D/Any.h"
#include "G3D/stringutils.h"

namespace G3D {

    
Matrix3x4::Matrix3x4(const Any& any) {
    any.verifyNameBeginsWith("Matrix3x4", "CFrame", "CoordinateFrame");
    any.verifyType(Any::ARRAY);

    const String& name = any.name();
    if (name == "Matrix3x4") {
        any.verifySize(12);

        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 4; ++c) {
                elt[r][c] = any[r * 3 + c];
            }
        }
    }  else if (name == "Matrix3x4::fromIdentity") {
        *this = fromIdentity();
    } else if (beginsWith(name, "CFrame") || beginsWith(name, "CoordinateFrame")) {
        *this = Matrix3x4(CFrame(any));
    } else {
        any.verify(false, "Expected Matrix4x4 constructor");
    }
}


Any Matrix3x4::toAny() const {
    Any any(Any::ARRAY, "Matrix3x4");
    any.resize(12);
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) {
            any[r * 3 + c] = elt[r][c];
        }
    }

    return any;
}


const Matrix3x4& Matrix3x4::fromIdentity() {
    static Matrix3x4 m(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0);
    return m;
}


const Matrix3x4& Matrix3x4::zero() {
    static Matrix3x4 m(
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0);
    return m;
}


Matrix3x4::Matrix3x4(const class CoordinateFrame& cframe) {
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            elt[r][c] = cframe.rotation[r][c];
        }
        elt[r][3] = cframe.translation[r];
    }
}


Matrix3x4::Matrix3x4(const Matrix3& m3x3) {
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            elt[r][c] = m3x3[r][c];
        }
        elt[r][3] = 0.0f;
    }
}


Matrix3x4::Matrix3x4(const Matrix4& m4x4) {
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) {
            elt[r][c] = m4x4[r][c];
        }
    }
}


bool Matrix3x4::fuzzyEq(const Matrix3x4& b) const {
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) {
            if (! G3D::fuzzyEq(elt[r][c], b[r][c])) {
                return false;
            }
        }
    }
    return true;
}


Matrix3x4::Matrix3x4
(
    float r1c1, float r1c2, float r1c3, float r1c4,
    float r2c1, float r2c2, float r2c3, float r2c4,
    float r3c1, float r3c2, float r3c3, float r3c4) {
    elt[0][0] = r1c1;  elt[0][1] = r1c2;  elt[0][2] = r1c3;  elt[0][3] = r1c4;
    elt[1][0] = r2c1;  elt[1][1] = r2c2;  elt[1][2] = r2c3;  elt[1][3] = r2c4;
    elt[2][0] = r3c1;  elt[2][1] = r3c2;  elt[2][2] = r3c3;  elt[2][3] = r3c4;
}


Matrix3x4::Matrix3x4(const float* init) {
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) {
            elt[r][c] = init[r * 3 + c];
        }
    }
}


Matrix3x4::Matrix3x4(const double* init) {
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) {
            elt[r][c] = (float)init[r * 3 + c];
        }
    }
}


Matrix3x4::Matrix3x4() {
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) {
            elt[r][c] = 0;
        }
    }
}


Matrix3x4 Matrix3x4::operator*(const Matrix4& other) const {
    Matrix3x4 result;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) {
            for (int i = 0; i < 4; ++i) {
                result.elt[r][c] += elt[r][i] * other[i][c];
            }
        }
    }

    return result;
}


Matrix3x4 Matrix3x4::operator*(const float s) const {
    Matrix3x4 result;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) {
            result.elt[r][c] = elt[r][c] * s;
        }
    }

    return result;
}


Matrix3x4 Matrix3x4::operator/(const float s) const {
    Matrix3x4 result;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) {
            result.elt[r][c] = elt[r][c] / s;
        }
    }

    return result;
}


Matrix3x4 Matrix3x4::operator+(const Matrix3x4& other) const {
    Matrix3x4 result;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) {
            result.elt[r][c] = elt[r][c] + other.elt[r][c];
        }
    }
    return result;
}


Matrix3x4 Matrix3x4::operator-(const Matrix3x4& other) const {
    Matrix3x4 result;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) {
            result.elt[r][c] = elt[r][c] - other.elt[r][c];
        }
    }
    return result;
}


Vector3 Matrix3x4::operator*(const Vector4& vector) const {
    Vector3 result(0,0,0);
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) {
            result[r] += elt[r][c] * vector[c];
        }
    }

    return result;
}


bool Matrix3x4::operator!=(const Matrix3x4& other) const {
    return ! (*this == other);
}


bool Matrix3x4::operator==(const Matrix3x4& other) const {

    // If the bit patterns are identical, they must be
    // the same matrix.  They might still differ due 
    // to floating point issues such as -0 == 0.
    if (memcmp(this, &other, sizeof(Matrix3x4)) == 0) {
        return true;
    } 

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) {
            if (elt[r][c] != other.elt[r][c]) {
                return false;
            }
        }
    }

    return true;
}


String Matrix3x4::toString() const {
    return G3D::format("[%g, %g, %g, %g; %g, %g, %g, %g; %g, %g, %g, %g]", 
            elt[0][0], elt[0][1], elt[0][2], elt[0][3],
            elt[1][0], elt[1][1], elt[1][2], elt[1][3],
            elt[2][0], elt[2][1], elt[2][2], elt[2][3]);
}


} // namespace


