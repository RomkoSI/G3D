/**
 @file Vector3int32.cpp
 
 @author Michael Mara
  
 @created 2008-01-11
 @edited  2013-01-11
 */

#include "G3D/platform.h"
#include "G3D/g3dmath.h"
#include "G3D/Vector4int32.h"
#include "G3D/Vector4int16.h"
#include "G3D/Vector4.h"
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"
#include "G3D/format.h"
#include "G3D/Vector3int32.h"
#include "G3D/Vector3int16.h"
#include "G3D/Vector2int32.h"
#include "G3D/Vector2int16.h"
#include "G3D/Any.h"

namespace G3D {
Vector4int32 iFloor(const Vector4& v) {
    return Vector4int32(iFloor(v.x), iFloor(v.y), iFloor(v.z), iFloor(v.w));
}

Vector4int32::Vector4int32(const Any& any) {
    *this = Vector4int32();
    any.verifyNameBeginsWith("Vector4int32", "Point4int32");
    
    switch (any.type()) {
    case Any::TABLE:
        
        for (Any::AnyTable::Iterator it = any.table().begin(); it.isValid(); ++it) {
            const String& key = toLower(it->key);
            
            if (key == "x") {
                x = it->value;
            } else if (key == "y") {
                y = it->value;
            } else if (key == "z") {
                z = it->value;
            } else if (key == "w") {
                w = it->value;
            } else {
                any.verify(false, "Illegal key: " + it->key);
            }
        }
        break;
            
    case Any::ARRAY:
        
        (void)any.name();
        if (any.size() == 1) {
            x = y = z = w = any[0];
        } else {
            any.verifySize(4);
            x = any[0];
            y = any[1];
            z = any[2];
            w = any[3];
        }
        break;
        
    default:
        any.verify(false, "Bad Vector4int32 constructor");
    }
}


Any Vector4int32::toAny() const {
    Any a(Any::ARRAY, "Vector4int32");
    a.append(x, y, z, w);
    return a;
}


Vector4int32::Vector4int32(const class Vector4& v) {
    x = (int32)(v.x + 0.5);
    y = (int32)(v.y + 0.5);
    z = (int32)(v.z + 0.5);
    w = (int32)(v.w + 0.5);
}



Vector4int32 Vector4int32::truncate(const class Vector4& v) {
    return Vector4int32(int32(v.x), int32(v.y), int32(v.z), int32(v.w));
}



String Vector4int32::toString() const {
    return G3D::format("(%d, %d, %d, %d)", x, y, z, w);
}

//----------------------------------------------------------------------------
// 2-char swizzles

Vector2int32 Vector4int32::xx() const  { return Vector2int32       (x, x); }
Vector2int32 Vector4int32::yx() const  { return Vector2int32       (y, x); }
Vector2int32 Vector4int32::zx() const  { return Vector2int32       (z, x); }
Vector2int32 Vector4int32::xy() const  { return Vector2int32       (x, y); }
Vector2int32 Vector4int32::yy() const  { return Vector2int32       (y, y); }
Vector2int32 Vector4int32::zy() const  { return Vector2int32       (z, y); }
Vector2int32 Vector4int32::xz() const  { return Vector2int32       (x, z); }
Vector2int32 Vector4int32::yz() const  { return Vector2int32       (y, z); }
Vector2int32 Vector4int32::zz() const  { return Vector2int32       (z, z); }
}
