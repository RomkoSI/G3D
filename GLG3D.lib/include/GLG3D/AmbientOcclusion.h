/**
 \file AmbientOcclusion.h
 \author Morgan McGuire and Michael Mara, NVIDIA Research

 Implementation of:
 
 Scalable Ambient Obscurance.
 Morgan McGuire, Michael Mara, and David Luebke, <i>HPG</i> 2012 
 
 AmbientOcclusion is an optimized variation of the "Alchemy AO" screen-space ambient obscurance algorithm. It is 3x-7x faster on NVIDIA GPUs 
 and easier to integrate than the original algorithm. The mathematical ideas were
 first described in McGuire, Osman, Bukowski, and Hennessy, The Alchemy Screen-Space Ambient Obscurance Algorithm, <i>HPG</i> 2011
 and were developed at Vicarious Visions.  

  Open Source under the "BSD" license: http://www.opensource.org/licenses/bsd-license.php

  Copyright (c) 2011-2012, NVIDIA
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */
#ifndef GLG3D_AmbientOcclusion_h
#define GLG3D_AmbientOcclusion_h

#include "G3D/platform.h"
#include "G3D/Array.h"
#include "G3D/Vector3.h"
#include "G3D/Vector4.h"
#include "G3D/ReferenceCount.h"
#include "GLG3D/TemporalFilter.h"


#define COMBINE_CSZ_INTO_ONE_TEXTURE 1

namespace G3D {

class RenderDevice;
class Camera;
class Texture;
class AmbientOcclusionSettings;
class Framebuffer;
class CoordinateFrame;

/**
 \brief Screen-space ambient obscurance.

  Create one instance of AmbientObscurance per viewport or Framebuffer rendered in the frame.  Otherwise
  every update() call will trigger significant texture reallocation.


 \author Morgan McGuire and Michael Mara, NVIDIA and Williams College, 
 http://research.nvidia.com, http://graphics.cs.williams.edu 
*/
class AmbientOcclusion : public ReferenceCountedObject {
protected:  

    class PerViewBuffers {
    public:
        /** Stores camera-space (negative) linear z values at various scales in the MIP levels */
        shared_ptr<Texture>                      cszBuffer;
        
        // buffer[i] is used for MIP level i
        Array< shared_ptr<Framebuffer> >         cszFramebuffers;
        void resizeBuffers(const String& name, shared_ptr<Texture>   texture, const shared_ptr<Texture>&   peeledTexture, const AmbientOcclusionSettings::ZStorage zStorage);
        static shared_ptr<PerViewBuffers> create();
        PerViewBuffers() {}
    };

    /** Used for debugging and visualization purposes */
    String                                   m_name;

    /** Prefix for the shaders. Default is "AmbientOcclusion_". This is useful when subclassing 
        AmbientOcclusion to avoid a conflict with the default shaders. */
    String                                   m_shaderFilenamePrefix;

    shared_ptr<Framebuffer>                  m_resultFramebuffer;
    shared_ptr<Texture>                      m_resultBuffer;

    /** As of the last call to update.  This is either m_resultBuffer or Texture::white() */
    shared_ptr<Texture>                      m_texture;

    int                                      m_guardBandSize;

    /** For now, can only be 1 or 2 in size */
    Array<shared_ptr<PerViewBuffers> >       m_perViewBuffers;

    /** Has AO in R and depth in G * 256 + B.*/
    shared_ptr<Texture>                      m_rawAOBuffer;
    shared_ptr<Framebuffer>                  m_rawAOFramebuffer;

    /** Has AO in R and depth in G * 256 + B.*/
    shared_ptr<Texture>                      m_temporallyFilteredBuffer;

    /** Has AO in R and depth in G */
    shared_ptr<Texture>                      m_hBlurredBuffer;
    shared_ptr<Framebuffer>                  m_hBlurredFramebuffer;

    /** If normals enabled, RGBA8, RG is CSZ, and BA is normal in Oct16 */
    shared_ptr<Framebuffer>                  m_packedKeyBuffer;

    TemporalFilter                           m_temporalFilter;

    /** Appended to all Args for shader passes run by this class. 
    
        Useful for prototyping minor variations; simply inherit from this class, 
        modify the shaders and add any new uniforms/macros required here.
        Note that because of the inherent slowness of iterating over hash tables,
        such a modification is not as performant as possible.  
        */
    shared_ptr<UniformTable>                 m_uniformTable;

