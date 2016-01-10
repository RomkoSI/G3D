// -*- c++ -*-
/** 
  \file Light.glsl
  \author Morgan McGuire (http://graphics.cs.williams.edu), Michael Mara (http://www.illuminationcodified.com)
  Defines helper functions to calculate the light contribution to a specified point.
  
  If HAS_NORMAL_BUMP_MAP is defined, then this file must have tan_Z and backside in scope
*/
#ifndef Light_glsl
#define Light_glsl

// HIGH_QUALITY_SHADOW_FILTERING is automatically triggered by setting any light in stochasticShadows mode
//#define HIGH_QUALITY_SHADOW_FILTERING 1

#include <g3dmath.glsl>

float shadowFetch(sampler2DShadow shadowMap, vec3 coord, vec2 invSize) {
    return texture(shadowMap, vec3(clamp(coord.xy, invSize, vec2(1) - invSize), coord.z));
}


/** Returns a number between 0 = fully shadowed and 1 = fully lit.  Assumes that the shadowCoord is already within the spotlight cone. */
float shadowMapVisibility(vec3 lightDirection, vec4 lightPosition, vec4 lightAttenuation, vec4 shadowCoord, sampler2DShadow shadowMap, vec2 invSize, bool highQualityFiltering) {    
    // Compute projected shadow coord.
    vec3 projShadowCoord = shadowCoord.xyz / shadowCoord.w;

    // "side" and "diagonal" offset variables used to produce vectors to the 
    // 8-neighbors, which leads to a smoother shadow result (0.71 is sqrt(2)/2).
    vec4 s = vec4(invSize, -invSize.x, 0.0);
    vec4 d = s * 0.71;

    if (highQualityFiltering) {
        // Nicer filtering
        float vSum = 0.0;
        float wSum = 0.0;
        const int radius = 7;
        for (int dx = -radius; dx <= +radius; ++dx) {
            for (int dy = -radius; dy <= +radius; ++dy) {
                if (dx * dx + dy * dy <= radius * radius) {
                    // Ad-hoc cone falloff. The constants are hopefully statically evaluated
                    float w = 1.0 / (radius + length(vec2(dx, dy)));
                    wSum += w;
                    // 1.25 gives a larger radius without too much dithering in the output
                    vSum += w * shadowFetch(shadowMap, projShadowCoord + 1.25 * vec3(dx * invSize.x, dy * invSize.y, 0), invSize);
                }
            }
        }
        return vSum /= wSum;
    } else {
        return
            ((shadowFetch(shadowMap, projShadowCoord, invSize) +
             
              shadowFetch(shadowMap, projShadowCoord + s.xww, invSize) +
              shadowFetch(shadowMap, projShadowCoord - s.xww, invSize) +
              shadowFetch(shadowMap, projShadowCoord + s.wyw, invSize) +
              shadowFetch(shadowMap, projShadowCoord - s.wyw, invSize) +
         
              shadowFetch(shadowMap, projShadowCoord + d.xyw, invSize) +
              shadowFetch(shadowMap, projShadowCoord - d.zyw, invSize) +
              shadowFetch(shadowMap, projShadowCoord + d.zyw, invSize) +
              shadowFetch(shadowMap, projShadowCoord - d.xyw, invSize)) / 9.0);
    }
    /*
    // Old code to clamp to zero beyond border. We clamp 1 pixel beyond in order to
    // get correct blended values near the border.
    float(all(bvec4(greaterThanEqual(projShadowCoord.xy, -s.xy),
                    lessThanEqual(projShadowCoord.xy, vec2(1.0, 1.0) + s.xy)))) */

}

float linstep(float minV, float maxV, float v) {
    return clamp((v-minV) / (maxV - minV), 0.0, 1.0);
}


float reduceLightBleeding(float p_max, float amount) {
    // Remove the [0, amount] tail and linearly rescale (amount, 1].  
    return linstep(amount, 1.0, p_max);
}

