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

    // Constants for specifying adjacency
    const int U = 0, V = 1;
    const int HI = 1, LO = -1;

    // Index of face on the left, axis to read from, and sign (in pixel coords)
    //                              +X               -X               +Y              -Y              +Z               -Z 
    const int left[]      = {CubeFace::NEG_Z, CubeFace::POS_Z, CubeFace::POS_X, CubeFace::POS_X, CubeFace::POS_X, CubeFace::NEG_X};
    const int leftAxis[]  = {        U,                U,               V,               V,            U,                U       };
    const int leftSign[]  = {        HI,               HI,              LO,              HI,           HI,               HI      };

    //                              +X               -X               +Y              -Y              +Z               -Z 
    const int right[]     = {CubeFace::POS_Z, CubeFace::NEG_Z, CubeFace::NEG_X, CubeFace::NEG_X, CubeFace::NEG_X, CubeFace::POS_X};
    const int rightAxis[] = {        U,                U,               V,               V,            U,                U       };
    const int rightSign[] = {        LO,               LO,              LO,              HI,           LO,               LO      };

    //                              +X               -X               +Y              -Y              +Z               -Z 
    const int top[]       = {CubeFace::POS_Y, CubeFace::POS_Y, CubeFace::NEG_Z, CubeFace::POS_Z, CubeFace::POS_Y, CubeFace::POS_Y};
    const int topAxis[]   = {        U,                U,               V,               V,            V,                V       };
    const int topSign[]   = {        LO,               HI,              LO,              HI,           LO,               HI      };

    //                              +X               -X               +Y              -Y              +Z               -Z 
    const int bottom[]    = {CubeFace::NEG_Y, CubeFace::NEG_Y, CubeFace::POS_Z, CubeFace::NEG_Z, CubeFace::NEG_Y, CubeFace::NEG_Y};
    const int bottomAxis[]= {        U,                U,               V,               V,            V,                V       };
    const int bottomSign[]= {        LO,               HI,              LO,              HI,           LO,               HI      };

    // Construct the source images
    for (int f = 0; f < 6; ++f) {

        Image& dst = m_faceArray[f];
        dst.setSize(m_iSize + 2, m_iSize + 2, format);
        // Copy the interior
        {
            const Image& src = *face[f].get();
            Thread::runConcurrently(Point2int32(0, 0), Point2int32(m_iSize, m_iSize), [&](Point2int32 P) {
                dst.set(P + Point2int32(1, 1), src.get<Color4>(P));
            });
        }

        // Copy left edge
        {
            const Image& src = *face[left[f]].get();
            const int fixedAxis     = leftAxis[f];
            const int iterationAxis = leftAxis[f];
            const int sign          = leftSign[f];
            Point2int32 P;
            P[fixedAxis] = (sign == HI) ? m_iSize - 1 : 0;
            for (int i = 0; i < m_iSize; ++i) {
                P[iterationAxis] = i;
                dst.set(m_iSize + 1, i + 1, src.get<Color4>(P));
            }
        }

        // Copy right edge
        {
            const Image& src = *face[right[f]].get();
            const int fixedAxis     = rightAxis[f];
            const int iterationAxis = rightAxis[f];
            const int sign          = rightSign[f];
            Point2int32 P;
            P[fixedAxis] = (sign == HI) ? m_iSize - 1 : 0;
            for (int i = 0; i < m_iSize; ++i) {
                P[iterationAxis] = i;
                dst.set(0, i + 1, src.get<Color4>(P));
            }
        }

        // Copy top edge
        {
            const Image& src  = *face[top[f]].get();
            const int fixedAxis     = topAxis[f];
            const int iterationAxis = topAxis[f];
            const int sign          = topSign[f];
            Point2int32 P;
            P[fixedAxis] = (sign == HI) ? m_iSize - 1 : 0;
            for (int i = 0; i < m_iSize; ++i) {
                P[iterationAxis] = i;
                dst.set(i + 1, 0, src.get<Color4>(P));
            }
        }

        // Copy bottom edge
        {
            const Image& src  = *face[bottom[f]].get();
            const int fixedAxis     = bottomAxis[f];
            const int iterationAxis = bottomAxis[f];
            const int sign          = bottomSign[f];
            Point2int32 P;
            P[fixedAxis] = (sign == HI) ? m_iSize - 1 : 0;
            for (int i = 0; i < m_iSize; ++i) {
                P[iterationAxis] = i;
                dst.set(i + 1, m_iSize + 1, src.get<Color4>(P));
            }
        }
    }

    // TODO: copy corners

    m_fSize = float(m_iSize);

}


Vector2 CubeMap::pixelCoord(const Vector3& vec, CubeFace& face) const {
    const Vector3::Axis faceAxis = vec.primaryAxis();
    face = (CubeFace)(int(faceAxis) * 2 + ((vec[faceAxis] < 0.0f) ? 1 : 0));

    // The other two axes
    const Vector3::Axis uAxis = Vector3::Axis((faceAxis + 1) % 3);
    const Vector3::Axis vAxis = Vector3::Axis((faceAxis + 2) % 3);

    // Texture coordinate, where (0, 0) is the upper left of the image
    Vector2 texCoord = 0.5f * Vector2(vec[uAxis], vec[vAxis]) / fabsf(vec[faceAxis]) + Vector2(0.5f, 0.5f);

    // Correct for OpenGL cube map rules
    switch (face) {
    case CubeFace::POS_X:
        texCoord = Vector2(1.0f - texCoord.y, 1.0f - texCoord.x);
        break;

    case CubeFace::NEG_X:
        texCoord = Vector2(texCoord.y, 1.0f - texCoord.x);
        break;

    case CubeFace::POS_Y:
        texCoord = Vector2(texCoord.y, texCoord.x);
        break;

    case CubeFace::NEG_Y:
        texCoord = Vector2(texCoord.y, 1.0f - texCoord.x);
        break;

    case CubeFace::POS_Z:
        texCoord = Vector2(texCoord.x, 1.0f - texCoord.y);
        break;

    case CubeFace::NEG_Z:
        texCoord = Vector2(1.0f - texCoord.x, 1.0f - texCoord.y);
        break;
    }

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
