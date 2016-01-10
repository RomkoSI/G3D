/**
  \file GLG3D/ShadowMap.h

  \author Morgan McGuire, http://graphics.cs.williams.edu
 */
#ifndef G3D_ShadowMap_h
#define G3D_ShadowMap_h

#include "G3D/platform.h"
#include "G3D/CoordinateFrame.h"
#include "G3D/Matrix4.h"
#include "G3D/CullFace.h"
#include "G3D/AABox.h"
#include "G3D/Projection.h"
#include "G3D/ReferenceCount.h"
#include "GLG3D/Texture.h"
#include "GLG3D/Framebuffer.h"

namespace G3D {

class Surface;
class Light;
class Projection;
class RenderDevice;

/** 
 When rendering into a shadow map, SHADOW_MAP_FRAMEBUFFER is set to 1 in the depth shader
 by the ShadowMap's framebuffer.
*/
class ShadowMap : public ReferenceCountedObject {
public:
    struct VSMSettings {
        bool                    enabled;
        int                     filterRadius;
        /** Multiplier for the standard deviation of the Gaussian Blur */
        float                   blurMultiplier;
        int                     downsampleFactor;

        /** If >0, apply a light bleeding reduction function, that skews towards overdarkening. */
        float                   lightBleedReduction;
        VSMSettings(const Any& any);
        Any toAny() const;
        bool operator==(const VSMSettings& s) const;
        bool operator!=(const VSMSettings& s) const {
            return !(operator==(s));
        }

        VSMSettings() : enabled(false), filterRadius(5), blurMultiplier(1.0f), downsampleFactor(1), lightBleedReduction(0.0f) {}
    };
protected:
    String                  m_name;

    class Layer {
    public:
        String                  name;
        mutable RealTime        lastUpdateTime;
        shared_ptr<Texture>     depthTexture;
        shared_ptr<Framebuffer> framebuffer;

        /** Used to detect when something has been *removed*
            from this shadow map. This is zero if there
            is nothing in the shadow map. */
        size_t                  entityHash;

        Layer(const String& name) : name(name), lastUpdateTime(0.0f), entityHash(0) {}
        void setSize(Vector2int16 desiredSize);

        /** Called from ShadowMap::updateDepth
            \param initialValues If not NULL, blit the depth from this buffer instead of
             clearing to infinity. */
        void updateDepth
        (RenderDevice*                      renderDevice,
         ShadowMap*                         shadowMap,
         const Array<shared_ptr<Surface> >& shadowCaster,
         CullFace                           cullFace,
         const shared_ptr<Framebuffer>&     initialValues,
         bool                               stochastic, 
         const Color3&                      transmissionWeight);

        /** Calls the other method of the same name multiple times with appropriate culling faces and polygon offsets */
        void renderDepthOnly(class RenderDevice* renderDevice, const ShadowMap* shadowMap, const Array< shared_ptr<Surface> >& shadowCaster, CullFace cullFace, bool stochastic, const Color3& transmissionWeight) const;
        void renderDepthOnly(class RenderDevice* renderDevice, const ShadowMap* shadowMap, const Array< shared_ptr<Surface> >& shadowCaster, CullFace cullFace, float polygonOffset, bool stochastic, const Color3& transmissionWeight) const;
    };

    /** For Surface::canMove() == false surfaces, regardless of whether they actually 
        behave correctly and don't move. If the surfaces in here do move (e.g., because
        the scene is being edited), then the shadow map will render a little more slowly
        due to excessive updates but will still be correct. */
    Layer                   m_baseLayer;

    /** For Surface::canMove() == true surfaces */
    Layer                   m_dynamicLayer;

    Matrix4                 m_lightMVP;

    CoordinateFrame         m_lightFrame;

    /**
     Maps projected homogeneous coordinates to [0, 1]
     */
    Matrix4                 m_unitLightProjection;

    /** 
     Maps projected homogeneous coordinates to [-1, 1]
     */
    Matrix4                 m_lightProjection;
    Projection              m_projection;
    
    /**
     Maps projected homogeneous coordinates to [0, 1]
     */
    Matrix4                 m_unitLightMVP;

    float                   m_bias;

    float                   m_polygonOffset;
    float                   m_backfacePolygonOffset;

    bool                    m_stochastic;

    VSMSettings m_vsmSettings;

    shared_ptr<Framebuffer> m_varianceShadowMapFB;
    shared_ptr<Framebuffer> m_varianceShadowMapHBlurFB;
    shared_ptr<Framebuffer> m_varianceShadowMapFinalFB;

    class RenderDevice*     m_lastRenderDevice;

    ShadowMap(const String& name);

public:

    /** For debugging purposes. */
    const String& name() const {
        return m_name;
    }

    /** \brief Wall-clock time at which updateDepth() was last invoked. 
        Used to avoid recomputing shadow maps for static lights and objects.
        Calling a mutating method like setSize()
        resets this to zero.*/
    RealTime lastUpdateTime() const {
        return m_dynamicLayer.lastUpdateTime;
    }

    /** Distance in meters to pull surfaces forward towards the light (along the -z axis) during shadow map rendering. 
        This is typically on the scale of centimeters.  During shadow map testing, surfaces are pulled three times
        this distance along their surface normal, so that faces in direct light are not shadowed by themselves. */
    float bias() const {
        return m_bias;
    }

    /** 
      \copybrief bias
      Call before updateDepth.
      \sa bias
     */
    void setBias(float f) {
        m_bias = f;
    }

