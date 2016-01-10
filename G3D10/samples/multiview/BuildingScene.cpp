#include "BuildingScene.h"

using namespace G3D::units;

MyEntity::MyEntity() {}


shared_ptr<MyEntity> MyEntity::create(const CFrame& c, const shared_ptr<ArticulatedModel>& m) {
    shared_ptr<MyEntity> e(new MyEntity());
    e->frame = c;
    e->model = m;
    return e;
}


void MyEntity::onPose(Array<shared_ptr<Surface> >& surfaceArray) {
    model->pose(surfaceArray, frame);
}


shared_ptr<BuildingScene> BuildingScene::create() {
    const shared_ptr<BuildingScene> s(new BuildingScene());
    s->m_lighting.setToDemoLightingEnvironment();
    s->m_lighting.ambientOcclusionSettings.numSamples = 20;
    const shared_ptr<ArticulatedModel>& model = ArticulatedModel::create(Any::fromFile(System::findDataFile("model/crytek_sponza/sponza.ArticulatedModel.Any")));
    s->m_entityArray.append(MyEntity::create(Point3::zero(), model));

    return s;
}
    

void BuildingScene::onPose(Array<shared_ptr<Surface> >& surfaceArray) {
    for (int e = 0; e < m_entityArray.size(); ++e) {
        m_entityArray[e]->onPose(surfaceArray);
    }
}