    /** Creates the per view buffers if necessary */
    void initializePerViewBuffers(int size);

    void resizeBuffers(const shared_ptr<Texture>& depthTexture, bool packKey);

    void packBlurKeys
        (RenderDevice* rd,
            const AmbientOcclusionSettings& settings,
            const shared_ptr<Texture>& depthBuffer,
            const Vector3& clipInfo,
            const shared_ptr<Texture>& normalBuffer);

    void computeCSZ
       (RenderDevice*                       rd,   
        const Array<shared_ptr<Framebuffer> >& cszFramebuffers,
        const shared_ptr<Texture>&          csZBuffer,
        const AmbientOcclusionSettings&     settings, 
        const shared_ptr<Texture>&          depthBuffer, 
        const Vector3&                      clipInfo,
        const shared_ptr<Texture>&          peeledDepthBuffer);

    void computeRawAO
       (RenderDevice* rd,         
        const AmbientOcclusionSettings&     settings, 
        const shared_ptr<Texture>&          depthBuffer, 
        const Vector3&                      clipConstant,
        const Vector4&                      projConstant,
        float                               projScale,
        const shared_ptr<Texture>&          csZBuffer,
        const shared_ptr<Texture>&          peeledCSZBuffer = shared_ptr<Texture>(),
        const shared_ptr<Texture>&          normalBuffer = shared_ptr<Texture>());

    /** normalBuffer and normalReadScaleBias are only used if settings.useNormalsInBlur is true and normalBuffer is non-null.
        projConstant is only used if settings.useNormalsInBlur is true and normalBuffer is null */
    void blurHorizontal
        (RenderDevice*                      rd, 
        const AmbientOcclusionSettings&     settings, 
        const shared_ptr<Texture>&          depthBuffer,
        const Vector4&                      projConstant = Vector4::zero(),
        const shared_ptr<Texture>&          normalBuffer = shared_ptr<Texture>());

    /** normalBuffer and normalReadScaleBias are only used if settings.useNormalsInBlur is true and normalBuffer is non-null.
        projConstant is only used if settings.useNormalsInBlur is true and normalBuffer is null */
    void blurVertical
       (RenderDevice*                       rd, 
        const AmbientOcclusionSettings&     settings, 
        const shared_ptr<Texture>&          depthBuffer,
        const Vector4&                      projConstant = Vector4::zero(),
        const shared_ptr<Texture>&          normalBuffer = shared_ptr<Texture>());

    /** Shared code for the vertical and horizontal blur passes */
    void blurOneDirection
       (RenderDevice*                       rd,
        const AmbientOcclusionSettings&     settings,
        const shared_ptr<Texture>&          depthBuffer,
        const Vector4&                      projConstant,
        const shared_ptr<Texture>&          normalBuffer,
        const Vector2int16&                 axis,
        const shared_ptr<Framebuffer>&      framebuffer,
        const shared_ptr<Texture>&          source);

    /**
     \brief Render the obscurance constant at each pixel to the currently-bound framebuffer.

     \param rd The rendering device/graphics context.  The currently-bound framebuffer must
     match the dimensions of \a depthBuffer.

     \param settings See G3D::AmbientOcclusionSettings.

     \param depthBuffer Standard hyperbolic depth buffer.  Can
      be from either an infinite or finite far plane
      depending on the values in projConstant and clipConstant.

     \param clipConstant Constants based on clipping planes:
     \code
        const double width  = rd->width();
        const double height = rd->height();
        const double z_f    = camera->farPlaneZ();
        const double z_n    = camera->nearPlaneZ();

        const Vector3& clipConstant = 
            (z_f == -inf()) ? 
                Vector3(float(z_n), -1.0f, 1.0f) : 
                Vector3(float(z_n * z_f),  float(z_n - z_f),  float(z_f));
     \endcode

     \param projConstant Constants based on the projection matrix:
     \code
        Matrix4 P;
        camera->getProjectUnitMatrix(rd->viewport(), P);
        const Vector4 projConstant
            (float(-2.0 / (width * P[0][0])), 
             float(-2.0 / (height * P[1][1])),
             float((1.0 - (double)P[0][2]) / P[0][0]), 
             float((1.0 + (double)P[1][2]) / P[1][1]));
     \endcode

     \param projScale Pixels-per-meter at z=-1, e.g., computed by:
     
      \code
       -height / (2.0 * tan(verticalFieldOfView * 0.5))
      \endcode

     This is usually around 500.


     \param peeledDepthBuffer An optional peeled depth texture, rendered from the same viewpoint as the depthBuffer,
      but not necessarily with the same resolution.
     */
    void compute
       (RenderDevice*                   rd,
        const AmbientOcclusionSettings& settings,
        const shared_ptr<Texture>&      depthBuffer, 
        const Vector3&                  clipConstant,
        const Vector4&                  projConstant,
        float                           projScale,
        const CoordinateFrame&          currentCameraFrame,
        const CoordinateFrame&          prevCameraFrame,
        const shared_ptr<Texture>&      peeledDepthBuffer = shared_ptr<Texture>(),
        const shared_ptr<Texture>&      normalBuffer = shared_ptr<Texture>(),
        const shared_ptr<Texture>&      ssVelocityBuffer = shared_ptr<Texture>());

