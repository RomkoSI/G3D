/**
   \file GLG3D/Renderer.cpp
 
   \maintainer Morgan McGuire, http://graphics.cs.williams.edu

   \created 2014-12-30
   \edited  2015-05-22

   Copyright 2000-2015, Morgan McGuire.
   All rights reserved.
*/
#include "G3D/typeutils.h"
#include "GLG3D/Renderer.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Framebuffer.h"
#include "GLG3D/LightingEnvironment.h"
#include "GLG3D/Camera.h"
#include "GLG3D/Surface.h"
#include "GLG3D/AmbientOcclusion.h"
#include "GLG3D/SkyboxSurface.h"

namespace G3D {

void Renderer::computeGBuffer
   (RenderDevice*                       rd,
    const Array<shared_ptr<Surface>>&   sortedVisibleSurfaces,
    const shared_ptr<GBuffer>&          gbuffer,
    const shared_ptr<Framebuffer>&      depthPeelFramebuffer,
    float                               depthPeelSeparationHint) {

    BEGIN_PROFILER_EVENT("Renderer::computeGBuffer");
    const shared_ptr<Camera>& camera = gbuffer->camera();

    Surface::renderIntoGBuffer(rd, sortedVisibleSurfaces, gbuffer, camera->previousFrame(), camera->expressivePreviousFrame());

    if (notNull(depthPeelFramebuffer)) {
        rd->pushState(depthPeelFramebuffer); {
            rd->clear();
            rd->setProjectionAndCameraMatrix(camera->projection(), camera->frame());
            Surface::renderDepthOnly(rd, sortedVisibleSurfaces, CullFace::BACK, gbuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL), depthPeelSeparationHint, true, Color3::white() / 3.0f);
        } rd->popState();
    }
    END_PROFILER_EVENT();
}


void Renderer::computeShadowing
   (RenderDevice*                       rd,
    const Array<shared_ptr<Surface>>&   allSurfaces, 
    const shared_ptr<GBuffer>&          gbuffer,
    const shared_ptr<Framebuffer>&      depthPeelFramebuffer,
    LightingEnvironment&                lightingEnvironment) {

    BEGIN_PROFILER_EVENT("Renderer::computeShadowing");
    // Compute shadows
    Surface::renderShadowMaps(rd, lightingEnvironment.lightArray, allSurfaces);

    if (! gbuffer->colorGuardBandThickness().isZero()) {
        rd->setGuardBandClip2D(gbuffer->colorGuardBandThickness());
    }        

    // Compute AO
    if (notNull(lightingEnvironment.ambientOcclusion)) {
        lightingEnvironment.ambientOcclusion->update
           (rd, 
            lightingEnvironment.ambientOcclusionSettings,
            gbuffer->camera(), gbuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL), 
            notNull(depthPeelFramebuffer) ? depthPeelFramebuffer->texture(Framebuffer::DEPTH) : shared_ptr<Texture>(), 
            gbuffer->texture(GBuffer::Field::CS_NORMAL), 
            gbuffer->texture(GBuffer::Field::SS_POSITION_CHANGE), 
            gbuffer->depthGuardBandThickness() - gbuffer->colorGuardBandThickness());
    }
    END_PROFILER_EVENT();
}


void Renderer::cullAndSort
   (RenderDevice*                       rd,
    const shared_ptr<GBuffer>&          gbuffer,
    const Array<shared_ptr<Surface>>&   allSurfaces, 
    Array<shared_ptr<Surface>>&         allVisibleSurfaces,
    Array<shared_ptr<Surface>>&         forwardOpaqueSurfaces,
    Array<shared_ptr<Surface>>&         forwardBlendedSurfaces) {

    BEGIN_PROFILER_EVENT("Renderer::cullAndSort");
    const shared_ptr<Camera>& camera = gbuffer->camera();
    Surface::cull(camera->frame(), camera->projection(), rd->viewport(), allSurfaces, allVisibleSurfaces);
    Surface::sortBackToFront(allVisibleSurfaces, camera->frame().lookVector());

    // Extract everything that uses a forward rendering pass (including the skybox, which is emissive
    // and benefits from a forward pass because it may have high dynamic range). Leave the skybox in the
    // deferred pass to produce correct motion vectors as well.
    for (int i = 0; i < allVisibleSurfaces.size(); ++i) {  
        const shared_ptr<Surface>& surface = allVisibleSurfaces[i];

        if (! surface->canBeFullyRepresentedInGBuffer(gbuffer->specification())) {
            if (surface->requiresBlending()) {
                forwardBlendedSurfaces.append(surface);
            } else {
                forwardOpaqueSurfaces.append(surface);
            }
        }
    }
    END_PROFILER_EVENT();
}


void Renderer::forwardShade
   (RenderDevice*                   rd, 
    Array<shared_ptr<Surface> >&    surfaceArray, 
    const shared_ptr<GBuffer>&      gbuffer, 
    const LightingEnvironment&      environment,
    const RenderPassType&           renderPassType, 
    const String&                   singlePassBlendedOutputMacro,
    Order                           order) {

    if (! gbuffer->colorGuardBandThickness().isZero()) {
        rd->setGuardBandClip2D(gbuffer->colorGuardBandThickness());
    }

    rd->setDepthWrite(renderPassType == RenderPassType::OPAQUE_SAMPLES);

    switch (order) {
    case FRONT_TO_BACK:
        // Render in the reverse order
        for (int i = surfaceArray.size() - 1; i >= 0; --i) {
            surfaceArray[i]->render(rd, environment, renderPassType, singlePassBlendedOutputMacro);
        }
        break;

    case BACK_TO_FRONT:
        // Render in the provided order
        for (int i = 0; i < surfaceArray.size(); ++i) {
            surfaceArray[i]->render(rd, environment, renderPassType, singlePassBlendedOutputMacro);
        }
        break;

    case ARBITRARY:
        {
            // Separate by type.  This preserves the sort order and ensures that the closest
            // object will still render first.
            Array< Array<shared_ptr<Surface> > > derivedTable;
            categorizeByDerivedType(surfaceArray, derivedTable);

            for (int t = 0; t < derivedTable.size(); ++t) {
                Array<shared_ptr<Surface> >& derivedArray = derivedTable[t];
                debugAssertM(derivedArray.size() > 0, "categorizeByDerivedType produced an empty subarray");

                derivedArray[0]->renderHomogeneous(rd, derivedArray, environment, renderPassType, singlePassBlendedOutputMacro);
            } // for each derived type
            break;
        } // ARBITRARY
    } // switch
}

} // namespace G3D
