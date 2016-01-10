
#include "GLG3D/VisualizeLightSurface.h"
#include "GLG3D/Light.h"
#include "GLG3D/Draw.h"
#include "GLG3D/RenderDevice.h"

namespace G3D {

VisualizeLightSurface::VisualizeLightSurface(const shared_ptr<Light>& c, bool showBounds) : 
    m_showBounds(showBounds), m_light(c) {}


shared_ptr<VisualizeLightSurface> VisualizeLightSurface::create(const shared_ptr<Light>& c, bool showBounds) {
    return shared_ptr<VisualizeLightSurface>(new VisualizeLightSurface(c, showBounds));
}


String VisualizeLightSurface::name() const {
    return m_light->name();
}


void VisualizeLightSurface::getCoordinateFrame(CoordinateFrame& cframe, bool previous) const {
    if (previous) {
        cframe = m_light->previousFrame();
    } else {
        cframe = m_light->frame();
    }
}


void VisualizeLightSurface::getObjectSpaceBoundingBox(AABox& box, bool previous) const {
    if (m_showBounds) {
        box = AABox::inf();
    } else {
        const float r = m_light->extent().length() / 2.0f;
        box = AABox(Point3(-r, -r, -r), Point3(r, r, r));
    }
}


void VisualizeLightSurface::getObjectSpaceBoundingSphere(Sphere& sphere, bool previous) const {
    if (m_showBounds) {
        sphere = Sphere(Point3::zero(), m_light->effectSphere().radius);
    } else {
        sphere = Sphere(Point3::zero(), m_light->extent().length() / 2.0f);
    }
}


void VisualizeLightSurface::render
   (RenderDevice*                           rd, 
    const LightingEnvironment&              environment,
    RenderPassType                          passType, 
    const String&                           singlePassBlendedOutputMacro) const {

    if (m_showBounds) {
        Draw::visualizeLightGeometry(m_light, rd);
    } else {
        Draw::light(m_light, rd);
    }
}


void VisualizeLightSurface::renderDepthOnlyHomogeneous
    (RenderDevice*                          rd, 
    const Array<shared_ptr<Surface> >&      surfaceArray,
    const shared_ptr<Texture>&              depthPeelTexture,
    const float                             depthPeelEpsilon,
    bool                                    requireBinaryAlpha,
    const Color3&                           transmissionWeight) const {
} 


void VisualizeLightSurface::renderWireframeHomogeneous
   (RenderDevice*                           rd, 
    const Array<shared_ptr<Surface> >&      surfaceArray, 
    const Color4&                           color,
    bool                                    previous) const {
    // Intentionally do not render in wireframe; nobody ever wants to see
    // how many polygons are on a debug visualization, so the caller probably
    // would like to see the REST of the scene in wireframe and the Lights
    // superimposed.
}

bool VisualizeLightSurface::canBeFullyRepresentedInGBuffer(const GBuffer::Specification& specification) const {
    return false;
}

} // G3D
