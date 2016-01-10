#ifndef PhysicsScene_h
#define PhysicsScene_h

#include <G3D/G3DAll.h>

class PhysicsScene : public Scene {
protected:
    Vector3 m_gravity;

    /** Polygons of all non-dynamic entitys */
    TriTree                     m_collisionTree;

    PhysicsScene(const shared_ptr<AmbientOcclusion>& ao) : Scene(ao) {}

public:

    static shared_ptr<PhysicsScene> create(const shared_ptr<AmbientOcclusion>& ao);

    void poseExceptExcluded(Array<shared_ptr<Surface> >& surfaceArray, const String& excludedEntity);

    void setGravity(const Vector3& newGravity) {
        m_gravity = newGravity;
    }

    Vector3 gravity() const {
        return m_gravity;
    }

    /** Extend to read in physics properties */
    virtual Any load(const String& sceneName) override;

    /** Gets all static triangles within this world-space sphere. */
    void staticIntersectSphere(const Sphere& sphere, Array<Tri>& triArray) const;
    void staticIntersectBox(const AABox& box, Array<Tri>& triArray) const;
    bool staticIntersectRay(const Ray& ray, Tri::Intersector& intersectCallback, float& distance) const;

    const CPUVertexArray& getCPUVertexArrayOfCollisionTree() const {
        return m_collisionTree.cpuVertexArray();
    }

     Any toAny() const;

};

#endif

