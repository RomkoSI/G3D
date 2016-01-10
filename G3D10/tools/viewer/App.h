/**
 @file App.h
 
 App that allows viewing of 3D assets
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2010-02-26
 */
#ifndef APP_H
#define APP_H

#include "G3D/G3DAll.h"
#include "GLG3D/GLG3D.h"
class Viewer;

class App : public GApp {
private:
    shared_ptr<LightingEnvironment> lighting;
    Viewer*	                        viewer;
    String	                        filename;
    
public:
    virtual shared_ptr<Scene> scene() const {
        return GApp::scene();
    }

    /** Used by GUIViewer */
    Color4						    colorClear;
    
    const shared_ptr<GBuffer>& gbuffer() const {
        return m_gbuffer;
    }

    const shared_ptr<Framebuffer>& framebuffer() const {
        return m_framebuffer;
    }

    const shared_ptr<Framebuffer>& depthPeelFramebuffer() const {
        return m_depthPeelFramebuffer;
    }

    App(const GApp::Settings& settings = GApp::Settings(), const G3D::String& file = "");
    
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override;
    virtual void onInit() override;
    virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surfaceArray) override;
    virtual void onGraphics2D(RenderDevice *rd, Array< shared_ptr<Surface2D> > &surface2D) override;
    virtual void onCleanup() override;
    virtual bool onEvent(const GEvent& event) override;
    virtual void onPose(Array<shared_ptr<Surface> >& posed3D, Array<shared_ptr<Surface2D> >& posed2D) override;
    
    shared_ptr<AmbientOcclusion> ambientOcclusion() { return m_ambientOcclusion; }

private:
    /** Called from onInit() and after a FILE_DROP in onEvent()*/
    void setViewer(const G3D::String& newFilename);
};

#endif
