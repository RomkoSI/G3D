/**
  \file G3D/CubeMap.h 
  \maintainer Morgan McGuire

  Copyright 2000-2016, Morgan McGuire.
  All rights reserved.
 */
#pragma once

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "G3D/Array.h"
#include "G3D/Image.h"

namespace G3D {

class Vector3;
class Color4;
class CubeFace;

/** \brief A CPU seamless cube map. 
    \sa G3D:Image, G3D::Texture */
class CubeMap : public ReferenceCountedObject {
protected:

    Color4      m_readMultiplyFirst;
    Color4      m_readAddSecond;

    Image       m_faceArray[6];

    /** Size before padding */
    int         m_iSize;

    /** Size before padding */
    float       m_fSize;

    CubeMap(const Array<shared_ptr<Image>>& face, const Color4& readMultiplyFirst, const Color4& readAddSecond);

    /** Returns a pixel coordinate in m_faceArray[face] */
    Vector2 pixelCoord(const Vector3& vec, CubeFace& face) const;

public:
    /** \param face All faces must have the same dimensions. */
    static shared_ptr<CubeMap> create(const Array<shared_ptr<Image>>& face, const Color4& readMultiplyFirst = Color4::one(), const Color4& readAddSecond = Color4::zero());
    Color4 nearest(const Vector3& v) const;
    Color4 bilinear(const Vector3& v) const;

    /** The size of one face, in pixels, based on the input (not counting padding used for seamless cube mapping */
    int size() const;

    const class ImageFormat* format() const;
};

} // G3D

