/**
 G3D Innovation Engine
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */
#ifndef GLG3D_VisualizeLightSurface_h
#define GLG3D_VisualizeLightSurface_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "GLG3D/Surface.h"

namespace G3D {

class Light;

/** 
  Displays a 3D representation of a Light.

  Intended for debugging.
 */
class VisualizeLightSurface : public Surface {
protected:

    /** For shadow map */
    bool                m_showBounds;
    shared_ptr<Light>   m_light;

    VisualizeLightSurface(const shared_ptr<Light>& c, bool showBounds);

public:

    static shared_ptr<VisualizeLightSurface> create(const shared_ptr<Light>& c, bool showBounds = false);

    virtual bool hasTransmission() const override {
        // Force rendering in back-to-front order
        return true;
    }

    virtual String name() const override;

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

    virtual bool castsShadows() const override {
        // This means that the light itself does not cast shadows when other lights shine on it
        return false;
    }

    virtual bool anyOpaque() const override {
        return true;
    }

    virtual bool requiresBlending() const override {
        return true;
    }
};

} // G3D

#endif
