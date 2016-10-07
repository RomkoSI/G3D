// -*- c++ -*- \file deferredHelpers.glsl */
#ifndef deferredHelpers_glsl
#define deferredHelpers_glsl

#ifndef GBuffer_glsl
#   error "Requires GBuffer.glsl to be included first"
#endif

#include <g3dmath.glsl>
#include <reconstructFromDepth.glsl>
#include <UniversalMaterial/UniversalMaterial_sample.glsl>

// Depends on gbuffer_* being defined as globals
void readUniversalMaterialSampleFromGBuffer(ivec2 C, const bool discardOnBackground, out Vector3 w_o, out UniversalMaterialSample surfel) {

    // Surface normal
    Vector3 csN = texelFetch(gbuffer_CS_NORMAL_buffer, C, 0).xyz;
    if ((dot(csN, csN) < 0.01) && discardOnBackground) {
        // This is a background pixel, not part of an object
        discard;
    } else {
        surfel.tsNormal = surfel.geometricNormal = surfel.shadingNormal = surfel.glossyShadingNormal = normalize(mat3x3(gbuffer_camera_frame) * (csN * gbuffer_CS_NORMAL_readMultiplyFirst.xyz + gbuffer_CS_NORMAL_readAddSecond.xyz));
    }
    surfel.offsetTexCoord = Point2(0);

    // Point being shaded
    float csZ = reconstructCSZ(texelFetch(gbuffer_DEPTH_buffer, C, 0).r, gbuffer_camera_clipInfo);
    Point3 csPosition = reconstructCSPosition(gl_FragCoord.xy, csZ, gbuffer_camera_projInfo);
    surfel.position = (gbuffer_camera_frame * vec4(csPosition, 1.0)).xyz;
    
    // View vector
    w_o = normalize(gbuffer_camera_frame[3] - surfel.position);

#   ifdef gbuffer_LAMBERTIAN_notNull    
        surfel.lambertianReflectivity = texelFetch(gbuffer_LAMBERTIAN_buffer, C, 0).rgb;
#   else
        surfel.lambertianReflectivity = Color3(0);
#   endif

    surfel.coverage = 1.0;

    {
        Color4  temp;
#       ifdef gbuffer_GLOSSY_notNull
            temp = texelFetch(gbuffer_GLOSSY_buffer, C, 0);
#       else
            temp = Color4(0);
#       endif
        surfel.fresnelReflectionAtNormalIncidence = temp.rgb;
        surfel.smoothness = temp.a;
    }

    surfel.transmissionCoefficient = Color3(0);
    surfel.collimation = 1.0;
#   ifdef gbuffer_EMISSIVE_notNull
        surfel.emissive = texelFetch(gbuffer_EMISSIVE_buffer, C, 0).rgb * gbuffer_EMISSIVE_readMultiplyFirst.rgb + gbuffer_EMISSIVE_readAddSecond.rgb;
#   else
        surfel.emissive = Radiance3(0);
#   endif

    surfel.lightMapRadiance = Radiance3(0);
}

#endif
