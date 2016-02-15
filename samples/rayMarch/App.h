#pragma once

#include <G3D/G3DAll.h>
#include <GLG3DVR/VRApp.h>

typedef
    GApp 
    // VRApp
AppBase;

class App : public AppBase {
public:

    typedef AppBase super;
    
    App(const VRApp::Settings& settings = VRApp::Settings());

    virtual void onInit() override;
    virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface3D) override;
};
