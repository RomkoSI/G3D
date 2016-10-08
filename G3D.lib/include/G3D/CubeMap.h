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

/** \brief A CPU seamless cube map. 
    \sa G3D:Image, G3D::Texture */
class CubeMap : public ReferenceCountedObject {
protected:
    Image m_faceArray[6];

    CubeMap(const Array<shared_ptr<Image>>& face);

public:
    /** \param face All faces must have the same dimensions. */
    static shared_ptr<CubeMap> create(const Array<shared_ptr<Image>>& face);
    Color4 nearest(const Vector3& v) const;
    Color4 bilinear(const Vector3& v) const;

    /** The size of one face, in pixels, based on the input (not counting padding used for seamless cube mapping */
    int size() const;

    const class ImageFormat* format() const;
};

} // G3D