/** Returns a number between 0 = fully shadowed and 1 = fully lit.  Assumes that the shadowCoord is already within the spotlight cone. */
float varianceShadowMapVisibility(vec4 shadowCoord, float lightSpaceZ, sampler2D varianceShadowMap, float lightBleedReduction) {
    vec2 texCoord = shadowCoord.xy / shadowCoord.w;
    //texCoord.y = 1.0-texCoord.y;
    vec2 M = texture(varianceShadowMap, texCoord).xy;

    float mean = M.x;
    float variance = M.y - M.x*M.x;

    // http://www.punkuser.net/vsm/vsm_paper.pdf; equation 5
    float t_sub_mean = (lightSpaceZ - mean);
    float chebeychev = variance / (variance + (t_sub_mean*t_sub_mean));


    return (lightSpaceZ >= mean) ? 1.0 : reduceLightBleeding(chebeychev, lightBleedReduction);
}


/** Below this value, attenuation is treated as zero. This is non-zero only to avoid numerical imprecision. */
const float attenuationThreshold = 2e-17;

/** Returns true if the vector w_i (which points at the light's center) is within the field of view of this light source. */
bool inLightFieldOfView
   (vec3                w_i, 
    vec3                lightLookVector, 
    vec3                lightRightVector, 
    vec3                lightUpVector,
    bool                rectangular, 
    float               cosFOV) {

    // When the light field of view is very small, we need to be very careful with precision 
    // on the computation below, so there are epsilon values in the comparisons.
    if (rectangular) {
        // Project wi onto the light's xz-plane and then normalize
        vec3 w_horizontal = normalize(w_i - dot(w_i, lightRightVector) * lightRightVector);
        vec3 w_vertical   = normalize(w_i - dot(w_i, lightUpVector)    * lightUpVector);

        // Now test against the view cone in each of the planes 
        return
            (dot(w_horizontal, lightLookVector) <= -cosFOV + 1e-5) &&
            (dot(w_vertical,   lightLookVector) <= -cosFOV + 1e-5);
    } else {
        return dot(lightLookVector, w_i) <= -cosFOV + 1e-5;
    }
}


/** Computes attenuation due to backface or radial falloff.
    \return vec2(subsurface/diffuse, surface/glossy)  
*/
vec2 computeAttenuation
  (in vec3              n, 
   in vec3              glossyN,
   in vec4              lightPosition, 
   in vec4              lightAttenuation, 
   in vec3              wsPosition, 
   in vec3              lightLook, 
   in vec3              lightUpVector, 
   in vec3              lightRightVector, 
   in bool              lightRectangular, 
   in float             lightRadius,
   in vec3              tan_Z,
   in float             backside,
   out vec3             w_i) {
    // Light vector
    w_i = lightPosition.xyz - wsPosition.xyz * lightPosition.w;
    float lightDistance = max(lightRadius, length(w_i));
    w_i = normalize(w_i);

    float attenuation =
        lightPosition.w *

       (inLightFieldOfView(w_i, lightLook, lightRightVector, lightUpVector, lightRectangular, lightAttenuation.w) ? 
         
         // Within spotlight cone
         (1.0 / (4.0 * pi * dot( vec3(1.0, lightDistance, lightDistance * lightDistance), lightAttenuation.xyz) )) : 
         
         // Outside spotlight cone
         0.0) + (1.0 - lightPosition.w);

#   ifdef HAS_NORMAL_BUMP_MAP
        // For a bump mapped surface, do not allow illumination on the back side even if the
        // displacement creates a light-facing surface, since it should be self-shadowed for any 
        // large polygon.
        attenuation *= float(dot(tan_Z.xyz, w_i) * backside > 0.0);
#   endif

    // Attenuation is modulated by the cosine of the angle of incidence, which may be different for the two surface
    // normal vectors
    return attenuation * max(vec2(dot(w_i, n), dot(w_i, glossyN)), 0.0);
}


void computeShading
   (in vec3             wsN,
    in vec3             wsGlossyN,
    in vec3             wsE, 
    in vec2             attenuation, 
    in float            glossyExponent, 
    in vec3             lightColor, 
    inout vec3          I_lambertian,
    inout vec3          I_glossy, 
    in vec3             wsL) {

    I_lambertian += attenuation[0] * lightColor;

    if (glossyExponent > 0.0) {
        // cosine of the angle between the normal and the half-vector
        vec3 wsH = normalize(wsL + wsE);
        float cos_h = max(dot(wsH, wsGlossyN), 0.0);
        I_glossy += lightColor * (attenuation[1] * pow(cos_h, glossyExponent));
    }
}


