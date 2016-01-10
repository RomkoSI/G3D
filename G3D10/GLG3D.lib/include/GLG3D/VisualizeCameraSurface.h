#ifndef GLG3D_VisualizeCameraSurface_h
#define GLG3D_VisualizeCameraSurface_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "GLG3D/Surface.h"

namespace G3D {

class Camera;

/** 
  Displays a 3D representation of a Camera.

  Intended for debugging.
 */
class VisualizeCameraSurface : public Surface {
protected:
    shared_ptr<Camera>  m_camera;

    VisualizeCameraSurface(const shared_ptr<Camera>& c);

public:

    static shared_ptr<VisualizeCameraSurface> create(const shared_ptr<Camera>& c);

    virtual String name() const override;

    virtual bool anyOpaque() const override {
        return true;
    }

    virtual void getCoordinateFrame(CoordinateFrame& cframe, bool previous = false) const override;

    virtual void getObjectSpaceBoundingBox(AABox& box, bool previous = false) const override;

    virtual void getObjectSpaceBoundingSphere(Sphere& sphere, bool previous = false) const override;

    virtual void render
    (RenderDevice*                          rd, 
     const LightingEnvironment&             environment,
     RenderPassType                         passType, 
     const String&                          singlePassBlendedOutputMacro) const override;

    virtual void renderDepthOnlyHomogeneous
    (RenderDevice*                          rd, 
     const Array<shared_ptr<Surface> >&     surfaceArray,
     const shared_ptr<Texture>&             depthPeelTexture,
     const float                            depthPeelEpsilon,
     bool                                   requireBinaryAlpha,
     const Color3&                          transmissionWeight) const override;
    
    virtual void renderWireframeHomogeneous
    (RenderDevice*                          rd, 
     const Array<shared_ptr<Surface> >&     surfaceArray, 
     const Color4&                          color,
     bool                                   previous) const override;

    virtual bool canBeFullyRepresentedInGBuffer(const GBuffer::Specification& specification) const override;

    virtual bool requiresBlending() const override {
        return false;
    }
};

} // G3D

#endif
