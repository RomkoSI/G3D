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
        const shared_ptr<Image4>& normalMap = bumpMap->normalBumpMap()->image();
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
    return UniversalBSDF::smoothnessToBlinnPhongExponent(smoothness);
}


Radiance3 UniversalSurfel::emittedRadiance(const Vector3& wo) const {
    return emission;
}


bool UniversalSurfel::transmissive() const {
    return isTransmissive;
}


Color3 UniversalSurfel::finiteScatteringDensity
(const Vector3&    w_i, 
 const Vector3&    w_o,
 const ExpressiveParameters& expressiveParameters) const {
    // Fresnel reflection at normal incidence
    const Color3& F_0 = glossyReflectionCoefficient;

    // Lambertian reflectivity (conditioned on not glossy reflected)
    const Color3& p_L = lambertianReflectivity * expressiveParameters.boost(lambertianReflectivity);

    // Surface normal
    const Vector3& n = shadingNormal;

    // Half vector
    const Vector3& w_h = (w_i + w_o).directionOrZero();

    // Frensel reflection coefficient for this angle. Ignore fresnel
    // on surfaces that are magically set to zero reflectance.
    const Color3& F =
        F_0.nonZero() ?
        UniversalBSDF::schlickFresnel(F_0, max(0.0f, w_h.dot(w_i)), smoothness) : Color3::zero();

    // Lambertian term
    Color3 result = (Color3::one() - F) * p_L / pif();

    // Ignore mirror impulse's contribution, which is handled in getImpulses().
    if (smoothness != 1.0f) {
        // Normalized Blinn-Phong lobe
        const float m = UniversalBSDF::smoothnessToBlinnPhongExponent(smoothness);
        const float glossyLobe = pow(max(w_h.dot(n), 0.0f), m) *
            (8.0f + m) / (8.0f * pif() * square(max(w_i.dot(n), w_o.dot(n))));
        result += F * glossyLobe;
    }

    return result;
}


void UniversalSurfel::getImpulses
(PathDirection     direction,
 const Vector3&    w_o,
 ImpulseArray&     impulseArray,
 const ExpressiveParameters& expressiveParameters) const {

    impulseArray.clear();

    // Fresnel reflection at normal incidence
    const Color3& F_0 = glossyReflectionCoefficient;

    // Lambertian reflectivity (conditioned on not glossy reflected)
    const Color3& p_L = lambertianReflectivity;

    // Transmission (conditioned on not glossy or lambertian reflected)
    const Color3& T = transmissionCoefficient;

    // Surface normal
    const Vector3& n = shadingNormal;

    // The half-vector IS the normal for mirror reflection purposes.
    // Frensel reflection coefficient for this angle. Ignore fresnel
    // on surfaces that are magically set to zero reflectance.
    const Color3& F = F_0.nonZero() ? UniversalBSDF::schlickFresnel(F_0, max(0.0f, n.dot(w_o)), smoothness) : Color3::zero();

    // Mirror reflection
    if ((smoothness == 1.0f) && F_0.nonZero()) {
        Surfel::Impulse& impulse = impulseArray.next();
        impulse.direction = w_o.reflectAbout(n);
        impulse.magnitude = F;
    }

    // Transmission
    const Color3& transmissionMagnitude = T * (Color3::one() - F) * (Color3::one() - (Color3::one() - F) * p_L);
    if (transmissionMagnitude.nonZero()) {
        const Vector3& transmissionDirection = (-w_o).refractionDirection(n, etaNeg, etaPos);

        // Test for total internal reflection before applying this impulse
        if (transmissionDirection.nonZero()) {
            Surfel::Impulse& impulse = impulseArray.next();
            impulse.direction = transmissionDirection;
            impulse.magnitude = transmissionMagnitude;
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
        const float boost = expressiveParameters.boost(lambertianReflectivity);
        return lambertianReflectivity * boost;
    } else {
        // Compute numerically
        return Surfel::probabilityOfScattering(pathDirection, w, rng, expressiveParameters);
    }
}


void UniversalSurfel::sampleFiniteDirectionPDF
   (PathDirection      pathDirection,
    const Vector3&     w_o,
    Random&            rng,
    const ExpressiveParameters& expressiveParameters,
    Vector3&           w_i,
    float&             pdfValue) const {

    // Surface normal
    const Vector3& n = shadingNormal;

    // Fresnel reflection at normal incidence
    const Color3& F_0 = glossyReflectionCoefficient;

    // Estimate the fresnel term coarsely, assuming mirror reflection. This is only used
    // for estimating the relativeGlossyProbability for the pdf; error will only lead to
    // noise, not bias in the result.
    const Color3& F = F_0.nonZero() ? UniversalBSDF::schlickFresnel(F_0, max(0.0f, n.dot(w_o)), smoothness) : Color3::zero();

    // Lambertian reflectivity (conditioned on not glossy reflected)
    const Color3& p_L = lambertianReflectivity;

    // Exponent for the cosine power lobe in the PDF that we're sampling. Rolling off
    // slightly from pure Blinn-Phong appears to give faster convergence.
    const float m = UniversalBSDF::smoothnessToBlinnPhongExponent(smoothness * 0.8f);

    float relativeGlossyProbability = F_0.nonZero() ? F.average() / (F + (Color3::one() - F) * p_L).average() : 0.0f;
    Vector3::cosHemiPlusCosPowHemiHemiRandom(w_o.reflectAbout(n), shadingNormal, m, relativeGlossyProbability, rng, w_i, pdfValue);
}

} // namespace
