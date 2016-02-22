/**
  \file GLG3D/Light.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2003-11-12
  \edited  2016-02-22

  G3D Innovation Engine
  Copyright 2000-2016, Morgan McGuire.
  All rights reserved.
 */

#ifndef GLG3D_Light_h
#define GLG3D_Light_h

#include "G3D/platform.h"
#include "G3D/Vector4.h"
#include "G3D/Vector3.h"
#include "G3D/Color4.h"
#include "G3D/Array.h"
#include "G3D/ReferenceCount.h"
#include "G3D/CoordinateFrame.h"
#include "G3D/enumclass.h"
#include "G3D/Spline.h"
#include "GLG3D/Scene.h"
#include "GLG3D/Entity.h"
#include "GLG3D/ShadowMap.h"

namespace G3D {
class Any;
class Scene;

typedef Spline<Power3> Power3Spline;

/**
   \brief An (invisible) emitting surface (AREA) or point (DIRECTIONAL, SPOT, OMNI) light.

   The light "faces" along the negative-z axis of its frame(), like
   all other G3D objects.

   The light properties do not change when it is disabled (except for the enabled() value, of course).
   The caller is responsible for ensuring that lights are enabled when using them.

   For reading from an Any, the following fields are supported:
       
   \code
    Light {
        castsShadows = bool;
        shadowMapSize = Vector2int16(w, h);
        shadowMapBias = number;   // In meters, default is zero. Larger creates dark leaks, smaller creates light leaks
        shadowCullFace = cullface;  // may not be CURRENT
        enabled      = bool;
        spotSquare   = bool;
        attenuation  = [number number number];
        bulbPower    = Power3; (for a spot or omni light)
        bulbPowerTrack = Power3Spline { ... };
        radiance     = Power3; (for a directional light)
        type         = "DIRECTIONAL", "SPOT", "OMNI", or "AREA";
        spotHalfAngleDegrees = number;
        producesIndirectIllumination = boolean;
        producesDirectIllumination = boolean;

        nearPlaneZLimit = number; (negative)
        farPlaneZLimit = number; (negative)
    }
    \endcode

    plus all G3D::Entity fields. 


   A directional light has position.w == 0.  A spot light has
   spotHalfAngle < pi() / 2 and position.w == 1.  An omni light has
   spotHalfAngle == pi() and position.w == 1.  

   For a physically correct light, set attenuation = (0,0,1) for SPOT
   and OMNI lights (the default).  UniversalSurface ignores attenuation on
   directional lights, although in general it should be (1,0,0).
   
   \image html normaloffsetshadowmap.png
 */
class Light : public Entity {
public:

    class Type {
    public:
        enum Value {
            /**
             A "wall of lasers" approximating an infinitely distant, very bright
             SPOT light.  This provides constant incident radiance from a single 
             direction everywhere in the scene.

             Distance Attenuation is not meaningful on directional lights.
             */
            DIRECTIONAL,

            /**
              An omni-directional point light within a housing that only allows
              light to emerge in a cone (or frustum, if square).
             */
            SPOT, 

            /** 
              An omni-directional point light that emits in all directions.
              G3D does not provide built-in support shadow maps for omni lights.
            */
            OMNI,

            /** Reserved for future use. */
            AREA
        } value;
    
        static const char* toString(int i, Value& v) {
            static const char* str[] = {"DIRECTIONAL", "SPOT", "OMNI", "AREA", NULL};
            static const Value val[] = {DIRECTIONAL, SPOT, OMNI, AREA};             
            const char* s = str[i];
            if (s) {
                v = val[i];
            }
            return s;
        }

        G3D_DECLARE_ENUM_CLASS_METHODS(Type);
    };

protected:

    Type                m_type;

    /** Spotlight cutoff half-angle in <B>radians</B>.  pi() = no
        cutoff (point/dir).  Values less than pi()/2 = spot light */
    float               m_spotHalfAngle;

    /** If true, setShaderArgs will bind a spotHalfAngle large
        enough to encompass the entire square that bounds the cutoff
        angle. This produces a frustum instead of a cone of light
        when used with a G3D::ShadowMap.  for an unshadowed light this
        has no effect.*/
    bool                m_spotSquare;

    bool                m_castsShadows;

    bool                m_stochasticShadows;
    ShadowMap::VSMSettings m_varianceShadowSettings;

    CullFace            m_shadowCullFace;

    /** If false, this light is ignored */
    bool                m_enabled;

    /** 
     Optional shadow map.
    */
    shared_ptr<class ShadowMap> m_shadowMap;

    /** \copydoc Light::extent() */
    Vector2             m_extent;

    /** If set, this is used in onSimulation */
    Power3Spline        m_bulbPowerTrack;

    bool                m_producesIndirectIllumination;
    bool                m_producesDirectIllumination;
    
    float               m_nearPlaneZLimit;
    float               m_farPlaneZLimit;

public:

