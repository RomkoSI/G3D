#include "DemoScene.h"
#include "PlayerEntity.h"
#include "G3D/Random.h"

shared_ptr<DemoScene> DemoScene::create(const shared_ptr<AmbientOcclusion>& ao) {
    return shared_ptr<DemoScene>(new DemoScene(ao));
}


void DemoScene::spawnAsteroids() {

    // An example of how to spawn Entitys at runtime

    Random r(1023, false);
    for (int i = 0; i < 1000; ++i) {
        const String& modelName = format("asteroid%dModel", r.integer(0, 4));

        // Find a random position that is not too close to the space ship
        Point3 pos;
        while (pos.length() < 15) {
            for (int a = 0; a < 3; ++a) {
                pos[a] = r.uniform(-50.0f, 50.0f);
            }
        }

        const shared_ptr<VisibleEntity>& v = 
            VisibleEntity::create
            (format("asteroid%02d", i), 
             this,
             m_modelTable[modelName].resolve(),
             CFrame::fromXYZYPRDegrees(pos.x, pos.y, pos.z, r.uniform(0, 360), r.uniform(0, 360), r.uniform(0, 360)));

        // Don't serialize generated objects
        v->setShouldBeSaved(false);

        insert(v);
    }
}
