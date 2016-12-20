/**
  @file App.h

  The G3D 10 default starter app is configured for OpenGL 4.1.
 */
#pragma once
#include <G3D/G3DAll.h>

class App : public GApp {
public:
    
    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit() override;
    virtual void onGraphics3D(RenderDevice* rd, Array< shared_ptr<Surface> >& surface) override;
    virtual void onGraphics2D(RenderDevice* rd, Array< shared_ptr<Surface2D> >& surface2D) override;
    virtual bool onEvent(const GEvent& e) override;
};
