/**
 \file MD3Viewer.h
 
 \Viewer for Quake3 .md3 models
 
 \maintainer Daniel Evangelakos
 \author     Daniel Evangelakos 
 
 \created     June 18, 2013
 \edited      June 18, 2013
 */
#ifndef MD3Viewer_h
#define MD3Viewer_h

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Viewer.h"

class MD3Viewer : public Viewer {
private:
    shared_ptr<MD3Model>               model;
    MD3Model::Pose                     currentPose;
    shared_ptr<Texture>                m_skybox;
    CoordinateFrame                    cframe;

    void pose(RealTime deltaTime);

public:

    virtual void onInit(const String& filename) override;
    virtual void onPose(Array<shared_ptr<Surface> >& posed3D, Array<shared_ptr<Surface2D> >& posed2D) override;
    virtual void onGraphics3D(RenderDevice* rd, App* app, const shared_ptr<LightingEnvironment>& lighting, Array<shared_ptr<Surface> >& surfaceArray) override;
};

#endif 
