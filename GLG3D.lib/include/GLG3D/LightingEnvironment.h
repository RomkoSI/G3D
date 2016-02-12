/**
  \file GLG3D/LightingEnvironment.h
  
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2013-03-13
  \edited  2015-01-03
 */ 
#ifndef GLG3D_LightingEnvironment_h
#define GLG3D_LightingEnvironment_h

#include "G3D/platform.h"
#include "G3D/debugAssert.h"
#include "G3D/Array.h"
#include "GLG3D/Texture.h"
#include "GLG3D/AmbientOcclusionSettings.h"
#include "GLG3D/UniformTable.h"

namespace G3D {

class Light;
class AmbientOcclusion;
class Args;
class Any;

/** \brief Lighting environment (not just "environment map") intended for hardware rendering
    using screen-space approximations of indirect light.
 */
class LightingEnvironment {
protected:

    shared_ptr<Texture>             m_copiedScreenColorTexture;
    shared_ptr<Texture>             m_copiedScreenDepthTexture;

    /** \sa GBuffer:colorGuardBandThickness */
    Vector2int16                    m_copiedScreenColorGuardBand;

    /** \sa GBuffer:depthGuardBandThickness */
    Vector2int16                    m_copiedScreenDepthGuardBand;
    Any                             m_any;

    void  maybeCopyBuffers() const;

public:

    Array< shared_ptr<Light> >      lightArray;

    shared_ptr<AmbientOcclusion>    ambientOcclusion;
    AmbientOcclusionSettings        ambientOcclusionSettings;

    /** All environment map contributions are summed. Environment maps are scaled by a factor of PI when sampled
        because most environment maps are authored too dark, since legacy shaders often dropped that factor
        from the Lambertian denominator.
    */
    Array< shared_ptr<Texture> >    environmentMapArray;

    /** If the array is empty, all elements are treated as 1.0 */
    Array<float>                    environmentMapWeightArray;

    /** Additional arguments passed when setShaderArgs() is invoked
        (if notNull).  This helps when prototyping new shader-based
        effects that require new uniform arguments. This is a pointer
        because it may contiain significant state and many renderers
        clone the lighting environment. */
    shared_ptr<UniformTable>        uniformTable;

    LightingEnvironment();

    LightingEnvironment(const Any& any);

    Any toAny() const;

    /** Intended for scene editors */
    Any& sourceAny() {
        return m_any;
    }

    void copyScreenSpaceBuffers(const shared_ptr<Framebuffer>& framebuffer, const Vector2int16 colorGuardBand, const Vector2int16 depthGuardBand);

    /** An image of the color buffer.  This is a copy of the
        previous buffer; it is never the Texture currently being
        rendered to.

        Commonly used for screen-space reflection and refraction
        effects. 

        Returns the all-black texture if not currently allocated
        \beta
    */
    const shared_ptr<Texture>& screenColorTexture() const {
        if (isNull(m_copiedScreenColorTexture)) {
            return Texture::opaqueBlack();
        } else {
            return m_copiedScreenColorTexture;
        }
    }

    /** For screenColorTexture(). \sa GBuffer::colorGuardBandThickness */
    const Vector2int16 screenColorGuardBand() const {
        return m_copiedScreenColorGuardBand;
    }

    /** \sa GBuffer::depthGuardBandThickness */
    const Vector2int16 screenDepthGuardBand() const {
        return m_copiedScreenDepthGuardBand;
    }

    /** Used for screen-space reflection and refraction
        effects. 

        Currently NULL, likely to be replaced with a linear camera-space
        Z value in a future release.

        \beta

        \sa screenColorTexture
    */
    const shared_ptr<Texture>& screenDepthTexture() const {
        return m_copiedScreenDepthTexture;
    }
      
    int numShadowCastingLights() const;

    /** Appends onto the array */
    void getNonShadowCastingLights(Array<shared_ptr<Light> >& array) const;
    
	/** Appends onto the array */
    void getIndirectIlluminationProducingLights(Array<shared_ptr<Light> >& array) const;

    /** Changes the order of the remaining lights. */
    void removeShadowCastingLights();

    /** Binds:
    
     \code
      prefix+light$(I)_ ... [See G3D::Light::setShaderArgs]
      uniform samplerCube prefix+environmentMap$(J)_buffer;
      uniform float       prefix+environmentMap$(J)_scale;
      uniform float       prefix+environmentMap$(J)_glossyMIPConstant;
      prefix+ambientOcclusion_ ... [See G3D::AmbientOcclusion::setShaderArgs]
     \endcode
    */
    void setShaderArgs(UniformTable& args, const String& prefix = "") const;

    /** Creates a default lighting environment for demos, which uses the file
    on the noonclouds/noonclouds_*.jpg textures.  The code that it uses is below.  Note that this loads
    a cube map every time that it is invoked, so this should not be used within the rendering loop.
   */
    void setToDemoLightingEnvironment();
};

} // namespace G3D

#endif