/**
 \param lightAttenuation OpenGL distance attenuation polynomial coefficients, with cosine of spotlight angle in the w component
 \param lightDirection   OpenGL spotlight direction
 */
void addLightContribution
   (in vec3             n, 
    in vec3             glossyN, 
    in vec3             wsE,
    in vec3             wsPosition, 
    in float            glossyExponent, 
    in vec4             lightPosition, 
    in vec4             lightAttenuation,
    in vec3             lightLook, 
    in vec3             lightUpVector, 
    in vec3             lightRightVector, 
    in bool             lightRectangular,
    in float            lightRadius,
    in vec3             lightColor,
    in vec3             tan_Z,
    in float            backside,
    inout vec3          I_lambertian,
    inout vec3          I_glossy,
    out vec3            w_i) {

    // vec2(subsurface/diffuse, surface/glossy)  
    vec2 attenuation = computeAttenuation(n, glossyN, lightPosition, lightAttenuation, wsPosition, lightLook, lightUpVector, lightRightVector, lightRectangular, lightRadius, tan_Z, backside, w_i);

    if (max(attenuation.x, attenuation.y) >= attenuationThreshold) {
        computeShading(n, glossyN, wsE, attenuation, glossyExponent, lightColor, I_lambertian, I_glossy, w_i);
    }
}


void addShadowedLightContribution
   (in vec3             n,
    in vec3             glossyN,
    in vec3             wsE,
    in vec3             wsPosition, 
    in float            glossyExponent,
    in vec4             lightPosition,
    in vec4             lightAttenuation,
    in vec3             lightLook,
    in vec3             lightUpVector, 
    in vec3             lightRightVector, 
    in bool             lightRectangular,
    in float            lightRadius,
    in vec3             lightColor, 
    in bool             lightStochastic,
    in vec4             shadowCoord,
    in sampler2DShadow  shadowMap,
    in vec2             invShadowMapSize,
    in vec3             tan_Z,
    in float            backside,
    inout vec3          I_lambertian, 
    inout vec3          I_glossy, 
    out vec3            w_i) {

    vec2 attenuation = computeAttenuation(n, glossyN, lightPosition, lightAttenuation, wsPosition, lightLook, lightUpVector, lightRightVector, lightRectangular, lightRadius, tan_Z, backside, w_i);

    if (max(attenuation.x, attenuation.y) >= attenuationThreshold) {
        // The following call assumes that attenuation is non-zero
        float s = shadowMapVisibility(lightLook, lightPosition, lightAttenuation, shadowCoord, shadowMap, invShadowMapSize, lightStochastic);

        if (s <= attenuationThreshold) {
            // No light due to shadowing
            return;
        }

        attenuation *= s;
        computeShading(n, glossyN, wsE, attenuation, glossyExponent, lightColor, I_lambertian, I_glossy, w_i);
    }
}


void addVarianceShadowedLightContribution
(in vec3                n,
    in vec3             glossyN,
    in vec3             wsE,
    in vec3             wsPosition,
    in float            glossyExponent,
    in vec4             lightPosition,
    in vec4             lightAttenuation,
    in vec3             lightLook,
    in vec3             lightUpVector,
    in vec3             lightRightVector,
    in bool             lightRectangular,
    in float            lightRadius,
    in vec3             lightColor,
    in vec4             shadowCoord,
    in float            lightSpaceZ,
    in float            lightBleedReduction,
    in sampler2D        varianceShadowMap,
    in vec2             invShadowMapSize,
    in vec3             tan_Z,
    in float            backside,
    inout vec3          I_lambertian,
    inout vec3          I_glossy,
    out vec3            w_i) {

    vec2 attenuation = computeAttenuation(n, glossyN, lightPosition, lightAttenuation, wsPosition, lightLook, lightUpVector, lightRightVector, lightRectangular, lightRadius, tan_Z, backside, w_i);

    if (max(attenuation.x, attenuation.y) >= attenuationThreshold) {
        // The following call assumes that attenuation is non-zero
        float s = varianceShadowMapVisibility(shadowCoord, lightSpaceZ, varianceShadowMap, lightBleedReduction);

        if (s <= attenuationThreshold) {
            // No light due to shadowing
            return;
        }

        attenuation *= s;
        computeShading(n, glossyN, wsE, attenuation, glossyExponent, lightColor, I_lambertian, I_glossy, w_i);
    }
}

#endif
