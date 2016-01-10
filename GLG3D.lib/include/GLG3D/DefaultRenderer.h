/**
   \file GLG3D/DefaultRenderer.h
 
   \maintainer Morgan McGuire, http://graphics.cs.williams.edu

   \created 2014-12-21
   \edited  2015-01-01

   Copyright 2000-2015, Morgan McGuire.
   All rights reserved.
*/
#ifndef GLG3D_DefaultRenderer_h
#define GLG3D_DefaultRenderer_h

#include "G3D/platform.h"
#include "GLG3D/Renderer.h"

namespace G3D {

class Camera;

/** \brief Supports both traditional forward shading and full-screen deferred shading.

    The basic rendering algorithm is:

\code
Renderer::render(all) {
    visible, requireForward, requireBlended = cullAndSort(all)
    renderGBuffer(visible)
    computeShadowing(all)
    if (deferredShading()) { renderDeferredShading()  }
    renderOpaqueSamples(deferredShading() ? requireForward : visible)
    lighting.updateColorImage() // For the next frame
    renderOpaqueScreenSpaceRefractingSamples(deferredShading() ? requireForward : visible)
    renderBlendedSamples(requireBlended, transparencyMode)
}
\endcode

    The DefaultRenderer::renderDeferredShading() pass uses whatever properties are available in the
    GBuffer, which are controlled by the GBufferSpecification. For most applications,
    it is necessary to enable the lambertian, glossy, camera-space normal,
    and emissive fields to produce good results. If the current GBuffer specification
    does not contain sufficient fields, most of the surfaces will take the fallback
    forward shading pass at reduced performance.

    \sa GApp::m_renderer, G3D::RenderDevice, G3D::Surface
*/
class DefaultRenderer : public Renderer {
protected:
    
    bool                        m_deferredShading;
    bool                        m_orderIndependentTransparency;

    /** For the transparent surface pass of the OIT algorithm.
        Shares the depth buffer with the main framebuffer. The
        subsequent compositing pass uses the regular framebuffer in 2D mode. 
        
        This framebuffer has several color render targets bound: 

        - RT0 = RGBA16F (accumulation.rgb, accumulation.a)
        - RT1 = R8 (revealage, -, -, -)
        - RT2 = original "background" framebuffer, modulated during rendering...or the modulation term product-accumulated against an initially white buffer

        It shares the depth with the original framebuffer but does not write to it.
       */
    shared_ptr<Framebuffer>     m_oitFramebuffer;

    String                      m_oitWriteDeclaration;

    virtual void renderDeferredShading
       (RenderDevice*                       rd, 
        const shared_ptr<GBuffer>&          gbuffer, 
        const LightingEnvironment&          environment);

    virtual void renderOpaqueSamples
       (RenderDevice*                       rd, 
        Array<shared_ptr<Surface> >&        surfaceArray, 
        const shared_ptr<GBuffer>&          gbuffer, 
        const LightingEnvironment&          environment);

    virtual void renderOpaqueScreenSpaceRefractingSamples
       (RenderDevice*                       rd, 
        Array<shared_ptr<Surface> >&        surfaceArray, 
        const shared_ptr<GBuffer>&          gbuffer, 
        const LightingEnvironment&          environment);

    virtual void renderSortedBlendedSamples       
        (RenderDevice*                      rd, 
        Array<shared_ptr<Surface> >&        surfaceArray, 
        const shared_ptr<GBuffer>&          gbuffer, 
        const LightingEnvironment&          environment);

    virtual void renderOrderIndependentBlendedSamples       
        (RenderDevice*                      rd, 
        Array<shared_ptr<Surface> >&        surfaceArray, 
        const shared_ptr<GBuffer>&          gbuffer, 
        const LightingEnvironment&          environment);

    DefaultRenderer();

public:

    static shared_ptr<Renderer> create() {
        return shared_ptr<DefaultRenderer>(new DefaultRenderer());
    }

    /** If true, use deferred shading on all surfaces that can be represented by the GBuffer.
        Default is false.
      */
    void setDeferredShading(bool b) {
        m_deferredShading = b;
    }

    bool deferredShading() const {
        return m_deferredShading;
    }

    /** If true, uses OIT.
        Default is false.
        
        The current implementation is based on:
        
        McGuire and Bavoil, Weighted Blended Order-Independent Transparency, Journal of Computer Graphics Techniques (JCGT), vol. 2, no. 2, 122--141, 2013
        Available online http://jcgt.org/published/0002/02/09/

        This can be turned on in both forward and deferred shading modes.

        This algorithm improves the quality of overlapping transparent surfaces for many scenes, eliminating popping and confusing appearance that can arise
        from imperfect sorting. It is especially helpful in scenes with lots of particles. This technique has relatively low overhead compared to alternative methods.
    */
    void setOrderIndependentTransparency(bool b) {
        m_orderIndependentTransparency = b;
    }

    bool orderIndependentTransparency() const {
        return m_orderIndependentTransparency;
    }

    /** Used by renderOrderIndependentBlendedSamples. The default implementation is 
        Weighted-Blended Order Independent Transparency by McGuire and Bavoil. This
        string can be overwritten to implement alternative algorithms, such as Adaptive
        Transparency. However, new buffers may need to be set by overriding
        renderOrderIndependentBlendedSamples() for certain algorithms. */
    void setOitWriteDeclaration(const String& s) {
        m_oitWriteDeclaration = s;
    }

    const String& oitWriteDeclaration() const {
        return m_oitWriteDeclaration;
    }

    virtual void render
       (RenderDevice*                       rd, 
        const shared_ptr<Framebuffer>&      framebuffer,
        const shared_ptr<Framebuffer>&      depthPeelFramebuffer,
        LightingEnvironment&                lightingEnvironment,
        const shared_ptr<GBuffer>&          gbuffer, 
        const Array<shared_ptr<Surface>>&   allSurfaces) override;
};

} // namespace
#endif
