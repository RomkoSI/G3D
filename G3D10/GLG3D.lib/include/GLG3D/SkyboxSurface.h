/**
 \file GLG3D/SkyboxSurface.h

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2012-07-16
 \edited  2015-01-02
 
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */
#ifndef GLG3D_SkyboxSurface_h
#define GLG3D_SkyboxSurface_h

#include "GLG3D/Surface.h"
#include "GLG3D/Texture.h"
#include "GLG3D/Shader.h"
#include "GLG3D/AttributeArray.h"

namespace G3D {

/**
  \brief An infinite cube that simulates the appearance of distant objects in the scene.
 */
class SkyboxSurface : public Surface {
protected:

    static AttributeArray     s_cubeVertices;
    static IndexStream        s_cubeIndices;

    /** At alpha = 0, use m_texture0.  At alpha = 1, use m_texture1 */
    float                     m_alpha;

    /** If the textures are 2D, then they are passed as spherical-coordinate maps */
    shared_ptr<Texture>       m_texture0;
    shared_ptr<Texture>       m_texture1;

    SkyboxSurface(const shared_ptr<Texture>& texture0, const shared_ptr<Texture>& texture1, float alpha);

public:

    /** Directly creates a SkyboxSurface from a texture, without going through Skybox.
        If the textures are Texture::DIM_2D, then they are passed as spherical-coordinate maps. Otherwise they are assumed to be cube maps. */
    static shared_ptr<SkyboxSurface> create(const shared_ptr<Texture>& texture0, const shared_ptr<Texture>& texture1 = shared_ptr<Texture>(), float alpha = 0.0f);

    static void setShaderGeometryArgs(RenderDevice* rd, Args& args);

    virtual bool requiresBlending() const override {
        return false;
    }

    virtual bool anyUnblended() const override {
        return true;
    }

    void setTextureArgs(Args& args) const;

    const shared_ptr<Texture>& texture0() const {
        return m_texture0;
    }

    /** May be NULL */
    const shared_ptr<Texture>& texture1() const {
        return m_texture1;
    }

    float alpha() const {
        return m_alpha;
    }

    virtual bool canBeFullyRepresentedInGBuffer(const GBuffer::Specification& specification) const override;
    
    virtual String name() const override;

    virtual void getCoordinateFrame(CoordinateFrame& cframe, bool previous = false) const override;

    virtual void getObjectSpaceBoundingBox(AABox& box, bool previous = false) const override;

    virtual void getObjectSpaceBoundingSphere(Sphere& sphere, bool previous = false) const override;

    virtual bool castsShadows() const override;

    virtual void render
    (RenderDevice*                        rd, 
     const LightingEnvironment&           environment,
     RenderPassType                       passType, 
     const String&                        singlePassBlendedOutputMacro) const override;
   
    virtual void renderIntoGBufferHomogeneous
    (RenderDevice*                        rd,
     const Array<shared_ptr<Surface> >&   surfaceArray,
     const shared_ptr<GBuffer>&           gbuffer,
     const CFrame&                        previousCameraFrame,
     const CoordinateFrame&               expressivePreviousCameraFrame,
     const shared_ptr<Texture>&           depthPeelTexture,
     const float                          minZSeparation,
     const LightingEnvironment&           lightingEnvironment) const override;

    virtual void renderWireframeHomogeneous
    (RenderDevice*                        rd, 
     const Array<shared_ptr<Surface> >&   surfaceArray, 
     const Color4&                        color,
     bool                                 previous) const override;

    virtual void renderDepthOnlyHomogeneous
    (RenderDevice*                        rd, 
     const Array<shared_ptr<Surface> >&   surfaceArray,
     const shared_ptr<Texture>&           depthPeelTexture,
     const float                          depthPeelEpsilon,
     bool                                 requireBinaryAlpha,
     const Color3&                        transmissionWeight) const override;

}; // class SkyboxSurface

} // namespace G3D

#endif
