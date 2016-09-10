/**
  \file GLG3D/TriTreeBase.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2009-06-10
  \edited  2016-09-09
*/

#include "G3D/AABox.h"
#include "G3D/CollisionDetection.h"
#include "GLG3D/TriTreeBase.h"
#include "GLG3D/Surface.h"

namespace G3D {

void TriTreeBase::clear() {
    m_triArray.fastClear();
    m_vertexArray.clear();
}


TriTreeBase::~TriTreeBase() {
    clear();
}


void TriTreeBase::setContents
(const Array<shared_ptr<Surface> >& surfaceArray, 
 ImageStorage                       newStorage) {

    const bool computePrevPosition = false;
    clear();
    Surface::getTris(surfaceArray, m_vertexArray, m_triArray, computePrevPosition);

    if (newStorage != IMAGE_STORAGE_CURRENT) {
        for (int i = 0; i < m_triArray.size(); ++i) {
            const Tri& tri = m_triArray[i];
            const shared_ptr<Material>& material = tri.material();
            if (notNull(material)) {
                material->setStorage(newStorage);
            }
        }
    }
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
        return m_triArray[hit.triIndex].sample(hit.u, hit.v, hit.triIndex, m_vertexArray, hit.backface);
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


shared_ptr<Surfel> TriTreeBase::sample(const Hit& hit) const {
    if (hit.triIndex != Hit::NONE) {
        return m_triArray[hit.triIndex].sample(hit.u, hit.v, hit.triIndex, m_vertexArray, hit.backface);
    } else {
        return nullptr;
    }
}

} // G3D