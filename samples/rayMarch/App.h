/**
  \file App.h
 */
#ifndef App_h
#define App_h
#include <G3D/G3DAll.h>

class App : public GApp {
public:
    
    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit() override;
    virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface3D) override;
};

#endif
