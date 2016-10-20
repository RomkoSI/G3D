/**
  \file CubeMap.cpp
  Copyright 2002-2016, Morgan McGuire

  \created 2002-05-27
  \edited  2016-10-07
 */

#include "G3D/CubeMap.h"
#include "G3D/Image3.h"
#include "G3D/Vector3.h"
#include "G3D/Color3.h"
#include "G3D/Color4.h"
#include "G3D/Thread.h"
#include "G3D/CubeFace.h"

namespace G3D {

shared_ptr<CubeMap> CubeMap::create
   (const Array<shared_ptr<Image3>>&    face,
    const Color3&                       readMultiplyFirst,
    const Color3&                       readAddSecond) {

    return createShared<CubeMap>(face, readMultiplyFirst, readAddSecond);
}


CubeMap::CubeMap
   (const Array<shared_ptr<Image3>>&    face,
    const Color3&                       readMultiplyFirst,
    const Color3&                       readAddSecond) {

    debugAssert(face.size() == 6);
    m_iSize = face[0]->width();
    for (int i = 0; i < face.size(); ++i) {
        debugAssertM((face[i]->width() == m_iSize) &&
                     (face[i]->height() == m_iSize),
            "Cube maps must use square faces with the same format");
    }

    // Constants for specifying adjacency
    const int U = 0, V = 1;
    const int HI = 1, LO = -1;

    // Index of face on the left, axis to read from, and sign (in pixel coords)
    //                              +X               -X               +Y              -Y              +Z               -Z 
    const int left[]      = {CubeFace::NEG_Z, CubeFace::POS_Z, CubeFace::POS_X, CubeFace::POS_X, CubeFace::POS_X, CubeFace::NEG_X};
    const int leftAxis[]  = {        U,                U,               V,               V,            U,                U       };
    const int leftSign[]  = {        LO,               LO,              HI,              HI,           LO,               LO      };

    //                              +X               -X               +Y              -Y              +Z               -Z 
    const int right[]     = {CubeFace::POS_Z, CubeFace::NEG_Z, CubeFace::NEG_X, CubeFace::NEG_X, CubeFace::NEG_X, CubeFace::POS_X};
    const int rightAxis[] = {        U,                U,               V,               V,            U,                U       };
    const int rightSign[] = {        HI,               HI,              HI,              HI,           HI,               HI      };

    // TODO: Top of +X and -Z are incorrect
    //                              +X               -X               +Y              -Y              +Z               -Z 
    const int top[]       = {CubeFace::POS_Y, CubeFace::POS_Y, CubeFace::NEG_Z, CubeFace::POS_Z, CubeFace::POS_Y, CubeFace::POS_Y};
    const int topAxis[]   = {        U,                U,               V,               V,            V,                V       };
    const int topSign[]   = {        LO,               LO,              HI,              HI,           HI,               LO      };

    //                              +X               -X               +Y              -Y              +Z               -Z 
    const int bottom[]    = {CubeFace::NEG_Y, CubeFace::NEG_Y, CubeFace::POS_Z, CubeFace::NEG_Z, CubeFace::NEG_Y, CubeFace::NEG_Y};
    const int bottomAxis[]= {        U,                U,               V,               V,            V,                V       };
    const int bottomSign[]= {        LO,               HI,              LO,              HI,           LO,               HI      };

    // Construct the source images
    for (int f = 0; f < 6; ++f) {

        Image3& dst = m_faceArray[f];
        dst.resize(m_iSize + 2, m_iSize + 2);
        // Copy the interior
        {
            const Image3& src = *face[f].get();
            Thread::runConcurrently(Point2int32(0, 0), Point2int32(m_iSize, m_iSize), [&](Point2int32 P) {
                dst.set(P.x + 1, P.y + 1, src.get(P.x, P.y) * readMultiplyFirst + readAddSecond);
            });
        }

        // Copy left edge
        {
            const Image3& src       = *face[left[f]].get();
            const int fixedAxis     = leftAxis[f];
            const int iterationAxis = (leftAxis[f] + 1) % 2;
            const int sign          = leftSign[f];
            Point2int32 P;
            P[fixedAxis] = (sign == HI) ? m_iSize - 1 : 0;
            for (int i = 0; i < m_iSize; ++i) {
                P[iterationAxis] = i;
                dst.set(m_iSize + 1, i + 1, src.get(P.x, P.y) * readMultiplyFirst + readAddSecond);
            }
        }

        // Copy right edge
        {
            const Image3& src       = *face[right[f]].get();
            const int fixedAxis     = rightAxis[f];
            const int iterationAxis = (rightAxis[f] + 1) % 2;
            const int sign          = rightSign[f];
            Point2int32 P;
            P[fixedAxis] = (sign == HI) ? m_iSize - 1 : 0;
            for (int i = 0; i < m_iSize; ++i) {
                P[iterationAxis] = i;
                dst.set(0, i + 1, src.get(P.x, P.y) * readMultiplyFirst + readAddSecond);
            }
        }

        // Copy top edge
        {
            const Image3& src       = *face[top[f]].get();
            const int fixedAxis     = topAxis[f];
            const int iterationAxis = (topAxis[f] + 1) % 2;
            const int sign          = topSign[f];
            Point2int32 P;
            P[fixedAxis] = (sign == HI) ? m_iSize - 1 : 0;
            for (int i = 0; i < m_iSize; ++i) {
                P[iterationAxis] = i;
                dst.set(i + 1, 0, src.get(P.x, P.y) * readMultiplyFirst + readAddSecond);
            }
        }

        // Copy bottom edge
        {
            const Image3& src       = *face[bottom[f]].get();
            const int fixedAxis     = bottomAxis[f];
            const int iterationAxis = (bottomAxis[f] + 1) % 2;
            const int sign          = bottomSign[f];
            Point2int32 P;
            P[fixedAxis] = (sign == HI) ? m_iSize - 1 : 0;
            for (int i = 0; i < m_iSize; ++i) {
                P[iterationAxis] =  i;
                dst.set(i + 1, m_iSize + 1, src.get(P.x, P.y) * readMultiplyFirst + readAddSecond);
            }
        }
    }

    // Implement corners by averaging adjacent row and column in linear space and re-gamma encoding.
    // This must run after the loop that sets border rows and columns.
    for (int f = 0; f < 6; ++f) {
        Image3& img = m_faceArray[f];
        Color3 a = img.get(0, 1);
        Color3 b = img.get(1, 0);
        img.set(0, 0, (a + b) * 0.5f);

        a = img.get(0, m_iSize);
        b = img.get(1, m_iSize + 1);
        img.set(0, m_iSize + 1, (a + b) * 0.5f);

        a = img.get(m_iSize + 1, m_iSize);
        b = img.get(m_iSize, m_iSize + 1);
        img.set(m_iSize + 1, m_iSize + 1, (a + b) * 0.5f);

        a = img.get(m_iSize + 1, 1);
        b = img.get(m_iSize, 0);
        img.set(m_iSize + 1, 0, (a + b) * 0.5f);
    }

    m_fSize = float(m_iSize);

    // For debugging wrapping
    //for (int i = 0; i < 6; ++i) { m_faceArray[i].save(G3D::format("%d.png", i)); }
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


Color3 CubeMap::nearest(const Vector3& vec) const {
    CubeFace face;
    const Vector2& P = pixelCoord(vec, face);
    return m_faceArray[face].nearest(P.x, P.y, WrapMode::CLAMP);
}


Color3 CubeMap::bilinear(const Vector3& vec) const {
    CubeFace face;
    const Vector2& P = pixelCoord(vec, face);
    return m_faceArray[face].bilinear(P, WrapMode::CLAMP);
}


int CubeMap::size() const {
    return m_iSize;
}


const ImageFormat* CubeMap::format() const {
    return m_faceArray[0].format();
}

} // namespace
