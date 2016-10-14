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
    for (const shared_ptr<Surface>& surface : posed) {
        insert(surface);
    }
}


void World::insert(const shared_ptr<Surface>& m) {
    debugAssert(m_mode == INSERT);

    const shared_ptr<SkyboxSurface>& skybox = dynamic_pointer_cast<SkyboxSurface>(m);

    if (notNull(skybox)) {
        m_skybox = skybox->texture0()->toCubeMap();
    } else {
        m_surfaceArray.append(m);
    }
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
    const float len = delta.length();
    const Ray& ray = Ray::fromOriginAndDirection(P0, delta / len, 0.0f, len - 1e-3f);

    TriTree::Hit ignore;
    return ! m_triTree.intersectRay(ray, ignore, TriTree::OCCLUSION_TEST_ONLY | TriTree::DO_NOT_CULL_BACKFACES);
}


shared_ptr<Surfel> World::intersect(const Ray& ray) const {
    debugAssert(m_mode == TRACE);
    return m_triTree.intersectRay(ray);
}

