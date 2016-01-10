/** \file App.h */
#ifndef App_h
#define App_h

#include <G3D/G3DAll.h>
#include "RayTracer.h"

/** Application framework. */
class App : public GApp {
protected:
    shared_ptr<RayTracer>  m_rayTracer;
    RayTracer::Settings    m_rayTracerSettings;
    RayTracer::Stats       m_rayTracerStats;
    GuiDropDownList*       m_resolutionList;

    bool                   m_showWireframe;
    bool                   m_showPhotons;
    bool                   m_showPhotonMap;

    /** Tracks the last time that the scene was updated in the ray tracer. */
    RealTime               m_lastSceneUpdateTime;
    RealTime               m_lastPhotonUpdateTime;

    /** Called from onInit */
    void makeGUI();

public:
    
    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit() override;

    virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& posed3D) override;
    
    void onRenderButton();
    void onResolutionChange();
};

/** Global pointer back to the app for showMessage calls */
extern App* app;

#endif
