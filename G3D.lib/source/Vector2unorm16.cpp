/**
 \file G3D/Vector2int16.cpp
 
 \author Morgan McGuire, http://graphics.cs.williams.edu
  
 \created 2012-03-13
 \edited  2012-03-13
 */

#include "G3D/platform.h"
#include "G3D/g3dmath.h"
#include "G3D/Vector2.h"
#include "G3D/Vector2unorm16.h"
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"
#include "G3D/Any.h"

namespace G3D {

Vector2unorm16::Vector2unorm16(const Any& any) {
    any.verifyName("Vector2unorm16", "Point2unorm16");
    any.verifyType(Any::TABLE, Any::ARRAY);
    any.verifySize(2);

    if (any.type() == Any::ARRAY) {
        x = unorm16(any[0]);
        y = unorm16(any[1]);
    } else {
        // Table
        x = unorm16(any["x"]);
        y = unorm16(any["y"]);
    }
}


Vector2unorm16& Vector2unorm16::operator=(const Any& a) {
    *this = Vector2unorm16(a);
    return *this;
}


Any Vector2unorm16::toAny() const {
    Any any(Any::ARRAY, "Vector2unorm16");
    any.append(x, y);
    return any;
}

Vector2unorm16::Vector2unorm16(const class Vector2& v) {
    x = unorm16(v.x);
    y = unorm16(v.y);
}


Vector2unorm16::Vector2unorm16(class BinaryInput& bi) {
    deserialize(bi);
}


void Vector2unorm16::serialize(class BinaryOutput& bo) const {
    bo.writeUInt16(x.bits());
    bo.writeUInt16(y.bits());
}


void Vector2unorm16::deserialize(class BinaryInput& bi) {
    x = unorm16::fromBits(bi.readUInt16());
    y = unorm16::fromBits(bi.readUInt16());
}

}
