/**
   \file GLG3D/DefaultRenderer.cpp
 
   \maintainer Morgan McGuire, http://graphics.cs.williams.edu

   \created 2014-12-21
   \edited  2015-05-22

   Copyright 2000-2016, Morgan McGuire.
   All rights reserved.
*/
#include "GLG3D/DefaultRenderer.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Framebuffer.h"
#include "GLG3D/LightingEnvironment.h"
#include "GLG3D/Camera.h"
#include "GLG3D/Surface.h"
#include "GLG3D/AmbientOcclusion.h"
#include "GLG3D/SkyboxSurface.h"
#include "GLG3D/GApp.h"

namespace G3D {

DefaultRenderer::DefaultRenderer() : m_deferredShading(false), m_orderIndependentTransparency(false) {
    // The following string must not contain newlines because it is a giant macro definition
    m_oitWriteDeclaration = STR(
        layout(location = 0) out float4 _accum;
        layout(location = 1) out float  _revealage;
        layout(location = 2) out float3 _modulate;

        void writePixel(Radiance3 premultipliedReflectionAndEmission, float coverage, Color3 transmissionCoefficient, float collimation, float etaRatio, Point3 csPosition, Vector3 csNormal) { 
            /* Perform this operation before modifying the coverage to account for transmission */
            _modulate = coverage * (vec3(1.0) - transmissionCoefficient);

            /* Modulate the net coverage for composition by the transmission. This does not affect the color channels of the
                transparent surface because the caller's BSDF model should have already taken into account if transmission modulates
                reflection. See 

                    McGuire and Enderton, Colored Stochastic Shadow Maps, ACM I3D, February 2011
                    http://graphics.cs.williams.edu/papers/CSSM/

                for a full explanation and derivation.*/
            coverage *= 1.0 - (transmissionCoefficient.r + transmissionCoefficient.g + transmissionCoefficient.b) * (1.0 / 3.0);

            // Intermediate terms to be cubed
            float tmp = (coverage * 8.0 + 0.01) *
                        (-gl_FragCoord.z * 0.95 + 1.0);

            /* If a lot of the scene is close to the far plane, then gl_FragCoord.z does not 
                provide enough discrimination. Add this term to compensate:

                tmp /= sqrt(abs(csZ)); */

            float w    = clamp(tmp * tmp * tmp * 1e3, 1e-2, 3e2);
            _accum     = vec4(premultipliedReflectionAndEmission, coverage) * w;
            _revealage = coverage;
        });
}


void DefaultRenderer::render
   (RenderDevice*                       rd, 
    const shared_ptr<Framebuffer>&      framebuffer,
    const shared_ptr<Framebuffer>&      depthPeelFramebuffer,
    LightingEnvironment&                lightingEnvironment,
    const shared_ptr<GBuffer>&          gbuffer, 
    const Array<shared_ptr<Surface>>&   allSurfaces) {

    alwaysAssertM(! lightingEnvironment.ambientOcclusionSettings.enabled || notNull(lightingEnvironment.ambientOcclusion),
        "Ambient occlusion is enabled but no ambient occlusion object is bound to the lighting environment");

    const shared_ptr<Camera>& camera = gbuffer->camera();

    // Share the depth buffer with the forward-rendering pipeline
    framebuffer->set(Framebuffer::DEPTH, gbuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL));
    if (notNull(depthPeelFramebuffer)) {
        depthPeelFramebuffer->resize(framebuffer->width(), framebuffer->height());
    }

    // Cull and sort
    Array<shared_ptr<Surface> > sortedVisibleSurfaces, forwardOpaqueSurfaces, forwardBlendedSurfaces;
    cullAndSort(rd, gbuffer, allSurfaces, sortedVisibleSurfaces, forwardOpaqueSurfaces, forwardBlendedSurfaces);

