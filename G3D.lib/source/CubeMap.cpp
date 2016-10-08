/**
  \file CubeMap.cpp
  Copyright 2002-2016, Morgan McGuire

  \created 2002-05-27
  \edited  2016-10-07
 */

#include "G3D/CubeMap.h"
#include "G3D/Image.h"
#include "G3D/Vector3.h"
#include "G3D/Color4.h"

namespace G3D {

shared_ptr<CubeMap> CubeMap::create(const Array<shared_ptr<Image>>& face) {
    return createShared<CubeMap>(face);
}


CubeMap::CubeMap(const Array<shared_ptr<Image>>& face) {
    debugAssert(face.size() == 6);
    const ImageFormat* format = face[0]->format();
    const int size = face[0]->width();
    for (int i = 0; i < face.size(); ++i) {
        debugAssertM((face[i]->width() == size) &&
                     (face[i]->height() == size) &&
                     (face[i]->format() == format),
            "Cube maps must use square faces with the same format");
    }

    // Construct the source images
    for (int f = 0; f < 6; ++f) {
        m_faceArray[f].setSize(size, size, format);
        // Copy the interior
//        m_faceArray[f].
    }

}


Color4 CubeMap::nearest(const Vector3& v) const {
    return Color4::zero();
}


Color4 CubeMap::bilinear(const Vector3& v) const {
    return Color4::zero();
}


int CubeMap::size() const {
    return m_faceArray[0].width() - 2;
}


const ImageFormat* CubeMap::format() const {
    return m_faceArray[0].format();
}

} // namespace
