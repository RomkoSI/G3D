#ifndef DemoScene_h
#define DemoScene_h

#include <G3D/G3DAll.h>

class DemoScene : public Scene {
protected:

    DemoScene(const shared_ptr<AmbientOcclusion>& ao) : Scene(ao) {}

public:

    static shared_ptr<DemoScene> create(const shared_ptr<AmbientOcclusion>& ao);
    
    void spawnAsteroids();
};

#endif

