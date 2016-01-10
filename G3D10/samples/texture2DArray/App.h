#ifndef App_h
#define App_h

#include <G3D/G3DAll.h>

class App : public GApp {
protected:
    shared_ptr<Texture> waterArray;
    float time;
public:

    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit();
    virtual void onGraphics3D(RenderDevice* rd, Array< shared_ptr<Surface> >& surface);
    virtual void onGraphics2D(RenderDevice* rd, Array< shared_ptr<Surface2D> >& surface2D);

    virtual bool onEvent(const GEvent& e);
};

#endif