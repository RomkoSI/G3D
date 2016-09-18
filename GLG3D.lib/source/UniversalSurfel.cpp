/**
  \file GLG3D.lib/source/UniversalSurfel.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2012-08-01
  \edited  2016-07-02

  Copyright 2001-2016, Morgan McGuire
 */
#include "GLG3D/UniversalSurfel.h"
#include "GLG3D/UniversalBSDF.h"
#include "GLG3D/UniversalMaterial.h"
#include "GLG3D/CPUVertexArray.h"
#include "GLG3D/Tri.h"
#include <memory>

namespace G3D {

UniversalSurfel::UniversalSurfel(const Tri& tri, float u, float v, int triIndex, const CPUVertexArray& vertexArray, bool backside) {
    this->source = Source(triIndex, u, v);

    const float w = 1.0f - u - v;
    const CPUVertexArray::Vertex& vert0 = tri.vertex(vertexArray, 0);
    const CPUVertexArray::Vertex& vert1 = tri.vertex(vertexArray, 1);
    const CPUVertexArray::Vertex& vert2 = tri.vertex(vertexArray, 2);    
  
    Vector3 interpolatedNormal =
       (w * vert0.normal + 
        u * vert1.normal +
        v * vert2.normal).direction();

    const Vector3& tangentX  =   
       (w * tri.tangent(vertexArray, 0) +
        u * tri.tangent(vertexArray, 1) +
        v * tri.tangent(vertexArray, 2)).direction();

    const Vector3& tangentY  =	
       (w * tri.tangent2(vertexArray, 0) +
        u * tri.tangent2(vertexArray, 1) +
        v * tri.tangent2(vertexArray, 2)).direction();

    const Vector2& texCoord =
        w * vert0.texCoord0 +
        u * vert1.texCoord0 +
        v * vert2.texCoord0;

    const shared_ptr<UniversalMaterial>& uMaterial = dynamic_pointer_cast<UniversalMaterial>(tri.material());
    debugAssertM(notNull(uMaterial), "Triangle does not have a UniversalMaterial on it");

    surface = tri.surface();
    const shared_ptr<UniversalBSDF>& bsdf = uMaterial->bsdf();
    material = uMaterial;

    geometricNormal = tri.normal(vertexArray);

    if (backside) {
        // Swap the normal direction here before we compute values relative to it
        interpolatedNormal = -interpolatedNormal;
        geometricNormal = -geometricNormal;

        // Swap sides
        etaNeg   = bsdf->etaReflect();
        etaPos   = bsdf->etaTransmit();
        kappaPos = bsdf->extinctionTransmit();
        kappaNeg = bsdf->extinctionReflect();
    } else {
        etaNeg   = bsdf->etaTransmit();
        etaPos   = bsdf->etaReflect();
        kappaPos = bsdf->extinctionReflect();
        kappaNeg = bsdf->extinctionTransmit();
    }    

    name = uMaterial->name();

    const shared_ptr<BumpMap>& bumpMap = uMaterial->bump();

    // TODO: support other types of bump map besides normal
    if (bumpMap && !tangentX.isNaN() && !tangentY.isNaN()) {
        const CFrame& tangentSpace = Matrix3::fromColumns(tangentX, tangentY, interpolatedNormal);
        Image4::Ref normalMap = bumpMap->normalBumpMap()->image();
        Vector2int32 mappedTexCoords(texCoord * Vector2(float(normalMap->width()), float(normalMap->height())));

        tangentSpaceNormal = Vector3(normalMap->get(mappedTexCoords.x, mappedTexCoords.y).rgb() * Color3(2.0f) + Color3(-1.0f));
        shadingNormal = tangentSpace.normalToWorldSpace(tangentSpaceNormal).direction();

        // "Shading tangents", or at least one tangent, is traditionally used in anisotropic
        // BSDFs. To combine this with bump-mapping we use Graham-Schmidt orthonormalization
        // to perturb the tangents by the bump map in a sensible way. 
        // See: http://developer.amd.com/media/gpu_assets/shaderx_perpixelaniso.pdf 
        // Note that if the bumped normal
        // is parallel to a tangent vector, this will give nonsensical results.
        // TODO: handle the parallel case elegantly
        // TODO: Have G3D support anisotropic direction maps so we can apply this transformation
        // to that instead of these tagents taken from texCoord directions
        // TODO: implement
        shadingTangent1 = tangentX;
        shadingTangent2 = tangentY;

        /*
        Vector3 tangentSpaceTangent1 = Vector3(1,0,0);
        Vector3 tangentSpaceTangent2 = Vector3(0,1,0);
        
        Vector3& t1 = tangentSpaceTangent1;
        Vector3& t2 = tangentSpaceTangent2;
        const Vector3& n = tangentSpaceNormal;
        t1 = (t1 - (n.dot(t1) * n)).direction();
        t2 = (t2 - (n.dot(t2) * n)).direction();

        shadingTangent1 = tangentSpace.normalToWorldSpace(tangentSpaceTangent1).direction();
        shadingTangent2 = tangentSpace.normalToWorldSpace(tangentSpaceTangent2).direction();*/
    } else {
        tangentSpaceNormal = Vector3(0, 0, 1);
        shadingNormal   = interpolatedNormal;
        shadingTangent1 = tangentX;
        shadingTangent2 = tangentY;
    } 

    position = 
       w * vert0.position + 
       u * vert1.position +
       v * vert2.position;
    
    if (vertexArray.hasPrevPosition()) {
        prevPosition = 
            w * vertexArray.prevPosition[tri.index[0]] + 
            u * vertexArray.prevPosition[tri.index[1]] +
            v * vertexArray.prevPosition[tri.index[2]];
    } else {
        prevPosition = position;
    }

    const Color4& lambertianSample = bsdf->lambertian().sample(texCoord);

    lambertianReflectivity = lambertianSample.rgb();
    coverage = lambertianSample.a;

    emission = uMaterial->emissive().sample(texCoord);

    const Color4& packG = bsdf->glossy().sample(texCoord);
    glossyReflectionCoefficient  = packG.rgb();
    smoothness     = packG.a;
    
    transmissionCoefficient = bsdf->transmissive().sample(texCoord);

    isTransmissive = transmissionCoefficient.nonZero() || (coverage < 1.0f);
}


float UniversalSurfel::blinnPhongExponent() const {
    return UniversalBSDF::unpackGlossyExponent(smoothness);
}


Radiance3 UniversalSurfel::emittedRadiance(const Vector3& wo) const {
    return emission;
}


bool UniversalSurfel::transmissive() const {
    return isTransmissive;
}


/** Computes F_r, given the cosine of the angle of incidence and 
    the reflectance at normal incidence. */
static inline Color3 schlickFresnel(const Color3& F0, float cos_i, float smoothness) {
    return UniversalBSDF::schlickFresnel(F0, cos_i, smoothness);
}


Color3 UniversalSurfel::finiteScatteringDensity
(const Vector3&    wi, 
 const Vector3&    wo,
 const ExpressiveParameters& expressiveParameters) const {

    static const float maxBlinnPhongExponent = 8000;
     
    if ((wi.dot(shadingNormal) < 0) || 
        (wo.dot(shadingNormal) < 0)) {
        // All transmission is by impulse, so there is no
        // finite density transmission.
        return Color3::zero();
    }
     
    const Vector3& n = shadingNormal;
    const float cos_i = fabs(wi.dot(n));
    
    static const float INV_8PI = 1.0f / (8.0f * pif());
    static const float INV_PI  = 1.0f / pif();

    // Base boost solely off Lambertian term
    const float boost = expressiveParameters.boost(lambertianReflectivity);

    Color3 S(Color3::zero());
    Color3 F(Color3::zero());
    if ((smoothness != 0.0f) && (glossyReflectionCoefficient.nonZero())) {
        // Glossy

        // Half-vector
        const Vector3& w_h = (wi + wo).direction();
        const float cos_h = max(0.0f, w_h.dot(n));
        
        const float s = min(blinnPhongExponent(), maxBlinnPhongExponent);
        
        F = schlickFresnel(glossyReflectionCoefficient, cos_i, smoothness);
        if (s == finf()) {
            // Will be handled by the mirror case
            S = Color3::zero();
        } else {
            S = F * (powf(cos_h, s) * (s + 8.0f) * INV_8PI);
        }
    }

    const Color3& D = (lambertianReflectivity * INV_PI) * (Color3::one() - F) * boost;

    return S + D;
}


void UniversalSurfel::getImpulses
(PathDirection     direction,
 const Vector3&    wi,
 ImpulseArray&     impulseArray,
 const ExpressiveParameters& expressiveParameters) const {

    impulseArray.clear();

    const Vector3& n = shadingNormal;
    debugAssert(wi.isUnit());
    debugAssert(n.isUnit());

    Color3 F;

    // Track whether the Fresnel coefficient is initialized
    bool Finit = false;

    ////////////////////////////////////////////////////////////////////////////////

    // If there is mirror reflection
    if (glossyReflectionCoefficient.nonZero() && (smoothness == 1.0f)) {
        // Cosine of the angle of incidence, for computing F
        const float cos_i = max(0.001f, wi.dot(n));
        F = schlickFresnel(glossyReflectionCoefficient, cos_i, 1.0f);
        Finit = true;
            
        // Mirror                
        Impulse& imp     = impulseArray.next();
        imp.direction    = wi.reflectAbout(n);
        imp.magnitude    = F;
    }
    
    // TODO: transmit should be conditioned on lambertian as well as glossy

    ////////////////////////////////////////////////////////////////////////////////

    // TODO: a constant transmit is not consistent with the extinction coefficient model--
    // let the caller choose.

    if (transmissionCoefficient.nonZero()) {
        // Fresnel transmissive coefficient
        Color3 F_t;

        if (Finit) {
            F_t = (Color3::one() - F);
        } else {
            // Cosine of the angle of incidence, for computing F
            const float cos_i = max(0.001f, wi.dot(n));
            // F = lerp(0, 1, pow5(1.0f - cos_i)) = pow5(1.0f - cos_i)
            // F_t = 1 - F
            F_t.r = F_t.g = F_t.b = 1.0f - pow5(1.0f - cos_i);
        }

        // Sample transmissive
        const Color3& T0 = transmissionCoefficient;
        const Color3& p_transmit  = F_t * T0;
       
        // Disabled; interpolated normals can be arbitrarily far out
        //debugAssertM(w_i.dot(n) >= -0.001, format("w_i dot n = %f", w_i.dot(n)));
        Impulse& imp     = impulseArray.next();

        imp.magnitude    = p_transmit;
        imp.direction    = (-wi).refractionDirection(n, etaNeg, etaPos);
        if (imp.direction.isZero()) {
            // Total internal refraction
            impulseArray.popDiscard();
        } else {
            debugAssert(imp.direction.isUnit());
        }
    }
}


Color3 UniversalSurfel::reflectivity(Random& rng,
 const ExpressiveParameters& expressiveParameters) const {

    // Base boost solely off Lambertian term
    const float boost = expressiveParameters.boost(lambertianReflectivity);

    // Only promises to be an approximation
    return lambertianReflectivity * boost + glossyReflectionCoefficient;
}


Color3 UniversalSurfel::probabilityOfScattering
   (PathDirection   pathDirection,
    const Vector3&  w,
    Random&         rng,
    const ExpressiveParameters& expressiveParameters) const {

    if (glossyReflectionCoefficient.isZero() && transmissionCoefficient.isZero()) {
        // No Fresnel term, so trivial to compute

        // Base boost solely off Lambertian term
        const float boost = expressiveParameters.boost(lambertianReflectivity);
        return lambertianReflectivity * boost;
    } else {
        // Compute numerically
        return Surfel::probabilityOfScattering(pathDirection, w, rng, expressiveParameters);
    }
}

} // namespace