    // Bind the main framebuffer
    rd->pushState(framebuffer); {
        rd->clear();
        rd->setProjectionAndCameraMatrix(camera->projection(), camera->frame());
        
        const bool needDepthPeel = lightingEnvironment.ambientOcclusionSettings.useDepthPeelBuffer && lightingEnvironment.ambientOcclusionSettings.enabled;
        computeGBuffer(rd, sortedVisibleSurfaces, gbuffer, needDepthPeel ? depthPeelFramebuffer : shared_ptr<Framebuffer>(), lightingEnvironment.ambientOcclusionSettings.depthPeelSeparationHint);

        // Shadowing + AO
        computeShadowing(rd, allSurfaces, gbuffer, depthPeelFramebuffer, lightingEnvironment);

        // Maybe launch deferred pass
        if (deferredShading()) {
            renderDeferredShading(rd, gbuffer, lightingEnvironment);
        }

        // Main forward pass
        renderOpaqueSamples(rd, deferredShading() ? forwardOpaqueSurfaces : sortedVisibleSurfaces, gbuffer, lightingEnvironment);

        // Prepare screen-space lighting for the *next* frame
        lightingEnvironment.copyScreenSpaceBuffers(framebuffer, gbuffer->colorGuardBandThickness());

        renderOpaqueScreenSpaceRefractingSamples(rd, deferredShading() ? forwardOpaqueSurfaces : sortedVisibleSurfaces, gbuffer, lightingEnvironment);

        // Samples that require blending
        if (m_orderIndependentTransparency) {
            renderOrderIndependentBlendedSamples(rd, forwardBlendedSurfaces, gbuffer, lightingEnvironment);
        } else {
            renderSortedBlendedSamples(rd, forwardBlendedSurfaces, gbuffer, lightingEnvironment);
        }
                
    } rd->popState();
}


void DefaultRenderer::renderDeferredShading(RenderDevice* rd, const shared_ptr<GBuffer>& gbuffer, const LightingEnvironment& environment) {
    // Make a pass over the screen, performing shading
    rd->push2D(); {
        rd->setGuardBandClip2D(gbuffer->colorGuardBandThickness());

        // Don't shade the skybox on this pass because it will be forward rendered
        rd->setDepthTest(RenderDevice::DEPTH_GREATER);
        Args args;

        environment.setShaderArgs(args);
        gbuffer->setShaderArgsRead(args, "gbuffer_");

        args.setRect(rd->viewport());

        LAUNCH_SHADER("DefaultRenderer_deferredShade.pix", args);
    } rd->pop2D();
}


void DefaultRenderer::renderOpaqueSamples
   (RenderDevice*                       rd, 
    Array<shared_ptr<Surface> >&        surfaceArray, 
    const shared_ptr<GBuffer>&          gbuffer, 
    const LightingEnvironment&          environment) {

    //screenPrintf("renderOpaqueSamples: %d", surfaceArray.length());
    BEGIN_PROFILER_EVENT("DefaultRenderer::renderOpaqueSamples");
    forwardShade(rd, surfaceArray, gbuffer, environment, RenderPassType::OPAQUE_SAMPLES, Surface::defaultWritePixelDeclaration(), ARBITRARY);
    END_PROFILER_EVENT();
}


void DefaultRenderer::renderOpaqueScreenSpaceRefractingSamples
   (RenderDevice*                       rd, 
    Array<shared_ptr<Surface> >&        surfaceArray, 
    const shared_ptr<GBuffer>&          gbuffer, 
    const LightingEnvironment&          environment) {

    BEGIN_PROFILER_EVENT("DefaultRenderer::renderOpaqueScreenSpaceRefractingSamples");
    //screenPrintf("renderOpaqueScreenSpaceRefractingSamples: %d", surfaceArray.length());
    forwardShade(rd, surfaceArray, gbuffer, environment, RenderPassType::UNBLENDED_SCREEN_SPACE_REFRACTION_SAMPLES, Surface::defaultWritePixelDeclaration(), ARBITRARY);
    END_PROFILER_EVENT();
}


void DefaultRenderer::renderSortedBlendedSamples       
   (RenderDevice*                       rd, 
    Array<shared_ptr<Surface> >&        surfaceArray, 
    const shared_ptr<GBuffer>&          gbuffer, 
    const LightingEnvironment&          environment) {

    // TODO: REMOVE THIS DEBUGGING CODE!
    // surfaceArray.reverse();

    BEGIN_PROFILER_EVENT("DefaultRenderer::renderSortedBlendedSamples");
    //screenPrintf("renderBlendedSamples: %d", surfaceArray.length());
    forwardShade(rd, surfaceArray, gbuffer, environment, RenderPassType::MULTIPASS_BLENDED_SAMPLES, Surface::defaultWritePixelDeclaration(), BACK_TO_FRONT);
    END_PROFILER_EVENT();
}