    /** The attenuation observed by an omni or spot light is 

        \f[ \frac{1}{4 \pi (a_0 + a_1 r + a_2 r^2)},\f]

        where \f$a_i\f$ <code>= attenuation[i]</code> and
        \f$r\f$ is the distance to the source.
    
        Directional lights ignore attenuation.  A physically correct
        light source should have $\f$a_0=0, a_1=0, a_2=1\f$, but it
        may be artistically desirable to alter the falloff function.

        To create a local light where the biradiance is equal to 
        the bulbPower with "no attenuation", use $\f$a_0=1/(4\pi), a_1=0, a_2=0\f$.
    */
    float               attenuation[3];

    /** 
        Point light: this is the the total power (\f$\Phi\f$) emitted
        uniformly over the sphere.  The incident normal irradiance at
        a point distance \f$r\f$ from the light is \f$ E_{\perp} =
        \frac{\Phi}{4 \pi r^2} \f$.
        
        Spot light: the power is the same as for a point light, but
        line of sight is zero outside the spot cone.  Thus the area
        within the spot cone does not change illumination when the
        cone shrinks.

        Directional light: this is the incident normal irradiance
        in the light's direction, \f$E_\perp\f$.
    */
    Color3              color;

    Light();

protected:
    
    /** Update m_frame's rotation from spotDirection and spotTarget. Called from factory methods
       to support the old API interface.
    */
    void computeFrame(const Vector3& spotDirection, const Vector3& rightDirection);

    void init(const String& name, AnyTableReader& propertyTable);

public:

    CullFace shadowCullFace() const {
        return m_shadowCullFace;
    }

    /** \param scene may be NULL */
    static shared_ptr<Entity> create
       (const String&               name, 
        Scene*                      scene,
        AnyTableReader&             propertyTable, 
        const ModelTable&           modelTable = ModelTable(),
        const Scene::LoadOptions&   options = Scene::LoadOptions());

    /** Is vector w_i (from a point in the scene to the light) within the field of view (e.g., spotlight cone) of this light?  Called from biradiance(). */
    bool inFieldOfView(const Vector3& w_i) const;

    Type type() const {
        return m_type;
    }

    void setSpotHalfAngle(float rad) {
        m_spotHalfAngle = rad;
    }

    /** Biradiance due to the entire emitter to point X, using the light's specified falloff and spotlight doors.*/
    Biradiance3 biradiance(const Point3& X) const;

    bool enabled() const {
        return m_enabled;
    }

    /** For a SPOT or OMNI light, the power of the bulb.  A SPOT light also has "barn doors" that
        absorb the light leaving in most directions, so their emittedPower() is less. 

        Useful for direct illumination.

        This is infinite for directional lights.
        \sa emittedPower */
    Power3 bulbPower() const;

    /** Position of the light's shadow map clipping plane along the light's z-axis.*/
    float nearPlaneZ() const;

    /** Position of the light's shadow map clipping plane along the light's z-axis.*/
    float farPlaneZ() const;

    /** Farthest that the farPlaneZ() is ever allowed to be (part of the Light's specification) */
    float farPlaneZLimit() const;

    /** Closest that the nearPlaneZ() is ever allowed to be (part of the Light's specification) */
    float nearPlaneZLimit() const;

    /**
       For a SPOT or OMNI light, the power leaving the light into the scene.  A SPOT light's
       "barn doors" absorb most of the light. (A real spot light has a reflector at the back so
       that the first half of the emitted light is not also lost, however this model is easier
       to use when specifying scenes.)

       Useful for photon emission.
       This is infinite for directional lights.
      \sa bulbPower
     */
    Power3 emittedPower() const;

    /** \brief Returns a unit vector selected uniformly at random within the world-space 
        solid angle of the emission cone, frustum, or sphere of the light source. For a directional 
        light, simply returns the light direction. */
    Vector3 randomEmissionDirection(class Random& rng) const;

    /** When this light is enabled, does it cast shadows? */
    bool castsShadows() const {
        return m_castsShadows;
    }

    /** Sends directional lights to infinity */
    virtual void setFrame(const CoordinateFrame& c) override;

    /** 
        \brief Homogeneous world space position of the center of the light source
        (for a DIRECTIONAL light, w = 0) 

        \sa extent(), frame()
    */
    Vector4 position() const {
        if (m_type == Type::DIRECTIONAL) {
            return Vector4(-m_frame.lookVector(), 0.0f);
        } else {
            return Vector4(m_frame.translation, 1.0f);
        }
    }

    /** Spot light cutoff half-angle in <B>radians</B>.  pi() = no
        cutoff (point/dir).  Values less than pi()/2 = spot light.

        A rectangular spot light circumscribes the cone of this angle.
        That is, spotHalfAngle() is the measure of the angle from 
        the center to each edge along the orthogonal axis.
    */
    float spotHalfAngle() const {
        return m_spotHalfAngle;
    }

    /** \deprecated \sa rectangular() */
    bool spotSquare() const {
        return m_spotSquare;
    }

    /** The translation of a DIRECTIONAL light is infinite.  While
        this is often inconvenient, that inconvenience is intended to
        force separate handling of directional sources.

        Use position() to find the homogeneneous position.

        \beta
     */   
    const CoordinateFrame& frame() const {
        return m_frame;
    }

