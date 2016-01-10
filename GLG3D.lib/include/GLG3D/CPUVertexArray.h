/**
 \file GLG3D/CPUVertexArray.h

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-07-22
 \edited  2012-10-03
 
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/

#ifndef GLG3D_CPUVertexArray_h
#define GLG3D_CPUVertexArray_h

#include "G3D/platform.h"
#include "G3D/Array.h"
#include "G3D/Vector2.h"
#include "G3D/Vector3.h"
#include "G3D/Vector4.h"
#include "G3D/Vector2unorm16.h"
#include "GLG3D/VertexBuffer.h"
#include "G3D/Vector4int32.h"

namespace G3D {

class AttributeArray;
class CoordinateFrame;

/** \brief Array of vertices with interlaced position, normal, texCoord, and tangent attributes.

\beta 

\sa G3D::Surface, G3D::UniversalSurface::CPUGeom, G3D::MeshAlg, G3D::Triangle, G3D::TriTree
*/

class CPUVertexArray {
private:
    //Intentionally unimplemented
    CPUVertexArray& operator=(const CPUVertexArray&); 

public:

    /** \brief Packed vertex attributes. 48 bytes per vertex.
    
    \sa Part::cpuVertexArray */
    class Vertex {
    public:
        /** Part-space position */
        Point3                  position;

        /** Part-space normal */
        Vector3                 normal;

        /** xyz = tangent, w = sign */
        Vector4                 tangent;

        /** Texture coordinate 0. */
        Point2                  texCoord0;

        void transformBy(const CoordinateFrame& cframe);
    };

    Array<Vertex>               vertex;

    /** A second texture coordinate (which is not necessarily stored in
        texture coordinate attribute 1 on a GPU).  This must be on [0,1].
        Typically used for light map coordinates. 
        
        This is stored outside of the CPUVertexArray::vertex array because 
        it is not used by most models. */
    Array<Point2unorm16>        texCoord1;

    /** Vertex Colors */
    Array<Color4>               vertexColors;

    /** 4 indices indicating the bones that affect animation for the vertex */
    Array<Vector4int32>         boneIndices;

    /** 4 floats indicating the weighting for the 4 bones that affect animation for the vertex */
    Array<Vector4>              boneWeights;

    /** The position of the vertex in the previous frame, in the same coordinate
        system as vertex.position.

        This is stored outside of the CPUVertexArray::vertex array because 
        it is not used by most models. */
    Array<Point3>               prevPosition;

    /** True if texCoord0 contains valid data. */
    bool                        hasTexCoord0;

    /** True if texCoord1 contains valid data. */
    bool                        hasTexCoord1;

    /** True if tangent contains valid data. */
    bool                        hasTangent;

    /** True if boneIndices and boneWeights contain valid data */
    bool                        hasBones;

    /** True if vertexColors contains valid data. */
    bool                        hasVertexColors;

    /** True if prevPosition is nonempty.  It is ambiguous if an empty
        vertex array has a prevPosition array. */
    bool hasPrevPosition() const {
        return prevPosition.size() > 0;
    }

    CPUVertexArray() : hasTexCoord0(true), hasTexCoord1(false), hasTangent(true), hasBones(false), hasVertexColors(false) {}

    explicit CPUVertexArray(const CPUVertexArray& otherArray);

    void transformAndAppend(const CPUVertexArray& otherArray, const CoordinateFrame& cframe);

    /** Transform otherArray.vertex.position by \a cframe and append
     to vertex.position.  Transform otherArray.vertex.position by \a
     prevCFrame and append to prevPosition.  Assumes that otherArray
     does not contain a prevPosition array of its own. */
    void transformAndAppend(const CPUVertexArray& otherArray, const CoordinateFrame& cframe, const CoordinateFrame& prevCFrame);

    int size() const {
        return vertex.size();
    }

    void copyFrom(const CPUVertexArray& other);

    /** \param texCoord1 This is not interleaved with the other data in GPU memory.
        \param vertexColors This is not interleaved with the other data in GPU memory. 
        */
    void copyToGPU
    (AttributeArray&               vertex, 
     AttributeArray&               normal, 
     AttributeArray&               packedTangent, 
     AttributeArray&               texCoord0,
     AttributeArray&               texCoord1,
     AttributeArray&               vertexColors,
     VertexBuffer::UsageHint    hint = VertexBuffer::WRITE_ONCE) const;

    
    /** \param texCoord1 This is not interleaved with the other data in GPU memory. 
        \param boneIndices, boneWeights These are only interleaved with each other in GPU memory
        \param vertexColors This is not interleaved with the other data in GPU memory.
        */
    void copyToGPU
    (AttributeArray&               vertex, 
     AttributeArray&               normal, 
     AttributeArray&               packedTangent, 
     AttributeArray&               texCoord0,
     AttributeArray&               texCoord1,  
     AttributeArray&               vertexColors,
     AttributeArray&               boneIndices,
     AttributeArray&               boneWeights,
     VertexBuffer::UsageHint    hint = VertexBuffer::WRITE_ONCE) const;

    void copyTexCoord0ToTexCoord1();

    void offsetAndScaleTexCoord1(const Point2& offset, const Point2& scale);
};

} // namespace G3D

#endif // GLG3D_CPUVertexArray
