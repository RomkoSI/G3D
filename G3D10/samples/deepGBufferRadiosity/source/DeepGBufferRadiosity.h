/**
 \file AmbientOcclusion.h
 \author Michael Mara, http://research.nvidia.com, http://illuminationcodified.com

 
 */
#ifndef DeepGBufferRadiosity_h
#define DeepGBufferRadiosity_h

#include <G3D/G3DAll.h>
#include "DeepGBufferRadiositySettings.h"
/**
 Implementation of Screen-Space Radiosity, based on 
 
 Lighting Deep G-Buffers:Single-Pass, Layered Depth Images
 with Minimum Separation Applied to Indirect Illumination.
 Michael Mara, Morgan McGuire, and David Luebke
*/
class DeepGBufferRadiosity {
    
protected:  

    class MipMappedBuffers {
    private:
        /** Stores camera-space (negative) linear z values at various scales in the MIP levels, packs  */
        shared_ptr<Texture>                     cszBuffer;
        shared_ptr<Texture>                     frontColorBuffer;
        shared_ptr<Texture>                     peeledColorBuffer;
        shared_ptr<Texture>                     normalBuffer;
        shared_ptr<Texture>                     peeledNormalBuffer;
        bool m_hasPeeledLayer;

        // buffer[i] is used for MIP level i
        Array< shared_ptr<Framebuffer> >         m_framebuffers;

        void initializeTextures(int width, int height, const ImageFormat* csZFormat, const ImageFormat* colorFormat, const ImageFormat* normalFormat);

        void computeMipMaps(RenderDevice* rd, bool hasPeeledLayer, bool useOct16Normals);

        void computeFullRes(RenderDevice* rd, const shared_ptr<Texture>& depthTexture, const shared_ptr<Texture>& frontColorLayer, const shared_ptr<Texture>& frontNormalLayer,
                const shared_ptr<Texture>& peeledDepthTexture, const shared_ptr<Texture>& peeledColorLayer,  const shared_ptr<Texture>& peeledNormalLayer, 
                const Vector3& clipInfo, bool useOct16Normals);

        
        void prepare(const shared_ptr<Texture>& frontDepthLayer, const shared_ptr<Texture>& frontColorLayer, const shared_ptr<Texture>& frontNormalLayer,
                    const shared_ptr<Texture>& peeledDepthLayer, const shared_ptr<Texture>& peeledColorLayer, const shared_ptr<Texture>& peeledNormalLayer, 
                    bool useOct16Normals, bool useHalfPrecisionColors);
        
        void resize(int w, int h) {
            cszBuffer->resize(w, h);
            frontColorBuffer->resize(w, h);
            peeledColorBuffer->resize(w, h);
            normalBuffer->resize(w, h);
            peeledNormalBuffer->resize(w, h);
        } 

    public:

        void compute
           (RenderDevice*               rd, 
            const shared_ptr<Texture>&  depthTexture,
            const shared_ptr<Texture>&  frontColorLayer,
            const shared_ptr<Texture>&  frontNormalLayer,
            const shared_ptr<Texture>&  peeledDepthTexture, 
            const shared_ptr<Texture>&  peeledColorLayer, 
            const shared_ptr<Texture>&  peeledNormalLayer,  
            const Vector3&              clipInfo,
            bool                        useOct16Normals,
            bool                        useHalfPrecisionColors);

        const shared_ptr<Texture>& csz() const {
            return cszBuffer;
        }

        const shared_ptr<Texture>& normals() const {
            return normalBuffer;
        }

        const shared_ptr<Texture>& peeledNormals() const {
            return peeledNormalBuffer;
        }

        void setArgs(Args& args) const;

        MipMappedBuffers(): m_hasPeeledLayer(false) {}
    };

    TemporalFilter                           m_temporalFilter;

    shared_ptr<Texture>                      m_temporallyFilteredResult;

    /** As of the last call to update.  This is either m_resultBuffer or Texture::white() */
    shared_ptr<Texture>                      m_texture;

    shared_ptr<Texture>                      m_peeledTexture;

    /** Ignore input outside of this region */
    int                                      m_inputGuardBandSize;

    /** Clip output to this region */
    int                                      m_outputGuardBandSize;

    MipMappedBuffers                         m_mipMappedBuffers;

    shared_ptr<Framebuffer>                  m_resultFramebuffer;
    /** RGBA, ao in A component */
    shared_ptr<Texture>                      m_resultBuffer;


