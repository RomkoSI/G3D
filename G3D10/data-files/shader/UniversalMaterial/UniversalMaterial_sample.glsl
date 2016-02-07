#ifndef UniversalMaterial_sample_glsl
#define UniversalMaterial_sample_glsl

#include <UniversalMaterial/UniversalMaterial.glsl>
#include <g3dmath.glsl>
#include <BumpMap/BumpMap.glsl>
#include <lightMap.glsl>
#include <AlphaHint.glsl>

#ifndef OPAQUE_PASS
#   define OPAQUE_PASS 0
#endif

/** 
  Stores interpolated samples of the UniversalMaterial parameters in the particular
  encoding format used for that class and struct (including vertex properties). 
  This is also the format used by the GBuffer.

  \sa UniversalSurfel */
struct UniversalMaterialSample {
    /** Glossy not taken into account. */
    Color3          lambertianReflectivity;

    float           coverage;

    Color3          fresnelReflectionAtNormalIncidence;

    float           smoothness;

    /** Fresnel, glossy, and lambertian not taken into account. */
    Color3          transmissionCoefficient;

    float           collimation;

    Point2          offsetTexCoord;

    Radiance3       emissive;

    Radiance3       lightMapRadiance;

    /** In world space */
    Vector3         shadingNormal;

    /** In world space */
    Vector3         glossyShadingNormal;

    /** Tangent space normal */
    Vector3         tsNormal;
};

/** 

  \param tsEye Tangent space unit outgoing vector, w_o, used for parallax mapping
  \param wsE   World space unit outgoing vector, w_o

  All of the 'const bool' arguments must be const so that the branches can be evaluated
  at compile time.
 */
#foreach (dim, n) in (2D, 2)
UniversalMaterialSample sampleUniversalMaterial$(dim)
   (UniversalMaterial$(dim)     material,
    Point$(n)                   texCoord,
    Point$(n)                   lightmapCoord,
    Vector3                     tan_X, 
    Vector3                     tan_Y, 
    Vector3                     tan_Z,
    Vector3                     tsEye,
    float                       backside,
    const bool                  discardIfZeroCoverage,
    const bool                  discardIfFullCoverage,
    Color4                      vertexColor,
    const AlphaHint             alphaHint,
    const int                   parallaxSteps,
    const bool                  hasNormalBumpMap,
    const bool                  hasVertexColor,
    const bool                  hasMaterialAlpha,
    const bool                  hasTransmissive,
    const bool                  hasEmissive,
    const int                   numLightMapDirections) {

    UniversalMaterialSample smpl;

    const vec3 BLACK = vec3(0.0, 0.0, 0.0);
    Point$(n) offsetTexCoord;
    smpl.tsNormal = Vector3(0.0,0.0,1.0);
    if (hasNormalBumpMap) {
        float rawNormalLength = 1.0;
        if (parallaxSteps > 0) {
            bumpMap(material.normalBumpMap, material.bumpMapScale, material.bumpMapBias, texCoord, tan_X, tan_Y, tan_Z, backside, normalize(tsEye), smpl.shadingNormal, offsetTexCoord, smpl.tsNormal, rawNormalLength, parallaxSteps);
        } else {
            // Vanilla normal mapping
            bumpMap(material.normalBumpMap, 0.0, 0.0, texCoord, tan_X, tan_Y, tan_Z, backside, vec3(0.0), smpl.shadingNormal, offsetTexCoord, smpl.tsNormal, rawNormalLength, parallaxSteps);
        }
    } else {
        // World space normal
        smpl.shadingNormal = normalize(tan_Z.xyz * backside);
        offsetTexCoord = texCoord;
    }      
    
    smpl.coverage = 1.0;
    {
        vec4 temp = sampleTexture(material.lambertian, offsetTexCoord);
        if (hasVertexColor) {
            temp *= vertexColor;
        }

        smpl.lambertianReflectivity = temp.rgb;

        if (hasMaterialAlpha) {
            smpl.coverage = computeCoverage(alphaHint, temp.a);
            if (discardIfZeroCoverage) {
#               if OPAQUE_PASS
                    if (smpl.coverage < 1.0) { discard; }
#               else
                    // In the transparent pass, eliminate fully opaque pixels as well
                    if ((smpl.coverage <= 0.0) || (discardIfFullCoverage && ! hasTransmissive && (smpl.coverage >= 1.0))) {
                        discard; 
                    }
#               endif
            }
        }
    }

    switch (numLightMapDirections) {
    case 1:
        smpl.lightMapRadiance = sampleTexture(material.lightMap0, lightmapCoord).rgb;
        break;

    case 3:
        if (hasNormalBumpMap) {
            smpl.lightMapRadiance = radiosityNormalMap(material.lightMap0.sampler, material.lightMap1.sampler, material.lightMap2.sampler, lightmapCoord, smpl.tsNormal) * material.lightMap0.readMultiplyFirst.rgb;
        } else {
            // If there's no normal map, then the lightMap axes will all be at the same angle to this surfel,
            // so there's no need to compute dot products: just average
            smpl.lightMapRadiance = 
                (sampleTexture(material.lightMap0, lightmapCoord).rgb +
                 sampleTexture(material.lightMap1, lightmapCoord).rgb +
                 sampleTexture(material.lightMap2, lightmapCoord).rgb) * (1.0 / 3.0);
        }
        break;

    default:
        smpl.lightMapRadiance = Radiance3(0,0,0);
        break;
    } // switch


    if (hasEmissive) {
        smpl.emissive = sampleTexture(material.emissive, offsetTexCoord).rgb;
    } else {
        smpl.emissive = Radiance3(0,0,0);
    }

    float glossyExponent = 1, glossyCoefficient = 0;
    // We separate out the normal used for glossy reflection from the one used for shading in general
    // to allow subclasses to easily compute anisotropic highlights
    smpl.glossyShadingNormal = smpl.shadingNormal;
       
    {
        vec4 temp = sampleTexture(material.glossy, offsetTexCoord);
        smpl.fresnelReflectionAtNormalIncidence = temp.rgb;
        smpl.smoothness = temp.a;
    }
    
    if (hasNormalBumpMap) {
        // TODO: apply toksvig computation to smoothness, material.glossy.a
        //  glossyExponent = computeToksvigGlossyExponent(glossyExponent, rawNormalLength);
    }


    if (hasTransmissive) {
        vec4 tmp = sampleTexture(material.transmissive, offsetTexCoord);
        smpl.transmissionCoefficient = tmp.rgb;
        smpl.collimation = tmp.a;
    } else {
        smpl.transmissionCoefficient = Color3(0, 0, 0);
        smpl.collimation = 1.0;
    }

    smpl.offsetTexCoord = offsetTexCoord.xy;
    return smpl;
}
#endforeach


#endif
