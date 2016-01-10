#ifndef App_h
#define App_h

#include <G3D/G3DAll.h>

class App : public GApp {
    shared_ptr<Renderer> m_otherRenderer;
protected:
    void makeGUI();

    void computeGBuffer(RenderDevice* rd, Array<shared_ptr<Surface> >& all);
    void computeShadows(RenderDevice* rd, Array<shared_ptr<Surface> >& all, LightingEnvironment& environment);
    void deferredShade(RenderDevice* rd, const LightingEnvironment& environment);
    void forwardShade(RenderDevice* rd, Array<shared_ptr<Surface> >& all, const LightingEnvironment& environment);

public:

    App(const GApp::Settings& settings = GApp::Settings());
    virtual void onInit() override;
};

#endif
