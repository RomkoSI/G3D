#include "World.h"

World::World() : m_mode(TRACE) {
    ambient = Radiance3::fromARGB(0x304855) * 0.3f;
}


void World::begin() {
    debugAssert(m_mode == TRACE);
    m_surfaceArray.clear();
    m_mode = INSERT;
}


void World::insert(const shared_ptr<ArticulatedModel>& model, const CFrame& frame) {
    Array<shared_ptr<Surface> > posed;
    model->pose(posed, frame);
    for (int i = 0; i < posed.size(); ++i) {
        insert(posed[i]);
    }
}


void World::insert(const shared_ptr<Surface>& m) {
    debugAssert(m_mode == INSERT);
    m_surfaceArray.append(m);
}


void World::clearScene() {
    m_surfaceArray.clear();
    lightArray.clear();
}


void World::end() {
    m_triTree.setContents(m_surfaceArray);
    debugAssert(m_mode == INSERT);
    m_mode = TRACE;
}


bool World::lineOfSight(const Point3& P0, const Point3& P1) const {
    debugAssert(m_mode == TRACE);
    
    const Vector3& delta = P1 - P0;
    float distance = delta.length();
    const Ray& ray = Ray::fromOriginAndDirection(P0, delta / distance);

    TriTree::Hit ignore;
    return ! m_triTree.intersectRay(ray, distance, ignore, TriTree::RETURN_ANY_HIT | TriTree::TWO_SIDED_TRIANGLES);
}


shared_ptr<Surfel> World::intersect(const Ray& ray, float& distance) const {
    debugAssert(m_mode == TRACE);
    return m_triTree.intersectRay(ray, distance, 0);
}
