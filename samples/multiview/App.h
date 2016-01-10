#ifndef App_h
#define App_h

#include <G3D/G3DAll.h>
#include "BuildingScene.h"

class App : public GApp {
    shared_ptr<BuildingScene>  m_scene;

public:
    
    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit();
    virtual void onPose(Array<shared_ptr<Surface> >& posed3D, Array<shared_ptr<Surface2D> >& posed2D);
    virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface);
    virtual void onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D> >& surface2D);
};

#endif
