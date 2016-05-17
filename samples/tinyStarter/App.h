/**
  @file App.h

  The G3D 10 default starter app is configured for OpenGL 4.0.
 */
#ifndef App_h
#define App_h

#include <G3D/G3DAll.h>

class App : public GApp {
public:
    
    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit();
    virtual void onGraphics3D(RenderDevice* rd, Array< shared_ptr<Surface> >& surface);
    virtual void onGraphics2D(RenderDevice* rd, Array< shared_ptr<Surface2D> >& surface2D);

    virtual bool onEvent(const GEvent& e);
};

#endif