    /** Has AO in A and lambertian indirect in RGB */
    shared_ptr<Texture>                      m_rawIIBuffer;
    shared_ptr<Framebuffer>                  m_rawIIFramebuffer;
    /** Has AO in A and lambertian indirect in RGB */
    shared_ptr<Texture>                      m_rawIIPeeledBuffer;

    /** Has AO in R and depth in G */
    shared_ptr<Texture>                      m_hBlurredBuffer;
    shared_ptr<Framebuffer>                  m_hBlurredFramebuffer;
    
    /** RGBA, ao in A component */
    shared_ptr<Texture>                      m_resultPeeledBuffer;
    shared_ptr<Framebuffer>                  m_resultPeeledFramebuffer;

    /** Has AO in R and depth in G */
    shared_ptr<Texture>                      m_hBlurredPeeledBuffer;
    shared_ptr<Framebuffer>                  m_hBlurredPeeledFramebuffer;

    /** \param width Total buffer size of the GBuffer, including the guard band */
    void resizeBuffers(const shared_ptr<Texture>& depthTexture, bool halfPrecisionColors);

    void computeRawII
       (RenderDevice*                       rd,
        const DeepGBufferRadiositySettings&          settings,
        const shared_ptr<Texture>&          depthBuffer,
        const Vector3&                      clipConstant,
        const Vector4&                      projConstant,
        const float                         projScale,
        const Matrix4&                      projectionMatrix,
        const MipMappedBuffers&             mipMappedBuffers,
        bool                                computePeeledLayer = false);
        

    /** normalBuffer is only used if settings.useNormalsInBlur is true and normalBuffer is non-null.
        projConstant is only used if settings.useNormalsInBlur is true and normalBuffer is null */
    void blurHorizontal
       (RenderDevice*                       rd, 
        const DeepGBufferRadiositySettings&          settings, 
        const Vector4&                      projConstant = Vector4::zero(),
        const shared_ptr<Texture>&          cszBuffer = shared_ptr<Texture>(),
        const shared_ptr<Texture>&          normalBuffer = shared_ptr<Texture>(),
        const shared_ptr<Texture>&          normalPeeledBuffer = shared_ptr<Texture>(),
        bool                                computePeeledLayer = false);

    /** normalBuffer is only used if settings.useNormalsInBlur is true and normalBuffer is non-null.
        projConstant is only used if settings.useNormalsInBlur is true and normalBuffer is null */
    void blurVertical
       (RenderDevice*                       rd, 
        const DeepGBufferRadiositySettings&          settings, 
        const Vector4&                      projConstant = Vector4::zero(),
        const shared_ptr<Texture>&          cszBuffer = shared_ptr<Texture>(),
        const shared_ptr<Texture>&          normalBuffer = shared_ptr<Texture>(),
        const shared_ptr<Texture>&          normalPeeledBuffer = shared_ptr<Texture>(),
        bool                                computePeeledLayer = false);

    /** Shared code for the vertical and horizontal blur passes */
    void blurOneDirection
       (RenderDevice*                       rd,
        const DeepGBufferRadiositySettings&          settings,
        const Vector4&                      projConstant,
        const shared_ptr<Texture>&          cszBuffer,
        const shared_ptr<Texture>&          normalBuffer,
        const Vector2int16&                 axis,
        const shared_ptr<Framebuffer>&      framebuffer,
        const shared_ptr<Texture>&          source,
        bool                                peeledLayer = false);

    /**

     \param rd The rendering device/graphics context.  The currently-bound framebuffer must
     match the dimensions of \a depthBuffer.

     \param radius Maximum obscurance radius in world-space scene units (e.g., meters, inches)

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
        (RenderDevice*                  rd,
        const DeepGBufferRadiositySettings&      settings,
        const shared_ptr<Texture>&      depthBuffer, 
        const shared_ptr<Texture>&      colorBuffer,
        const Vector3&                  clipConstant,
        const Vector4&                  projConstant,
        float                           projScale,
        const Matrix4&                  projectionMatrix,
        const shared_ptr<Texture>&      peeledDepthBuffer = shared_ptr<Texture>(),
        const shared_ptr<Texture>&      peeledColorBuffer = shared_ptr<Texture>(),
        const shared_ptr<Texture>&      normalBuffer = shared_ptr<Texture>(),
        const shared_ptr<Texture>&      peeledNormalBuffer  = shared_ptr<Texture>(),
        bool                            computePeeledLayer = false,
        const shared_ptr<GBuffer>&      gbuffer = shared_ptr<GBuffer>(),
        const shared_ptr<Scene>&        scene = shared_ptr<Scene>());

    /** \brief Convenience wrapper for the full version of compute() when
        using a camera. 

        \param camera The camera that the scene was rendered with.
    */
    void compute
       (RenderDevice*                   rd,
        const DeepGBufferRadiositySettings&      settings,
        const shared_ptr<Texture>&      depthBuffer, 
        const shared_ptr<Texture>&      colorBuffer,
        const shared_ptr<Camera>&       camera,
        const shared_ptr<Texture>&      peeledDepthBuffer = shared_ptr<Texture>(),
        const shared_ptr<Texture>&      peeledColorBuffer = shared_ptr<Texture>(),
        const shared_ptr<Texture>&      normalBuffer = shared_ptr<Texture>(),
        const shared_ptr<Texture>&      peeledNormalBuffer        = shared_ptr<Texture>(),
        bool                            computePeeledLayer = false,
        const shared_ptr<GBuffer>&      gbuffer = shared_ptr<GBuffer>(),
        const shared_ptr<Scene>&        scene = shared_ptr<Scene>());

