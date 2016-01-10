/**
 \file G3D/source/Color1unorm8.cpp
 
 \author Morgan McGuire, http://graphics.cs.williams.edu
  
 \created 2007-01-30
 \edited  2011-08-11
 */
#include "G3D/platform.h"
#include "G3D/unorm8.h"
#include "G3D/Color1unorm8.h"
#include "G3D/Color1.h"
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"

namespace G3D {

Color1unorm8::Color1unorm8(const class Color1& c) : value(c.value) {
}


Color1unorm8::Color1unorm8(class BinaryInput& bi) {
    deserialize(bi);
}


void Color1unorm8::serialize(class BinaryOutput& bo) const {
    bo.writeUInt8(value.bits());
}


void Color1unorm8::deserialize(class BinaryInput& bi) {
    value = unorm8::fromBits(bi.readUInt8());
}


}
