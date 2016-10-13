/**
  \file GLG3D/TriTreeBase.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2009-06-10
  \edited  2016-09-16
*/
#include "G3D/AABox.h"
#include "G3D/CollisionDetection.h"
#include "GLG3D/TriTreeBase.h"
#include "GLG3D/Surface.h"
#include "GLG3D/Scene.h"

namespace G3D {

void TriTreeBase::clear() {
    m_triArray.fastClear();
    m_vertexArray.clear();
}


TriTreeBase::~TriTreeBase() {
    clear();
}


void TriTreeBase::setContents
   (const shared_ptr<Scene>&            scene, 
    ImageStorage                        newStorage) {
    Array< shared_ptr<Surface> > surfaceArray;
    scene->onPose(surfaceArray);
    setContents(surfaceArray, newStorage);
}


void TriTreeBase::setContents
(const Array<shared_ptr<Surface> >& surfaceArray, 
 ImageStorage                       newStorage) {

    const bool computePrevPosition = false;
    clear();
    Surface::getTris(surfaceArray, m_vertexArray, m_triArray, computePrevPosition);
    Surface::setStorage(surfaceArray, newStorage);
    rebuild();
}


void TriTreeBase::setContents
   (const Array<Tri>&                  triArray, 
    const CPUVertexArray&              vertexArray,
    ImageStorage                       newStorage) {

    //const bool computePrevPosition = false;
    clear();
    m_triArray = triArray;
    m_vertexArray.copyFrom(vertexArray);   
    Tri::setStorage(m_triArray, newStorage);
    rebuild();
}


void TriTreeBase::intersectRays
   (const Array<Ray>&      rays,
    Array<Hit>&            results,
    IntersectRayOptions    options) const {

    results.resize(rays.size());
    Thread::runConcurrently(0, rays.size(), [&](int i) { intersectRay(rays[i], results[i], options); });
}


shared_ptr<Surfel> TriTreeBase::intersectRay
   (const Ray&             ray, 
    IntersectRayOptions    options,
    const Vector3&         directiondX,
    const Vector3&         directiondY) const {
 
    Hit hit;
    if (intersectRay(ray, hit, options)) {
        shared_ptr<Surfel> surfel;
        m_triArray[hit.triIndex].sample(hit.u, hit.v, hit.triIndex, m_vertexArray, hit.backface, surfel);
        return surfel;
    } else {
        return nullptr;
    }
}


void TriTreeBase::intersectBox
   (const AABox&           box,
    Array<Tri>&            results) const {

    results.fastClear();
    Spinlock resultLock;
    Thread::runConcurrently(0, m_triArray.size(), [&](int t) {
        const Tri& tri = m_triArray[t];
        if ((tri.area() > 0.0f) &&
            CollisionDetection::fixedSolidBoxIntersectsFixedTriangle(box, Triangle(tri.position(m_vertexArray, 0), 
                                                                                   tri.position(m_vertexArray, 1), 
                                                                                   tri.position(m_vertexArray, 2)))) {
            resultLock.lock();
            results.append(tri);
            resultLock.unlock();
        }
    });
}


void TriTreeBase::intersectSphere
   (const Sphere&                      sphere,
    Array<Tri>&                        triArray) const {

    AABox box;
    sphere.getBounds(box);
    intersectBox(box, triArray);

    // Iterate backwards because we're removing
    for (int i = triArray.size() - 1; i >= 0; --i) {
        const Tri& tri = triArray[i];
        if (! CollisionDetection::fixedSolidSphereIntersectsFixedTriangle(sphere, Triangle(tri.position(m_vertexArray, 0), tri.position(m_vertexArray, 1), tri.position(m_vertexArray, 2)))) {
            triArray.fastRemove(i);
        }
    }
}


void TriTreeBase::sample(const Hit& hit, shared_ptr<Surfel>& surfel) const {
    if (hit.triIndex != Hit::NONE) {
        m_triArray[hit.triIndex].sample(hit.u, hit.v, hit.triIndex, m_vertexArray, hit.backface, surfel);
    } else {
        surfel = nullptr;
    }
}


void TriTreeBase::intersectRays
    (const Array<Ray>&                 rays,
    Array<shared_ptr<Surfel>>&         results,
    IntersectRayOptions                options) const {

    Array<Hit> hits;
    results.resize(rays.size());
    intersectRays(rays, hits, options);

    const Hit* pHit = hits.getCArray();
    shared_ptr<Surfel>* pSurfel = results.getCArray();
    const Tri* pTri = m_triArray.getCArray();

	tbb::parallel_for(tbb::blocked_range<size_t>(0, hits.size(), 128), [&](const tbb::blocked_range<size_t>& r) {
		const size_t start = r.begin();
		const size_t end   = r.end();
		for (size_t i = start; i < end; ++i) {
            const Hit& hit = pHit[i];
            if (hit.triIndex != Hit::NONE) {
                pTri[hit.triIndex].sample(hit.u, hit.v, hit.triIndex, m_vertexArray, hit.backface, pSurfel[i]);
            } else {
                pSurfel[i] = nullptr;
            }
        }
    });

}


void TriTreeBase::intersectRays
    (const Array<Ray>&                 rays,
    Array<bool>&                       results,
    IntersectRayOptions                options) const {

    Array<Hit> hits;
    results.resize(rays.size());
    intersectRays(rays, hits, options);
    Thread::runConcurrently(0, rays.size(), [&](int i) {
        results[i] = (hits[i].triIndex != Hit::NONE);
    });
}

} // G3D