    /** \brief Convenience wrapper for the full version of compute().

        \param camera The camera that the scene was rendered with.
    */
    void compute
       (RenderDevice*                   rd,
        const AmbientOcclusionSettings& settings,
        const shared_ptr<Texture>&      depthBuffer, 
        const shared_ptr<Camera>&       camera,
        const shared_ptr<Texture>&      peeledDepthBuffer = shared_ptr<Texture>(),
        const shared_ptr<Texture>&      normalBuffer = shared_ptr<Texture>(),
        const shared_ptr<Texture>&      ssVelocityBuffer = shared_ptr<Texture>());

    AmbientOcclusion(const String& name) : m_name(name), m_shaderFilenamePrefix("AmbientOcclusion_") {}

public:

    /** For debugging and visualization purposes */
    const String& name() const {
        return m_name;
    }

    /** \brief Create a new AmbientOcclusion instance. 
    
        Only one is ever needed, but if you are rendering to differently-sized
        framebuffers it is faster to create one instance per resolution than to
        constantly force AmbientOcclusion to resize its internal buffers. */
    static shared_ptr<AmbientOcclusion> create(const String& name = "G3D::AmbientOcclusion");

    /** Convenience method for resizing the AO texture from aoFramebuffer to match the size of its depth buffer and then
        computing AO from the depth buffer.  
        
        \param guardBandSize Required to be the same in both dimensions and non-negative
        \see texture */
    void update(RenderDevice* rd, 
        const AmbientOcclusionSettings& settings, 
        const shared_ptr<Camera>&       camera, 
        const shared_ptr<Texture>&      depthTexture,
        const shared_ptr<Texture>&      peeledDepthBuffer   = shared_ptr<Texture>(),  
        const shared_ptr<Texture>&      normalBuffer        = shared_ptr<Texture>(),
        const shared_ptr<Texture>&      ssVelocityBuffer    = shared_ptr<Texture>(),
        const Vector2int16              guardBandSize = Vector2int16(0, 0));   

    /** Backwards compatible version of update, to avoid breaking projects */
    void G3D_DEPRECATED update(RenderDevice* rd, 
        const AmbientOcclusionSettings& settings, 
        const shared_ptr<Camera>&       camera, 
        const shared_ptr<Texture>&      depthTexture,
        const shared_ptr<Texture>&      peeledDepthBuffer,  
	    const shared_ptr<Texture>&      normalBuffer,
	    const Vector2int16              guardBandSize) {
        
      update(rd, settings, camera, depthTexture, peeledDepthBuffer, normalBuffer, shared_ptr<Texture>(), guardBandSize);
    }


    /**
       Returns the ao buffer texture, or Texture::white() if AO is disabled or 
       supported on this GPU. Modulate indirect illumination by this */
    shared_ptr<Texture> texture() const {
        return m_texture;
    }

    /**
        Binds:
            sampler2D   <prefix>##buffer;
            ivec2       <prefix>##offset;
            #define     <prefix>##notNull 1;
	to \a args.
    */
    void setShaderArgs(UniformTable& args, const String& prefix = "ambientOcclusion_", const Sampler& sampler = Sampler::buffer());

    /** Returns false if this graphics card is known to perform AmbientOcclusion abnormally slowly */
    static bool supported();

};

} // namespace GLG3D

#endif // AmbientOcclusion_h
