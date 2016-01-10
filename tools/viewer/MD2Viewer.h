/**
 @file MD2Viewer.h
 
 Viewer for Quake2 .md2 models
 
 \maintainer Morgan McGuire
 @author Eric Muller, Dan Fast, Katie Creel
 
 @created 2007-05-31
 @edited  2014-10-19
 */
#ifndef MD2VIEWER_H
#define MD2VIEWER_H

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Viewer.h"

class MD2Viewer : public Viewer {
private:
    shared_ptr<MD2Model>               model;
    Array<shared_ptr<Surface>>         posed;
    MD2Model::Pose                     currentPose;

    void pose(RealTime deltaTime);

public:
    virtual void onInit(const String& filename) override;
    virtual void onGraphics3D(RenderDevice* rd, App* app, const shared_ptr<LightingEnvironment>& lighting, Array<shared_ptr<Surface> >& surfaceArray) override;
};

#endif 
