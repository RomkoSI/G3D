/**
 @file Vector2uint32.cpp
 
 @author Morgan McGuire, http://graphics.cs.williams.edu
  
 @created 2003-08-09
 @edited  2015-07-13
 */

#include "G3D/platform.h"
#include "G3D/g3dmath.h"
#include "G3D/Vector2uint32.h"
#include "G3D/Vector2.h"
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"
#include "G3D/Vector2int16.h"
#include "G3D/Any.h"

namespace G3D {

Vector2uint32::Vector2uint32(const Any& any) {
    any.verifyName("Vector2uint32", "Point2uint32");
    any.verifyType(Any::TABLE, Any::ARRAY);
    any.verifySize(2);

    if (any.type() == Any::ARRAY) {
        x = any[0];
        y = any[1];
    } else {
        // Table
        x = any["x"];
        y = any["y"];
    }
}


Vector2uint32::Vector2uint32(const class Vector2int16& v) : x(v.x), y(v.y) {
}

Vector2uint32::Vector2uint32(const class Vector2& v) {
    x = (uint32)iFloor(v.x + 0.5);
    y = (uint32)iFloor(v.y + 0.5);
}


Vector2uint32::Vector2uint32(class BinaryInput& bi) {
    deserialize(bi);
}


void Vector2uint32::serialize(class BinaryOutput& bo) const {
    bo.writeUInt32(x);
    bo.writeUInt32(y);
}


void Vector2uint32::deserialize(class BinaryInput& bi) {
    x = bi.readUInt32();
    y = bi.readUInt32();
}


Vector2uint32 Vector2uint32::clamp(const Vector2uint32& lo, const Vector2uint32& hi) {
    return Vector2uint32(uiClamp(x, lo.x, hi.x), uiClamp(y, lo.y, hi.y));
}


String Vector2uint32::toString() const {
    return G3D::format("(%d, %d)", x, y);
}

}
