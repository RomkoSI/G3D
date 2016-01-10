/**
 \file Vector4int16.cpp
 
 \author Morgan McGuire, http://graphics.cs.williams.edu
  
 \created 2011-06-19
 \edited  2011-06-19
 */

#include "G3D/platform.h"
#include "G3D/g3dmath.h"
#include "G3D/Vector4int16.h"
#include "G3D/Vector4.h"
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"
#include "G3D/format.h"

namespace G3D {

Vector4int16::Vector4int16(const class Vector4& v) {
    x = (int16)iFloor(v.x + 0.5f);
    y = (int16)iFloor(v.y + 0.5f);
    z = (int16)iFloor(v.z + 0.5f);
    w = (int16)iFloor(v.w + 0.5f);
}


Vector4int16::Vector4int16(class BinaryInput& bi) {
    deserialize(bi);
}


void Vector4int16::serialize(class BinaryOutput& bo) const {
    bo.writeInt16(x);
    bo.writeInt16(y);
    bo.writeInt16(z);
    bo.writeInt16(w);
}


void Vector4int16::deserialize(class BinaryInput& bi) {
    x = bi.readInt16();
    y = bi.readInt16();
    z = bi.readInt16();
    w = bi.readInt16();
}


String Vector4int16::toString() const {
    return G3D::format("(%d, %d, %d, %d)", x, y, z, w);
}

}
