/**
 \file Color3uint8.cpp
 
 \author Morgan McGuire, http://graphics.cs.williams.edu
  
 \created 2003-04-07
 \edited  2011-08-11
 */

#include "G3D/platform.h"
#include "G3D/g3dmath.h"
#include "G3D/Color3unorm8.h"
#include "G3D/Color3.h"
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"

namespace G3D {

Color3unorm8::Color3unorm8(const class Color3& c) : r(c.r), g(c.g), b(c.b) {
}


Color3unorm8::Color3unorm8(class BinaryInput& bi) {
    deserialize(bi);
}


void Color3unorm8::serialize(class BinaryOutput& bo) const {
    bo.writeUInt8(r.bits());
    bo.writeUInt8(g.bits());
    bo.writeUInt8(b.bits());
}


void Color3unorm8::deserialize(class BinaryInput& bi) {
    r = unorm8::fromBits(bi.readUInt8());
    g = unorm8::fromBits(bi.readUInt8());
    b = unorm8::fromBits(bi.readUInt8());
}


}
