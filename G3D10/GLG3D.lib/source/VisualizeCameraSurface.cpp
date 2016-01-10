/**
  \file VisualizeCameraSurface.cpp
  
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2003-11-15
  \edited  2015-01-02

  Copyright 2000-2015, Morgan McGuire.
  All rights reserved. 
*/

#include "GLG3D/VisualizeCameraSurface.h"
#include "GLG3D/Camera.h"
#include "GLG3D/Draw.h"
#include "GLG3D/Light.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/LightingEnvironment.h"

namespace G3D {

VisualizeCameraSurface::VisualizeCameraSurface(const shared_ptr<Camera>& c) : m_camera(c) {}


shared_ptr<VisualizeCameraSurface> VisualizeCameraSurface::create(const shared_ptr<Camera>& c) {
    return shared_ptr<VisualizeCameraSurface>(new VisualizeCameraSurface(c));
}


String VisualizeCameraSurface::name() const {
    return m_camera->name();
}


void VisualizeCameraSurface::getCoordinateFrame(CoordinateFrame& cframe, bool previous) const {
    if (previous) {
        cframe = m_camera->previousFrame();
    } else {
        cframe = m_camera->frame();
    }
}


void VisualizeCameraSurface::getObjectSpaceBoundingBox(AABox& box, bool previous) const {
    box = AABox(Point3(-0.2, -0.2, -0.2), Point3(0.2, 0.2, 0.2));
}


void VisualizeCameraSurface::getObjectSpaceBoundingSphere(Sphere& sphere, bool previous) const {
    sphere = Sphere(Point3::zero(), 0.2f);
}


void VisualizeCameraSurface::render
   (RenderDevice*                           rd, 
    const LightingEnvironment&              environment,
    RenderPassType                          passType, 
    const String&                           singlePassBlendedOutputMacro) const {

    Draw::camera(m_camera, rd);

}


void VisualizeCameraSurface::renderDepthOnlyHomogeneous
    (RenderDevice*                          rd, 
    const Array<shared_ptr<Surface> >&      surfaceArray,
    const shared_ptr<Texture>&              depthPeelTexture,
    const float                             depthPeelEpsilon,
    bool                                    requireBinaryAlpha,
    const Color3&                           transmissionWeight) const {
    Draw::camera(m_camera, rd);
} 


void VisualizeCameraSurface::renderWireframeHomogeneous
   (RenderDevice*                           rd, 
    const Array<shared_ptr<Surface> >&      surfaceArray, 
    const Color4&                           color,
    bool                                    previous) const {
    // Intentionally do not render in wireframe; nobody ever wants to see
    // how many polygons are on a debug visualization, so the caller probably
    // would like to see the REST of the scene in wireframe and the cameras
    // superimposed.
}

bool VisualizeCameraSurface::canBeFullyRepresentedInGBuffer(const GBuffer::Specification& specification) const {
    return false;
}

} // G3D
