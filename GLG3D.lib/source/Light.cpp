/**
  \file Light.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2003-11-12
  \edited  2016-02-22
*/
#include "G3D/Sphere.h"
#include "G3D/CoordinateFrame.h"
#include "G3D/Any.h"
#include "G3D/stringutils.h"
#include "G3D/units.h"
#include "G3D/Random.h"
#include "GLG3D/Light.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/ShadowMap.h"
#include "G3D/Cone.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/GApp.h"
#include "GLG3D/GuiPane.h"

namespace G3D {

shared_ptr<Entity> Light::create(const String& name, Scene* scene, AnyTableReader& propertyTable, const ModelTable& modelTable,
    const Scene::LoadOptions& options) {

    const shared_ptr<Light> light(new Light());

    light->Entity::init(name, scene, propertyTable);
    light->Light::init(name, propertyTable);
    propertyTable.verifyDone();

    return light;
}


void Light::init(const String& name, AnyTableReader& propertyTable){
    propertyTable.getIfPresent("extent", m_extent);
    propertyTable.getIfPresent("bulbPower", color);
    propertyTable.getIfPresent("bulbPowerTrack", m_bulbPowerTrack);
    propertyTable.getIfPresent("biradiance", color);
    propertyTable.getIfPresent("type", m_type);
    propertyTable.getIfPresent("stochasticShadows", m_stochasticShadows);
    propertyTable.getIfPresent("varianceShadowSettings", m_varianceShadowSettings);

    Any temp;
    if (propertyTable.getIfPresent("shadowCullFace", temp)) {
        m_shadowCullFace = temp;
        temp.verify(m_shadowCullFace != CullFace::CURRENT, "A Light's shadowCullFace may not be CURRENT.");
    }

    // Default shadow maps to 2048 by 2048
    Vector2int16 shadowMapSize(2048, 2048);
    if (propertyTable.getIfPresent("shadowMapSize", shadowMapSize)) {
        // If the light has a shadowmap, it probably should cast shadows by default
        m_castsShadows = true;
    }

    propertyTable.getIfPresent("castsShadows", m_castsShadows);

    float f;
    bool hasShadowMapBias = propertyTable.getIfPresent("shadowMapBias", f);
    if (m_castsShadows && (min(shadowMapSize.x, shadowMapSize.y) > 0)) {
        m_shadowMap = ShadowMap::create(name + " Shadow Map", shadowMapSize, m_stochasticShadows, m_varianceShadowSettings);
        if (hasShadowMapBias) {
            m_shadowMap->setBias(f);
        }
    }

    propertyTable.getIfPresent("producesIndirectIllumination", m_producesIndirectIllumination);
    propertyTable.getIfPresent("producesDirectIllumination", m_producesDirectIllumination);

    propertyTable.getIfPresent("enabled",       m_enabled);
    
    float spotHalfAngleDegrees = 180;
    propertyTable.getIfPresent("spotHalfAngleDegrees", spotHalfAngleDegrees);
    m_spotHalfAngle = toRadians(spotHalfAngleDegrees);

    propertyTable.getIfPresent("spotSquare",    m_spotSquare);

    Any test;
    if (propertyTable.getIfPresent("nearPlaneZLimit", test)) {
        m_nearPlaneZLimit = test;
        test.verify(m_nearPlaneZLimit < 0, "nearPlaneZLimit must be negative");
    }

    if (propertyTable.getIfPresent("farPlaneZLimit", test)) {
        m_farPlaneZLimit = test;
        test.verify(m_farPlaneZLimit < m_nearPlaneZLimit, "farPlaneZLimit must be less than nearPlaneZLimit");
    }    
    
    const float radius = m_extent.length() / 2.0f;
    Array<float> tempAtt(square(radius), 0.0f, 1.0f);
    if (m_type == Type::DIRECTIONAL) {
        // No attenuation on directional lights by default
        tempAtt[0] = 1.0f;
        tempAtt[1] = 0.0f;
        tempAtt[2] = 0.0f;
    }
    propertyTable.getIfPresent("attenuation",  tempAtt);
    for (int i = 0; i < 3; ++i) {
        attenuation[i] = tempAtt[i];
    }
}


int Light::findBrightestLightIndex(const Array<shared_ptr<Light> >& array, const Point3& point) {
    Radiance r = -finf();
    int index = -1;
    for (int i = 0; i < array.length(); ++i) {
        Radiance t = array[i]->biradiance(point).sum();
        if (t > r) {
            r = t;
            index = i;
        }
    }

    return index;
}


shared_ptr<Light> Light::findBrightestLight(const Array<shared_ptr<Light> >& array, const Point3& point) {
    if (array.size() > 0) {
        return array[findBrightestLightIndex(array, point)];
    } else {
        return shared_ptr<Light>();
    }
}


Power3 Light::bulbPower() const {
    if (m_type == Type::DIRECTIONAL) {
        // Directional lights have infinite power
        return Power3((color.r > 0) ? color.r * finf() : 0, (color.g > 0) ? color.g * finf() : 0, (color.b > 0) ? color.b * finf() : 0);
    } else {
        return color;
    }
}


Power3 Light::emittedPower() const {
    switch (m_type) {
    case Type::OMNI:
    case Type::DIRECTIONAL:
        return bulbPower();

    case Type::SPOT:
    default:
        // Bulb power scaled by the fraction of sphere's solid angle subtended
        return (1.0f - cos(spotHalfAngle())) * bulbPower() * 0.5f *
            (rectangular()? (4.0f / pif())  : 1.0f);
    }
}


bool Light::inFieldOfView(const Vector3& wi) const {
    const CFrame& lightFrame = frame();

    switch (type()) {
    case Type::OMNI:
        return true;

    case Type::DIRECTIONAL:
        return wi == -lightFrame.lookVector();

    case Type::SPOT:
        {
            const float threshold = -cos(spotHalfAngle());
            if (rectangular()) {
                // Project wi onto the light's xz-plane and then normalize
                const Vector3& w_horizontal = (wi - wi.dot(lightFrame.rightVector()) * lightFrame.rightVector()).direction();
                const Vector3& w_vertical   = (wi - wi.dot(lightFrame.upVector())    * lightFrame.upVector()).direction();

                // Now test against the view cone in each of the planes 
                return 
                    (w_horizontal.dot(lightFrame.lookVector()) < threshold) &&
                    (w_vertical.dot(lightFrame.lookVector())   < threshold);
            } else {
                // Test against a cone
                return wi.dot(lightFrame.lookVector()) < threshold;
            }
        }

    case Type::AREA:
        return true;

    default:
        alwaysAssertM(false, "Invalid light type for Light::inFieldOfView()");
        return false;
    }
}


Biradiance3 Light::biradiance(const Point3& X) const {
    Vector3 wi = frame().translation - X;
    const float d = wi.length();
    wi /= d;

    if (inFieldOfView(wi)) {
        return bulbPower() / (4 * pif() * (attenuation[0] + attenuation[1] * d + attenuation[2] * square(d)));
    } else {
        return Biradiance3::zero();
    }
}


Vector3 Light::randomEmissionDirection(Random& rng) const {
    if (m_type == Type::SPOT) {
        if( rectangular() ) {
            Vector3 v;
            do {
                v = Vector3::random(rng);
            } while (!inFieldOfView(-v.direction()));
            return v.direction();
        }
        Cone spotCone(Point3(), m_frame.lookVector(), spotHalfAngle());
        return spotCone.randomDirectionInCone(rng);
    } if (m_type == Type::DIRECTIONAL) {
        return m_frame.lookVector();
    } else {
        return Vector3::random(rng);
    }  
}


void Light::onSimulation(SimTime absoluteTime, SimTime deltaTime) {
    Entity::onSimulation(absoluteTime, deltaTime);

    static const float radius = extent().length() / 2.0f;
    static const float dirDist = 1000.0f;
    switch(m_type){

    case Type::SPOT:
    case Type::OMNI:
        m_lastSphereBounds  = Sphere(position().xyz(), radius);
        break;
    case Type::DIRECTIONAL:
        m_lastSphereBounds  = Sphere(position().xyz() * dirDist, radius * dirDist);
        
        break;
    default:
        alwaysAssertM(false, "Invalid light type");
        break;
    }

    m_lastSphereBounds.getBounds(m_lastAABoxBounds);
    m_lastBoxBoundArray.resize(1);
    m_lastBoxBoundArray[0] = m_lastBoxBounds = m_lastAABoxBounds;

    if (m_bulbPowerTrack.size() > 0) {
        const Color3& c = max(m_bulbPowerTrack.evaluate((float)absoluteTime), Color3::zero());
        if (color != c) {
            m_lastChangeTime = System::time();
        }
        color = c;
    }
}


Any Light::toAny(const bool forceAll) const {
    
    Any a = Entity::toAny(forceAll);
    a.setName("Light");
    
    //check if the color has changed if it has change it
    AnyTableReader oldValues(a);
    Color3 temp;
    oldValues.getIfPresent("biradiance", temp);
    oldValues.getIfPresent("bulbPower", temp);
    if (temp != color) { 
        a["bulbPower"] = color;
    }
    
    //check if the attenuation has changed. if it has change it
    bool setAttenuation = false;
    Array<float> tempAtt(0.0001f, 0.0f, 1.0f);
    oldValues.getIfPresent("attenuation",  tempAtt);
    for (int i = 0; i < 3; ++i) {
        if (attenuation[i] != tempAtt[i]) {
            setAttenuation = true;
        }
    }
    if (setAttenuation) {
        a["attenuation"] = tempAtt; 
    }

    float spotHalfAngleDegrees;
    oldValues.getIfPresent("spotHalfAngleDegrees", spotHalfAngleDegrees);
    if ( m_spotHalfAngle != toRadians(spotHalfAngleDegrees) ) {
        a["spotHalfAngleDegrees"] = toDegrees(m_spotHalfAngle);
    }

    bool castsShadows = false;
    oldValues.getIfPresent("castsShadows", castsShadows);
    if (m_castsShadows != castsShadows) {
        a["castsShadows"] = m_castsShadows; 
    }

    bool stochasticShadows = false;
    oldValues.getIfPresent("stochasticShadows", stochasticShadows);
    if (m_stochasticShadows != stochasticShadows) {
        a["stochasticShadows"] = m_stochasticShadows;
    }

    ShadowMap::VSMSettings vsmSettings;
    oldValues.getIfPresent("varianceShadowSettings", vsmSettings);
    if (m_varianceShadowSettings != vsmSettings) {
        a["varianceShadowSettings"] = vsmSettings;
    }


    bool enabled;
    oldValues.getIfPresent("enabled", enabled); 
    if ( m_enabled != enabled ) {
        a["enabled"] = m_enabled;
    }

    bool producesIndirect;
    oldValues.getIfPresent("producesIndirectIllumination", producesIndirect);
    if ( m_producesIndirectIllumination != producesIndirect ) {
        a["producesIndirectIllumination"] = m_producesIndirectIllumination;
    }

    bool producesDirect;
    oldValues.getIfPresent("producesDirectIllumination", producesDirect);
    if ( m_producesDirectIllumination != producesDirect ) {
        a["producesDirectIllumination"] = m_producesDirectIllumination;
    }

    return a;
}

    
Light::Light() :
    m_type(Type::DIRECTIONAL),
    m_spotHalfAngle(pif()),
    m_spotSquare(false),
    m_castsShadows(true),
    m_stochasticShadows(false),
    m_varianceShadowSettings(),
    m_shadowCullFace(CullFace::BACK),
    m_enabled(true),
    m_extent(0.2f, 0.2f),
    m_producesIndirectIllumination(true),
    m_producesDirectIllumination(true),
    m_nearPlaneZLimit(-0.01f),
    m_farPlaneZLimit(-finf()),
    color(Color3::white()) {

    // Directional light attenuation
    attenuation[0]  = 0.0001f;
    attenuation[1]  = 0.0f;
    attenuation[2]  = 1.0f;
}


void Light::setFrame(const CFrame& c) {
    Entity::setFrame(c);
    if (m_type == Type::DIRECTIONAL) {
        const Vector3& v = -m_frame.lookVector();
        // Send each component towards infinity in the appropriate direction
        for (int a = 0; a < 3; ++a) {
            m_frame.translation[a] = (v[a] > 0) ? finf() : (v[a] < 0) ? -finf() : 0.0f;
        }
    }
}


shared_ptr<Light> Light::directional(const String& name, const Vector3& toLight, const Color3& color, bool s, int shadowMapRes) {
    
    shared_ptr<Light> L(new Light());
    L->m_name = name;
    L->m_type = Type::DIRECTIONAL;

    CFrame c;
    c.lookAt(-toLight.direction());
    L->setFrame(c);

    L->color    = color;
    L->m_castsShadows = s;
    if (L->m_castsShadows && (shadowMapRes > 0)) {
        L->m_shadowMap = ShadowMap::create("shadow map", Vector2int16(shadowMapRes, shadowMapRes));
    }
    return L;
}


shared_ptr<Light> Light::point(const String& name, const Vector3& pos, const Color3& color, float constAtt, float linAtt, float quadAtt, bool s, int shadowMapRes) {
    
    shared_ptr<Light> L(new Light());

    L->m_name = name;
    L->m_type = Type::OMNI;
    L->m_frame.translation = pos;
    L->color    = color;
    L->attenuation[0] = constAtt;
    L->attenuation[1] = linAtt;
    L->attenuation[2] = quadAtt;
    L->m_castsShadows   = s;
    if (L->m_castsShadows && (shadowMapRes > 0)) {
        L->m_shadowMap = ShadowMap::create(name + " shadow map", Vector2int16(shadowMapRes, shadowMapRes));
    }
    L->computeFrame(-Vector3::unitZ(), Vector3::unitX());

    return L;
    
}


shared_ptr<Light> Light::spot(const String& name, const Vector3& pos, const Vector3& pointDirection, float spotHalfAngle, const Color3& color, float constAtt, float linAtt, float quadAtt, bool s, int shadowMapRes) {
    
    shared_ptr<Light> L(new Light());
    L->m_name         = name;
    L->m_type         = Type::SPOT;
    L->m_frame.translation = pos;
    debugAssert(spotHalfAngle <= pif() / 2.0f);
    L->m_spotHalfAngle  = spotHalfAngle;
    L->color          = color;
    L->attenuation[0] = constAtt;
    L->attenuation[1] = linAtt;
    L->attenuation[2] = quadAtt;
    L->m_castsShadows   = s;
    if (L->m_castsShadows && (shadowMapRes > 0)) {
        L->m_shadowMap = ShadowMap::create(name + " shadow map", Vector2int16(shadowMapRes, shadowMapRes));
    }
    L->computeFrame(pointDirection.direction(), Vector3::zero());

    return L;

}


Sphere Light::effectSphere(float cutoff) const {
    if (m_type == Type::DIRECTIONAL) {
        // Directional light
        return Sphere(Vector3::zero(), finf());
    } else {
        // Avoid divide by zero
        cutoff = max(cutoff, 0.00001f);
        float maxIntensity = max(color.r, max(color.g, color.b));

        float radius = finf();
            
        if (attenuation[2] != 0) {

            // Solve I / attenuation.dot(1, r, r^2) < cutoff for r
            //
            //  a[0] + a[1] r + a[2] r^2 > I/cutoff
            //
            
            float a = attenuation[2];
            float b = attenuation[1];
            float c = attenuation[0] - maxIntensity / cutoff;
            
            float discrim = square(b) - 4 * a * c;
            
            if (discrim >= 0) {
                discrim = sqrt(discrim);
                
                float r1 = (-b + discrim) / (2 * a);
                float r2 = (-b - discrim) / (2 * a);

                if (r1 < 0) {
                    if (r2 > 0) {
                        radius = r2;
                    }
                } else if (r2 > 0) {
                    radius = min(r1, r2);
                } else {
                    radius = r1;
                }
            }
                
        } else if (attenuation[1] != 0) {
            
            // Solve I / attenuation.dot(1, r) < cutoff for r
            //
            // r * a[1] + a[0] = I / cutoff
            // r = (I / cutoff - a[0]) / a[1]

            float radius = (maxIntensity / cutoff - attenuation[0]) / attenuation[1];
            radius = max(radius, 0.0f);
        }

        return Sphere(m_frame.translation, radius);

    }
}


void Light::computeFrame(const Vector3& spotDirection, const Vector3& rightDirection) {
    if (rightDirection == Vector3::zero()) {
        // No specified right direction; choose one automatically
        if (m_type == Type::DIRECTIONAL) {
            // Directional light
            m_frame.lookAt(-m_frame.translation);
        } else {
            // Spot light
            m_frame.lookAt(m_frame.translation + spotDirection);
        }
    } else {
        const Vector3& Z = -spotDirection.direction();
        Vector3 X = rightDirection.direction();

        // Ensure the vectors are not too close together
        while (abs(X.dot(Z)) > 0.9f) {
            X = Vector3::random();
        }

        // Ensure perpendicular
        X -= Z * Z.dot(X);
        const Vector3& Y = Z.cross(X);

        m_frame.rotation.setColumn(Vector3::X_AXIS, X);
        m_frame.rotation.setColumn(Vector3::Y_AXIS, Y);
        m_frame.rotation.setColumn(Vector3::Z_AXIS, Z);
    }
}


void Light::setShaderArgs(UniformTable& args, const String& prefix) const {
    args.setUniform(prefix + "stochasticShadows", m_stochasticShadows);
    args.setUniform(prefix + "position",    position());
    args.setUniform(prefix + "color",       color);
    args.setUniform(prefix + "rectangular", m_spotSquare);
    args.setUniform(prefix + "direction",   frame().lookVector());
    args.setUniform(prefix + "up",          frame().upVector());
    args.setUniform(prefix + "right",       frame().rightVector());
    
    float cosThresh = -1;

    if (spotHalfAngle() < pif()) {
        // Spot light
        const float angle = spotHalfAngle();
        cosThresh = cos(angle);
    }

    args.setUniform(prefix + "attenuation", 
        Vector4(attenuation[0], 
                attenuation[1], 
                attenuation[2], 
                cosThresh));

    const float lightRadius = m_extent.length() / 2.0f;
    args.setUniform(prefix + "radius", lightRadius);

    if (castsShadows()) {
        shadowMap()->setShaderArgsRead(args, prefix + "shadowMap_");
    }
}


float Light::nearPlaneZ() const {
    if (notNull(m_shadowMap)) {
        return m_shadowMap->projection().nearPlaneZ();
    } else {
        return m_nearPlaneZLimit;
    }
}


float Light::farPlaneZ() const {
    if (notNull(m_shadowMap)) {
        return m_shadowMap->projection().farPlaneZ();
    } else {
        return m_farPlaneZLimit;
    }
}


float Light::nearPlaneZLimit() const {
    return m_nearPlaneZLimit;
}


float Light::farPlaneZLimit() const {
    return m_farPlaneZLimit;
}


void Light::makeGUI(class GuiPane* pane, class GApp* app) {
    Entity::makeGUI(pane, app);
    pane->addCheckBox("Enabled", Pointer<bool>(this, &Light::enabled, &Light::setEnabled));
}

} // G3D
