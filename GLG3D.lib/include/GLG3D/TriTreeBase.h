/**
  \file GLG3D/TriTreeBase.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2009-06-10
  \edited  2016-09-11
*/
#pragma once

#include <functional>
#include "G3D/platform.h"
#include "G3D/Vector3.h"
#include "G3D/Ray.h"
#include "G3D/Array.h"
#include "GLG3D/Tri.h"
#include "GLG3D/CPUVertexArray.h"
#ifndef _MSC_VER
#include <stdint.h>
#endif

namespace G3D {
class Surface;
class Surfel;
class Material;
class AABox;

/** Base class for ray-casting data structures. */
class TriTreeBase : public ReferenceCountedObject {
protected:

    Array<Tri>              m_triArray;
    CPUVertexArray          m_vertexArray;

public:

    /** CPU timing of API conversion overhead for the most recent call to intersectRays */
    mutable RealTime debugConversionOverheadTime = 0;

    /** Options for intersectRays. Default is full intersection with no backface culling optimization and partial coverage (alpha) test passing for values over 0.5. */
    typedef unsigned int IntersectRayOptions;

    /** Test for occlusion and do not necessarily return valid triIndex, backfacing, etc. data
        (useful for shadow rays and testing line of sight) */
    static const IntersectRayOptions OCCLUSION_TEST_ONLY = 1;

    /** Do not allow the intersector to perform backface culling as an
        optimization. Backface culling is not required in any case. */
    static const IntersectRayOptions DO_NOT_CULL_BACKFACES = 2;

    /** Only fail the partial coverage (alpha) test on zero coverage. */
    static const IntersectRayOptions PARTIAL_COVERAGE_THRESHOLD_ZERO = 4;

    /** Disable partial coverage (alpha) testing. */
    static const IntersectRayOptions NO_PARTIAL_COVERAGE_TEST = 8;

    /** Make optimizations appropriate for coherent rays (same origin) */
    static const IntersectRayOptions COHERENT_RAY_HINT = 16;
    
    class Hit {
    public:
        enum { NONE = -1 };
        /** NONE if no hit. For occlusion ray casts, this will be an undefined value not equal to NONE. */
        int         triIndex;
        float       u;
        float       v;
        float       distance;

        /** For occlusion ray casts, this will always be false. */
        bool        backface;

        Hit() : triIndex(NONE), u(0), v(0), distance(0), backface(false) {}
    };

    virtual ~TriTreeBase();

    virtual void clear();

    const Array<Tri>& triArray() const {
        return m_triArray;
    }

    const CPUVertexArray& vertexArray() const {
        return m_vertexArray;
    }

    /** If you mutate this, you must call rebuild() */
    Array<Tri>& triArray() {
        return m_triArray;
    }

    /** If you mutate this, you must call rebuild() */
    CPUVertexArray& vertexArray() {
        return m_vertexArray;
    }

    /** Array access to the stored Tris */
    const Tri& operator[](int i) const {
        debugAssert(i >= 0 && i < m_triArray.size());
        return m_triArray[i];
    }

    int size() const {
        return m_triArray.size();
    }

    /** Rebuil the tree after m_triArray or CPUVertexArray have been mutated. Called automatically by setContents() */
    virtual void rebuild() = 0;

    /** Base class implementation populates m_triArray and m_vertexArray and applies the image storage option. */
    virtual void setContents
        (const Array<shared_ptr<Surface>>&  surfaceArray, 
         ImageStorage                       newImageStorage = ImageStorage::COPY_TO_CPU);

    virtual void setContents
       (const Array<Tri>&                   triArray, 
        const CPUVertexArray&               vertexArray,
        ImageStorage                        newStorage = ImageStorage::COPY_TO_CPU);

    virtual void setContents
       (const shared_ptr<class Scene>&      scene, 
        ImageStorage                        newStorage = ImageStorage::COPY_TO_CPU);

    /** Helper function that samples materials. The default implementation calls the intersectRay
        override that takes a Hit and then samples from it.

        \param directiondX Reserved for future use
        \param directiondY Reserved for future use
     */
    virtual shared_ptr<Surfel> intersectRay
        (const Ray&                         ray, 
         IntersectRayOptions                options         = IntersectRayOptions(0),
         const Vector3&                     directiondX     = Vector3::zero(),
         const Vector3&                     directiondY     = Vector3::zero()) const;

    /** Intersect a single ray. Return value is `hit.triIndex != Hit::NONE` for convenience. 
        \param filterFunction Set to nullptr to accept any geometric ray-triangle instersection.
      */
    virtual bool intersectRay
        (const Ray&                         ray,
         Hit&                               hit,
         IntersectRayOptions                options         = IntersectRayOptions(0)) const = 0;

    /** Batch ray casting. The default implementation calls the single-ray version using
        Thread::runConcurrently. */
    virtual void intersectRays
        (const Array<Ray>&                  rays,
         Array<Hit>&                        results,
         IntersectRayOptions                options         = IntersectRayOptions(0)) const;

    virtual void intersectRays
        (const Array<Ray>&                  rays,
         Array<shared_ptr<Surfel>>&         results,
         IntersectRayOptions                options         = IntersectRayOptions(0)) const;

    virtual void intersectRays
        (const Array<Ray>&                  rays,
         Array<bool>&                       results,
         IntersectRayOptions                options         = IntersectRayOptions(0)) const;

    /** Returns all triangles that lie within the box. Default implementation
        tests each triangle in turn (linear time). */
    virtual void intersectBox
        (const AABox&                       box,
         Array<Tri>&                        results) const;

    /** Returns all triangles that intersect or are contained within
        the sphere (technically, this is a ball intersection).

        Default implementation calls intersectBox and then filters the results for the sphere.
     */
    virtual void intersectSphere
        (const Sphere&                      sphere,
         Array<Tri>&                        triArray) const;

    virtual void sample(const Hit& hit, shared_ptr<Surfel>& surfel) const;
};

} // G3D
