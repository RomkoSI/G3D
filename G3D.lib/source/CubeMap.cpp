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
#include "G3D/Thread.h"
#include "G3D/CubeFace.h"

namespace G3D {

shared_ptr<CubeMap> CubeMap::create
   (const Array<shared_ptr<Image>>&     face,
    const Color4&                       readMultiplyFirst,
    const Color4&                       readAddSecond) {

    return createShared<CubeMap>(face, readMultiplyFirst, readAddSecond);
}


CubeMap::CubeMap
   (const Array<shared_ptr<Image>>&     face, 
    const Color4&                       readMultiplyFirst,
    const Color4&                       readAddSecond) : 
    m_readMultiplyFirst(readMultiplyFirst),
    m_readAddSecond(readAddSecond) {

    debugAssert(face.size() == 6);
    const ImageFormat* format = face[0]->format();
    m_iSize = face[0]->width();
    for (int i = 0; i < face.size(); ++i) {
        debugAssertM((face[i]->width() == m_iSize) &&
                     (face[i]->height() == m_iSize) &&
                     (face[i]->format() == format),
            "Cube maps must use square faces with the same format");
    }

    // Construct the source images
    for (int f = 0; f < 6; ++f) {

        const Image& src = *face[f].get();
        Image& dst = m_faceArray[f];

        dst.setSize(m_iSize + 2, m_iSize + 2, format);
        // Copy the interior

        Thread::runConcurrently(Point2int32(0, 0), Point2int32(m_iSize, m_iSize), [&](Point2int32 P) {
            dst.set(P + Point2int32(1, 1), src.get<Color4>(P));
        });
    }

    m_fSize = float(m_iSize);

    // TODO: copy edges and corners
}


Vector2 CubeMap::pixelCoord(const Vector3& vec, CubeFace& face) const {
    const Vector3::Axis faceAxis = vec.primaryAxis();
    face = (CubeFace)(int(faceAxis) * 2 + ((vec[faceAxis] < 0.0f) ? 1 : 0));

    // The other two axes
    const Vector3::Axis uAxis = Vector3::Axis((faceAxis + 1) % 3);
    const Vector3::Axis vAxis = Vector3::Axis((faceAxis + 2) % 3);

    const Vector2& texCoord = 0.5f * Vector2(vec[uAxis], vec[vAxis]) / fabsf(vec[faceAxis]) + Vector2(0.5f, 0.5f);

    return m_fSize * texCoord + Vector2::one();
}


Color4 CubeMap::nearest(const Vector3& vec) const {
    CubeFace face;
    const Vector2& P = pixelCoord(vec, face);
    return m_faceArray[face].nearest(P, WrapMode::CLAMP) * m_readMultiplyFirst + m_readAddSecond;
}


Color4 CubeMap::bilinear(const Vector3& vec) const {
    CubeFace face;
    const Vector2& P = pixelCoord(vec, face);
    return m_faceArray[face].bilinear(P, WrapMode::CLAMP) * m_readMultiplyFirst + m_readAddSecond;
}


int CubeMap::size() const {
    return m_iSize;
}


const ImageFormat* CubeMap::format() const {
    return m_faceArray[0].format();
}

} // namespace