void DefaultRenderer::renderOrderIndependentBlendedSamples       
   (RenderDevice*                       rd, 
    Array<shared_ptr<Surface> >&        surfaceArray, 
    const shared_ptr<GBuffer>&          gbuffer, 
    const LightingEnvironment&          environment) {
    BEGIN_PROFILER_EVENT("DefaultRenderer::renderOrderIndependentBlendedSamples");
    if (surfaceArray.size() > 0) {

        //screenPrintf("renderOrderIndependentBlendedSamples: %d", surfaceArray.length());

        // Do we need to allocate the OIT buffers?
        if (isNull(m_oitFramebuffer)) {
            m_oitFramebuffer = Framebuffer::create("G3D::DefaultRenderer::m_oitFramebuffer");
            m_oitFramebuffer->set(Framebuffer::COLOR0, Texture::createEmpty("G3D::DefaultRenderer accum", rd->width(), rd->height(), ImageFormat::RGBA16F()));
		    m_oitFramebuffer->setClearValue(Framebuffer::COLOR0, Color4::zero());

            const shared_ptr<Texture>& texture = Texture::createEmpty("G3D::DefaultRenderer revealage", rd->width(), rd->height(), ImageFormat::R8());
            texture->visualization.channels = Texture::Visualization::RasL;
            m_oitFramebuffer->set(Framebuffer::COLOR1, texture);
		    m_oitFramebuffer->setClearValue(Framebuffer::COLOR1, Color4::one());

            // Buffer 2 reserved for the background colored modulation term
        }

        // Do we need to resize the OIT buffers?
        if ((m_oitFramebuffer->width() != rd->width()) ||
            (m_oitFramebuffer->height() != rd->height())) {
            m_oitFramebuffer->texture(Framebuffer::COLOR0)->resize(rd->width(), rd->height());
            m_oitFramebuffer->texture(Framebuffer::COLOR1)->resize(rd->width(), rd->height());
        }

        m_oitFramebuffer->set(Framebuffer::DEPTH, rd->drawFramebuffer()->texture(Framebuffer::DEPTH));

        ////////////////////////////////////////////////////////////////////////////////////
        //
        // 3D accumulation pass over transparent surfaces
        //
        
        const shared_ptr<Framebuffer>& oldBuffer = rd->drawFramebuffer();

        rd->setFramebuffer(m_oitFramebuffer);
		rd->clearFramebuffer(true, false);

        // After the clear, bind the color buffer from the main screen
        m_oitFramebuffer->set(Framebuffer::COLOR2, oldBuffer->texture(Framebuffer::COLOR0));
        rd->pushState(m_oitFramebuffer); {

            // Set blending modes
            rd->setBlendFunc(RenderDevice::BLEND_ONE,  RenderDevice::BLEND_ONE,                 RenderDevice::BLENDEQ_ADD, RenderDevice::BLENDEQ_SAME_AS_RGB, Framebuffer::COLOR0);
            rd->setBlendFunc(RenderDevice::BLEND_ZERO, RenderDevice::BLEND_ONE_MINUS_SRC_COLOR, RenderDevice::BLENDEQ_ADD, RenderDevice::BLENDEQ_SAME_AS_RGB, Framebuffer::COLOR1);
            rd->setBlendFunc(RenderDevice::BLEND_ZERO, RenderDevice::BLEND_ONE_MINUS_SRC_COLOR, RenderDevice::BLENDEQ_ADD, RenderDevice::BLENDEQ_SAME_AS_RGB, Framebuffer::COLOR2);

            forwardShade(rd, surfaceArray, gbuffer, environment, RenderPassType::SINGLE_PASS_UNORDERED_BLENDED_SAMPLES, m_oitWriteDeclaration, ARBITRARY);
        } rd->popState();

        // Remove the color buffer binding
        m_oitFramebuffer->set(Framebuffer::COLOR2, shared_ptr<Texture>());
        rd->setFramebuffer(oldBuffer);

        ////////////////////////////////////////////////////////////////////////////////////
        //
        // 2D compositing pass
        //

        rd->push2D(); {
            rd->setDepthTest(RenderDevice::DEPTH_ALWAYS_PASS);
            rd->setBlendFunc(RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA, RenderDevice::BLEND_ONE);
            Args args;
            args.setUniform("accumTexture",     m_oitFramebuffer->texture(0), Sampler::buffer());
            args.setUniform("revealageTexture", m_oitFramebuffer->texture(1), Sampler::buffer());
            args.setRect(rd->viewport());
            LAUNCH_SHADER("DefaultRenderer_compositeWeightedBlendedOIT.pix", args);
        } rd->pop2D();
    }

    END_PROFILER_EVENT();
}

} //namespace