    /**
     Binds these parameters:
     \code
       #expect prefix_notNull "Defined if the shadow map is bound"
       uniform mat4 prefix_lightMVP;
       uniform float prefix_bias;             
     \endcode
      Plus all of the Texture::setShaderArgs values for prefix_depth
     */
    void setShaderArgsRead(class UniformTable& args, const String& prefix) const;

    /** The framebuffer used for rendering to the texture */
    const shared_ptr<Framebuffer>& framebuffer() const {
        return m_dynamicLayer.framebuffer;
    }

    /** 
    \brief Computes a reference frame (as a camera) and projection
    matrix for the light.
    
    \param lightFrame For a directional light, this is a finite reference
     frame.

    \param projection For a SPOT light, this is the perspective projection.

    \param lightProjX Scene bounds in the light's reference frame for
    a directional light.  Not needed for a spot light

    \param lightProjY Scene bounds in the light's reference frame for
    a directional light.  Not needed for a spot light

    \param lightProjNearMin Shadow map near plane depth in the light's
    reference frame for a directional light.  For a spot light, a
    larger value will be chosen if the method determines that it can
    safely do so.  For directional and point lights, this value is
    used directly.

    \param lightProjFarMax Shadow map far plane depth in the light's
    reference frame for a directional light.   For a spot light, a
    smaller value will be chosen if the method determines that it can
    safely do so.  For directional and point lights, this value is
    used directly.    

    \param intensityCutoff Don't bother shadowing objects that cannot be brighter than this value.
    Set to 0 to cast shadows as far as the entire scene.
    */
    static void computeMatrices
    (const shared_ptr<Light>& light, AABox shadowCasterBounds, CFrame& lightFrame, Projection& projection, Matrix4& lightProjectionMatrix,
     float lightProjX = 20, float lightProjY = 20, float lightProjNearMin = 0.3f, float lightProjFarMax = 500.0f,
     float intensityCutoff = 1/255.0f);

    /** Call with desiredSize = 0 to turn off shadow maps.
     */
    virtual void setSize(Vector2int16 desiredSize = Vector2int16(1024, 1024));

    static shared_ptr<ShadowMap> create
        (const String&              name        = "Shadow Map", 
         Vector2int16               desiredSize = Vector2int16(1024, 1024),
         bool                       stochastic  = false,
         const VSMSettings&         vsmSettings = VSMSettings());
    
    /** Increase to hide self-shadowing artifacts, decrease to avoid
        gap between shadow and object.  Default = 0.0.  
        
        \param b if nan(), the backface offset is set to s, otherwise it is set to b */
    inline void setPolygonOffset(float s, float b = nan()) {
        m_polygonOffset = s;
        if (isNaN(b)) {
            m_backfacePolygonOffset = s;
        } else {
            m_backfacePolygonOffset = b;
        }
    }

    /** Should transparency be handled stochastically? */
    bool stochastic() const {
        return m_stochastic;
    }

    bool useVarianceShadowMap() const {
        return m_vsmSettings.enabled;
    }

    inline float polygonOffset() const {
        return m_polygonOffset;
    }
    
    inline float backfacePolygonOffset() const {
        return m_backfacePolygonOffset;
    }

    /** MVP adjusted to map to [0,0], [1,1] texture coordinates. 
        
        Equal to unitLightProjection() * Matrix4::translation(0,0,-bias()) * lightFrame().inverse().

        This includes Y inversion, on the assumption that shadow maps are rendered
        to texture.
     */
    const Matrix4& unitLightMVP() const {
        return m_unitLightMVP;
    }

    /** The coordinate frame of the light source */
    const CoordinateFrame& lightFrame() const {
        return m_lightFrame;
    }

    /** Unit projection matrix for the light, biased to avoid self-shadowing.*/
    const Matrix4& unitLightProjection() const {
        return m_unitLightProjection;
    }

    /** Projection matrix for the light, biased to avoid self-shadowing.*/
    const Matrix4& lightProjection() const {
        return m_lightProjection;
    }

    const Projection& projection() const {
        return m_projection;
    }

    Projection& projection() {
        return m_projection;
    }

    bool enabled() const;

    /** 
    Assumes that the caller has ensured that shadowCaster contains only
    shadow-casting surfaces.

    \param cullFace Use NONE or BACK. Culling FRONT faces will create light leaks with our "normal offset" scheme.
    */
    virtual void updateDepth
    (class RenderDevice*           renderDevice, 
     const CoordinateFrame&        lightFrame,
     const Matrix4&                lightProjectionMatrix,
     const Array< shared_ptr<Surface> >& shadowCaster,
     CullFace                      cullFace  = CullFace::BACK,
     const Color3&                 transmissionWeight = Color3::white() / 3.0f); 

    /** Model-View-Projection matrix that maps world space to the
        shadow map pixels; used for rendering the shadow map itself.  Note that
        this maps XY to [-1,-1],[1,1]. Most applications will use biasedLightMVP
        to avoid self-shadowing problems. */
    const Matrix4& lightMVP() const {
        return m_lightMVP;
    }

    const shared_ptr<Texture>& depthTexture() const {
        return m_dynamicLayer.depthTexture;
    }

    const shared_ptr<Texture>& varianceShadowMap() const {
        static shared_ptr<Texture> nullTexture;
        return isNull(m_varianceShadowMapFinalFB) ? nullTexture : m_varianceShadowMapFinalFB->texture(0);
    }

    inline Rect2D rect2DBounds() const {
        return depthTexture()->rect2DBounds();
    }
};

} // G3D

#endif // G3D_ShadowMap_h
