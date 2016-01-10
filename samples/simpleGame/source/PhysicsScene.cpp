#include "PhysicsScene.h"
#include "PlayerEntity.h"

shared_ptr<PhysicsScene> PhysicsScene::create(const shared_ptr<AmbientOcclusion>& ao) {
    return shared_ptr<PhysicsScene>(new PhysicsScene(ao));
}


 void PhysicsScene::poseExceptExcluded(Array<shared_ptr<Surface> >& surfaceArray, const String& excludedEntity) {
    for (int e = 0; e < m_entityArray.size(); ++e) {
        if (m_entityArray[e]->name() != excludedEntity) {
            m_entityArray[e]->onPose(surfaceArray);
        }
    }
}


Any PhysicsScene::toAny() const {
    Any a = Scene::toAny();

    Any physicsTable(Any::TABLE, "Physics");
    physicsTable.set("gravity", m_gravity);
    a["Physics"] = physicsTable;

    return a;
}


Any PhysicsScene::load(const String& sceneName) {
    Any resultAny = Scene::load(sceneName);
    
    m_gravity = Vector3(0, -5 * units::meters() / square(units::seconds()), 0);

    if ( resultAny.containsKey("Physics") ) {
        const Any& physics = resultAny["Physics"];
        AnyTableReader physicsTable(physics);
        physicsTable.getIfPresent("gravity", m_gravity);
    }


    // Set the initial positions
    Array<shared_ptr<Surface> > collisionSurfaces;
    for (int e = 0; e < m_entityArray.size(); ++e) {
        shared_ptr<VisibleEntity> entity = dynamic_pointer_cast<VisibleEntity>(m_entityArray[e]);
        if (notNull(entity) && ! entity->canChange()) {
            entity->onSimulation(0, 0);
            entity->onPose(collisionSurfaces);
        }
    }
    m_collisionTree.setContents(collisionSurfaces, IMAGE_STORAGE_CURRENT);



    return resultAny;
}



void PhysicsScene::staticIntersectSphere(const Sphere& sphere, Array<Tri>& triArray) const {
    m_collisionTree.intersectSphere(sphere, triArray);
}


void PhysicsScene::staticIntersectBox(const AABox& box, Array<Tri>& triArray) const {
    m_collisionTree.intersectBox(box, triArray);
}


bool PhysicsScene::staticIntersectRay(const Ray& ray, Tri::Intersector& intersectCallback, float& distance) const {
    return m_collisionTree.intersectRay(ray, intersectCallback, distance);
}
