/**
   \file GLG3D/Renderer.h
 
   \maintainer Morgan McGuire, http://graphics.cs.williams.edu

   \created 2014-12-03
   \edited  2015-01-03

   Copyright 2000-2015, Morgan McGuire.
   All rights reserved.
*/
#ifndef GLG3D_Renderer_h
#define GLG3D_Renderer_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "G3D/Array.h"

namespace G3D {

class RenderDevice;
class Framebuffer;
class GBuffer;
class Surface;
class Camera;
class LightingEnvironment;
class SceneVisualizationSettings;
class RenderPassType;

/** \brief Base class for 3D rendering pipelines. 
    \sa GApp::onGraphics3D */
class Renderer : public ReferenceCountedObject {
protected:

    enum Order { 
        /** Good for early depth culling */
        FRONT_TO_BACK,
        
        /** Good for painter's algorithm sorted transparency */
        BACK_TO_FRONT,

        /** Allows minimizing state changes by batching primitives */
        ARBITRARY 
    };

    /**
     \brief Appends to \a sortedVisibleSurfaces and \a forwardSurfaces.

     \para sortedVisibleSurfaces All surfaces visible to the GBuffer::camera(), sorted from back to front

     \param forwardOpaqueSurfaces Surfaces for which Surface::canBeFullyRepresentedInGBuffer() returned false.
     These require a forward pass in a deferred shader.
     (They may be capable of deferred shading for <i>some</i> pixels covered, e.g.,
      if the GBuffer did not contain a sufficient emissive channel.)

     \param forwardBlendedSurfaces Surfaces that returned true Surface::hasBlendedTransparency()
     because they require per-pixel blending at some locations 
    */
    virtual void cullAndSort
       (RenderDevice*                       rd, 
        const shared_ptr<GBuffer>&          gbuffer,
        const Array<shared_ptr<Surface>>&   allSurfaces, 
        Array<shared_ptr<Surface>>&         sortedVisibleSurfaces, 
        Array<shared_ptr<Surface>>&         forwardOpaqueSurfaces,
        Array<shared_ptr<Surface>>&         forwardBlendedSurfaces);

    /** 
       \brief Render z-prepass, depth peel, and G-buffer. 
       Called from render().

       \param gbuffer Must already have had GBuffer::prepare() called
       \param depthPeelFramebuffer Only rendered if notNull()
    */
    virtual void computeGBuffer
       (RenderDevice*                       rd,
        const Array<shared_ptr<Surface>>&   sortedVisibleSurfaces,
        const shared_ptr<GBuffer>&          gbuffer,
        const shared_ptr<Framebuffer>&      depthPeelFramebuffer,
        float                               depthPeelSeparationHint);

    /** \brief Compute ambient occlusion and direct illumination shadow maps. */
    virtual void computeShadowing
       (RenderDevice*                       rd,
        const Array<shared_ptr<Surface>>&   allSurfaces, 
        const shared_ptr<GBuffer>&          gbuffer,
        const shared_ptr<Framebuffer>&      depthPeelFramebuffer,
        LightingEnvironment&                lightingEnvironment);

    /** \brief Forward shade everything in \a surfaceArray.
        Called from render() 
        
        \param surfaceArray Visible surfaces sorted from front to back

      */
    virtual void forwardShade
       (RenderDevice*                       rd, 
        Array<shared_ptr<Surface> >&        surfaceArray, 
        const shared_ptr<GBuffer>&          gbuffer, 
        const LightingEnvironment&          environment,
        const RenderPassType&               renderPassType,
        const String&                       singlePassBlendedOutputMacro,
        Order                               order);

public:

    /** 
      The active camera and time interval are taken from the GBuffer.
      \param framebfufer Target color and depth framebuffer. Will be rendered in high dynamic range (HDR) linear radiance.
      \param gbuffer Must be allocated, sized, and prepared. Will be rendered according to its specification by this method.
      \param allSurfaces Surfaces not visible to the camera will automatically be culled
      \param depthPeelFramebuffer May be NULL
      \param lightingEnvironment Shadow maps will be updated for any lights that require them. AO will be updated if the ambientOcclusion field is non-NULL. Screen-space color buffer will be updated with textures the next frame.*/
    virtual void render
       (RenderDevice*                       rd, 
        const shared_ptr<Framebuffer>&      framebuffer,
        const shared_ptr<Framebuffer>&      depthPeelFramebuffer,
        LightingEnvironment&                lightingEnvironment,
        const shared_ptr<GBuffer>&          gbuffer, 
        const Array<shared_ptr<Surface>>&   allSurfaces) = 0;
};

} // namespace G3D
#endif
