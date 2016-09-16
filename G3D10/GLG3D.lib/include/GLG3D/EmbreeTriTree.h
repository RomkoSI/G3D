/**
  \file GLG3D/EmbreeTriTree.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2016-09-14
  \edited  2016-09-16

  G3D Innovation Engine
  Copyright 2000-2016, Morgan McGuire.
  All rights reserved.
*/
#pragma once

#include "G3D/platform.h"

#ifdef G3D_WINDOWS

#include "GLG3D/TriTreeBase.h"

struct RTCRay;
struct RTCIntersectContext;
struct RTCRayN;
struct RTCHitN;
struct __RTCScene;
struct __RTCDevice;

namespace G3D {

/** \brief a TriTreeBase that has high performance on multicore SIMD CPUs for ray-triangle intersection
    and a fast rebuild time. Unoptimized for box-triangle intersection.
*/
class EmbreeTriTree : public TriTreeBase {
public:
    using TriTreeBase::intersectRays;
    using TriTreeBase::intersectRay;

protected:


    class RTCTriangle { 
    public:
        /** Indices into m_vertexArray */
        int i0, i1, i2; 

        /** Index into m_triArray */
        int triIndex;
        RTCTriangle() : i0(0), i1(0), i2(0), triIndex(0) {}
        RTCTriangle(int i0, int i1, int i2, int t) : i0(i0), i1(i1), i2(i2), triIndex(t) {}
    };

    static void apiConvert(const Ray& ray, RTCRay& rtcRay);

    static void apiConvert(const RTCRay& rtcRay, int triIndex, TriTreeBase::Hit& hit);

    static void apiConvertOcclusion(const RTCRay& rtcRay, TriTreeBase::Hit& hit);

    Array<RTCTriangle>      m_opaqueTriangleArray;
    Array<RTCTriangle>      m_alphaTriangleArray;

    /** Mesh with no partial coverage. */
    unsigned int            m_opaqueGeomID;

    /** Mesh with partial coverage surfaces. */
    unsigned int            m_alphaGeomID;

    __RTCScene*             m_scene;
    __RTCDevice*            m_device;

public:    

    EmbreeTriTree();

    ~EmbreeTriTree();

    virtual void clear() override;

    virtual void rebuild() override;

private:

    class FilterAdapter {
        IntersectRayOptions options;

    public:

        FilterAdapter(IntersectRayOptions options);

        static void rtcFilterFuncN
           (int*                        valid, 
            void*                       userDataPtr, 
            const RTCIntersectContext*  context,
            RTCRayN*                    ray,
            const RTCHitN*              potentialHit,
            const size_t                N);
    };

public:

    virtual bool intersectRay
       (const Ray&                         ray,
        Hit&                               hit,
        IntersectRayOptions                options         = IntersectRayOptions(0)) const override;
    
    virtual void intersectRays
       (const Array<Ray>&                  rays,
        Array<Hit>&                        results,
        IntersectRayOptions                options         = IntersectRayOptions(0)) const override;
};

} // G3D

#endif // Windows
