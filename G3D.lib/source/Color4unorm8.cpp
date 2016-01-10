/**
 \file G3D.lib/source/Color4unorm8.cpp
 
 \author Morgan McGuire, http://graphics.cs.williams.edu
  
 \created 2003-04-07
 \edited  2011-09-07

  Copyright 2000-2015, Morgan McGuire.
  All rights reserved.
 */
#include "G3D/platform.h"
#include "G3D/g3dmath.h"
#include "G3D/Color4unorm8.h"
#include "G3D/Color4.h"
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"

namespace G3D {

Color4unorm8::Color4unorm8(const class Color4& c) : r(c.r), g(c.g), b(c.b), a(c.a) {
}


Color4unorm8::Color4unorm8(class BinaryInput& bi) {
    deserialize(bi);
}


void Color4unorm8::serialize(class BinaryOutput& bo) const {
    bo.writeUInt8(r.bits());
    bo.writeUInt8(g.bits());
    bo.writeUInt8(b.bits());
    bo.writeUInt8(a.bits());
}


void Color4unorm8::deserialize(class BinaryInput& bi) {
    r = unorm8::fromBits(bi.readUInt8());
    g = unorm8::fromBits(bi.readUInt8());
    b = unorm8::fromBits(bi.readUInt8());
    a = unorm8::fromBits(bi.readUInt8());
}


}
