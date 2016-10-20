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
#include "G3D/Image3.h"

namespace G3D {

class Vector3;
class Color4;
class Color3;
class CubeFace;

/** \brief A CPU seamless cube map. 
    \sa G3D:Image, G3D::Texture */
class CubeMap : public ReferenceCountedObject {
protected:

    Image3      m_faceArray[6];

    /** Size before padding */
    int         m_iSize;

    /** Size before padding */
    float       m_fSize;

    CubeMap(const Array<shared_ptr<Image3>>& face, const Color3& readMultiplyFirst, const Color3& readAddSecond);

    /** Returns a pixel coordinate in m_faceArray[face] */
    Vector2 pixelCoord(const Vector3& vec, CubeFace& face) const;

public:
    /** \param face All faces must have the same dimensions. 
        \param gamma Apply this gamma: L = p^gamma after reading back values (before bilinear interpolation, and before scale and bias)
      */
    static shared_ptr<CubeMap> create(const Array<shared_ptr<Image3>>& face, const Color3& readMultiplyFirst = Color3::one(), const Color3& readAddSecond = Color3::zero());
    Color3 nearest(const Vector3& v) const;
    Color3 bilinear(const Vector3& v) const;

    /** The size of one face, in pixels, based on the input (not counting padding used for seamless cube mapping */
    int size() const;

    const class ImageFormat* format() const;
};

} // G3D

