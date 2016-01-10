/**
 \file Viewer.h
 
 The base class for the more specialized Viewers
 
 \maintainer Morgan McGuire
 \author Eric Muller, Dan Fast, Katie Creel
 
 \created 2007-05-31
 \edited  2014-10-08
 */
#ifndef Viewer_h
#define Viewer_h

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "App.h"

class Viewer {
public:
    virtual ~Viewer() {}
    virtual void onInit(const G3D::String& filename) = 0;
    virtual bool onEvent(const GEvent& e, App* app) { return false; }
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {}
    virtual void onGraphics3D(RenderDevice* rd, App* app, const shared_ptr<LightingEnvironment>& lighting, Array<shared_ptr<Surface> >& surfaceArray) {};
    virtual void onGraphics2D(RenderDevice* rd, App* app) {}
    virtual void onPose(Array<shared_ptr<Surface> >& posed3D, Array<shared_ptr<Surface2D> >& posed2D) {};

};

#endif 
