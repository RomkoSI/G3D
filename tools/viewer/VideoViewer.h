/**
 \file FontViewer.h
 
 Viewer for supported video files.
 
*/
#ifndef VideoViewer_h
#define VideoViewer_h

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Viewer.h"

class VideoViewer : public Viewer {
private:

    shared_ptr<VideoInput> m_video;
    shared_ptr<Texture>    m_videoTexture;
    int                    m_frame;

public:
	VideoViewer();
	virtual void onInit(const G3D::String& filename) override;
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override;
    virtual void onGraphics3D(RenderDevice* rd, App* app, const shared_ptr<LightingEnvironment>& lighting, Array<shared_ptr<Surface> >& surfaceArray) override {}
    virtual void onGraphics2D(RenderDevice* rd, App* app) override;

};

#endif 
