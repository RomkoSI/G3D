/**
  \file App.h
 */
#ifndef App_h
#define App_h
#include <G3D/G3DAll.h>
#include <GLG3DVR/VRApp.h>

/** Application framework. */
class App : public VRApp {
protected:
    
    /** Called from onInit */
    void makeGUI();

public:

    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit() override;

    virtual bool onEvent(const GEvent& e) override;

    // You can override onGraphics if you want more control over the rendering loop.
    // virtual void onGraphics(RenderDevice* rd, Array<shared_ptr<Surface> >& surface, Array<shared_ptr<Surface2D> >& surface2D) override;

    virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface3D) override;

    virtual void onAfterLoadScene(const Any &any, const String &sceneName) override;
    
};

#endif
