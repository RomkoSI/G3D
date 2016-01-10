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


bool World::lineOfSight(const Vector3& v0, const Vector3& v1) const {
    debugAssert(m_mode == TRACE);
    
    Vector3 d = v1 - v0;
    float len = d.length();
    Ray ray = Ray::fromOriginAndDirection(v0, d / len);
    float distance = len;
    Tri::Intersector intersector;

    // For shadow rays, try to find intersections as quickly as possible, rather
    // than solving for the first intersection
    static const bool exitOnAnyHit = true, twoSidedTest = true;
    return ! m_triTree.intersectRay(ray, intersector, distance, exitOnAnyHit, twoSidedTest);

}


shared_ptr<Surfel> World::intersect(const Ray& ray, float& distance) const {
    debugAssert(m_mode == TRACE);
    return m_triTree.intersectRay(ray, distance);
}