    /** Returns a pointer to the raw results or the blurred buffer, depending on whether their was any blur */
    shared_ptr<Texture> getActualResultTexture(const DeepGBufferRadiositySettings& settings, bool peeled);

public:

    /** Returns the camera space linear z for layer 0 in R and for layer 1 in G */
    const shared_ptr<Texture>& packedCSZ() const {
        return m_mipMappedBuffers.csz();
    }

    /** \brief Create a new DeepGBufferRadiosity instance. 
    
        Only one is ever needed, but if you are rendering to differently-sized
        framebuffers it is faster to create one instance per resolution than to
        constantly force DeepGBufferRadiosity to resize its internal buffers. */
    static shared_ptr<DeepGBufferRadiosity> create();


    /** Convenience method for resizing the AO texture from aoFramebuffer to match the size of its depth buffer and then
        computing AO from the depth buffer.  
        
        \param inputGuardBandSize Number of pixels to trim from the input. Required to be the same in both dimensions and non-negative.
        \param outputGuardBandSize Number of pixels to trim from the input. Required to be the same in both dimensions and non-negative.
        \see texture
     */
    void update(RenderDevice*           rd, 
        const DeepGBufferRadiositySettings&      settings, 
        const shared_ptr<Camera>&       camera, 
        const shared_ptr<Texture>&      depthTexture,
        const shared_ptr<Texture>&      previousBounceBuffer,
        const shared_ptr<Texture>&      peeledDepthBuffer   = shared_ptr<Texture>(),
        const shared_ptr<Texture>&      peeledColorBuffer   = shared_ptr<Texture>(),
        const shared_ptr<Texture>&      normalBuffer        = shared_ptr<Texture>(),
        const shared_ptr<Texture>&      peeledNormalBuffer  = shared_ptr<Texture>(),
        const shared_ptr<Texture>&      lambertianBuffer    = shared_ptr<Texture>(),
        const shared_ptr<Texture>&      peeledLambertianBuffer = shared_ptr<Texture>(),
        const Vector2int16              inputGuardBandSize = Vector2int16(0, 0),
        const Vector2int16              outputGuardBandSize = Vector2int16(0, 0),
        const shared_ptr<GBuffer>&      gbuffer = shared_ptr<GBuffer>(),
        const shared_ptr<Scene>&        scene = shared_ptr<Scene>());    

    /** Convenience method that uses GBuffers */
    void update(RenderDevice*           rd, 
        const DeepGBufferRadiositySettings&      settings, 
        const shared_ptr<GBuffer>&      gBuffer,
        const shared_ptr<Texture>&      previousBounceBuffer,
        const shared_ptr<GBuffer>&      peeledGBuffer   = shared_ptr<GBuffer>(),
        const shared_ptr<Texture>&      peeledPreviousBounceBuffer = shared_ptr<Texture>(),
        const Vector2int16              inputGuardBandSize = Vector2int16(0, 0),
        const Vector2int16              outputGuardBandSize = Vector2int16(0, 0),
        const shared_ptr<Scene>         scene = shared_ptr<Scene>());  

    /**
       Returns the radiosity buffer texture, or Texture::white() if AO is disabled or 
       supported on this GPU. Modulate indirect illumination by this.
      */
    shared_ptr<Texture> texture() const {
        return m_texture;
    }

    shared_ptr<Texture> peeledTexture() const {
        return m_peeledTexture;
    }    
};


#endif // DeepGBufferRadiosity_h