    /** If there is a bulbPowerTrack, then the bulbPower will be overwritten from it during simulation.*/
    virtual void onSimulation(SimTime absoluteTime, SimTime deltaTime) override;

    /** 
        Optional shadow map.  May be NULL
    */
    shared_ptr<class ShadowMap> shadowMap() const {
        return m_shadowMap;
    }
    
    virtual Any toAny(const bool forceAll = false) const override;

    /** 
        \param toLight will be normalized 
        Only allocates the shadow map if \a shadowMapRes is greater than zero and castsShadows is true

        \deprecated
    */
    static shared_ptr<Light> directional(const String& name, const Vector3& toLight, const Radiance3& color, bool castsShadows = true, int shadowMapRes = 2048);

    /** \deprecated */
    static shared_ptr<Light> point(const String& name, const Point3& pos, const Power3& color, float constAtt = 0.01f, float linAtt = 0, float quadAtt = 1.0f, bool castsShadows = true, int shadowMapRes = 2048);

    /** \param pointDirection Will be normalized.  Points in the
        direction that light propagates.

        \param halfAngleRadians Must be on the range [0, pi()/2]. This
        is the angle from the point direction to the edge of the light
        cone.  I.e., a value of pi() / 4 produces a light with a pi() / 2-degree 
        cone of view.

        \deprecated
    */
    static shared_ptr<Light> spot
       (const String& name, 
        const Point3& pos, 
        const Vector3& pointDirection, 
        float halfAngleRadians, 
        const Color3& color, 
        float constAtt = 0.01f,
        float linAtt = 0, 
        float quadAtt = 1.0f,
        bool castsShadows = true,
        int shadowMapRes = 2048);

    /** Creates a spot light that looks at a specific point (by calling spot() ) 
    \deprecated */
    static shared_ptr<Light> spotTarget
       (const String& name, 
        const Point3& pos, 
        const Point3& target, 
        float halfAngleRadians, 
        const Color3& color, 
        float constAtt = 0.01f, 
        float linAtt = 0, 
        float quadAtt = 1.0f,
        bool  castsShadows = true,
        int  shadowMapRes = 2048) {
        return spot(name, pos, target - pos, halfAngleRadians, color, constAtt, linAtt, quadAtt, castsShadows);
    }
    
    /** Returns the sphere within which this light has some noticable effect.  May be infinite.
        \param cutoff The value at which the light intensity is considered negligible. */
    class Sphere effectSphere(float cutoff = 30.0f / 255) const;
    
    /** Distance from the point to the light (infinity for DIRECTIONAL lights).

        \beta */
    float distance(const Point3& p) const {
        if (m_type == Type::DIRECTIONAL) {
            return finf();
        } else {
            return (p - m_frame.translation).length();
        }
    }


    /** 
       The size ("diameter") of the emitter along the x and y axes of its frame().

       AREA and DIRECTIONAL lights emit from the entire surface.  POINT and SPOT lights
       only emit from the center, although they use the extent for radial falloff to 
       avoid superbrightenening. Extent is also used for Draw::light, debugging
       and selection by SceneEditorWindow. 
      
       \cite http://imdoingitwrong.wordpress.com/2011/01/31/light-attenuation
      */
    const Vector2& extent() const {
        return m_extent;
    }

    static int findBrightestLightIndex(const Array<shared_ptr<Light> >& array, const Point3& point);

    static shared_ptr<Light> findBrightestLight(const Array<shared_ptr<Light> >& array, const Point3& point);

        

    /** If true, the emitter (and its emission cone for a spot light)
        is rectangular instead elliptical.

        Defaults to false. */
    bool rectangular() const {
        return m_spotSquare;
    }

    /** In a global illumination renderer, should this light create
        indirect illumination (in addition to direct illumination)
        effects (e.g., by emitting photons in a photon mapper)?
        
        Defaults to true.*/
    bool producesIndirectIllumination() const {
        return m_producesIndirectIllumination;
    }

    bool producesDirectIllumination() const {
        return m_producesDirectIllumination;
    }


    /** Sets the following arguments in \param args:
        \code
        vec4  prefix+position;
        vec3  prefix+color;
        vec4  prefix+attenuation;
        vec3  prefix+direction;
        bool  prefix+rectangular;
        vec3  prefix+up;
        vec3  prefix+right;
        float prefix+radius;
        prefix+shadowMap...[See ShadowMap::setShaderArgs]
        \endcode
        */
    void setShaderArgs(class UniformTable& args, const String& prefix) const;

    void setCastsShadows(bool castsShadows) {
        if (m_castsShadows != castsShadows) {
            m_castsShadows = castsShadows;
            m_lastChangeTime = System::time();
        }
    }    

    void setEnabled(bool enabled) {
        if (m_enabled != enabled) {
            m_enabled = enabled;
            m_lastChangeTime = System::time();
        }  
    }    

    virtual void makeGUI(class GuiPane* pane, class GApp* app) override;

};

} // namespace

G3D_DECLARE_ENUM_CLASS_HASHCODE(G3D::Light::Type);

#endif

