#version 410 // -*- c++ -*-
#include <compatibility.glsl>
#include <g3dmath.glsl>


// needed to make the bump map code compile on AMD GPUs,
// which don't eliminate the dead code before compiling it for
// this GS profile
#define dFdx(g) ((g) * 0.0)   
#define dFdy(g) ((g) * 0.0)   
#define discard

layout(points) in;

#define CONSTRUCT_CENTER_VERTEX 1
#if CONSTRUCT_CENTER_VERTEX
    layout(triangle_strip, max_vertices = 8) out;
#else
    layout(triangle_strip, max_vertices = 4) out;
#endif

#define HAS_LAMBERTIAN_TERM 1
#define HAS_GLOSSY_TERM 1

#include <LightingEnvironment/LightingEnvironment_uniforms.glsl>
#include <Light/Light.glsl>


#include <UniversalMaterial/UniversalMaterial.glsl>
uniform UniversalMaterial2DArray material;

const float WRAP_SHADING_AMOUNT = 8.0;
const float ENVIRONMENT_SCALE   = max(0.0, 1.0 - WRAP_SHADING_AMOUNT / 20.0);

const int  RECEIVES_SHADOWS_MASK = 2;

// Allow direct lights to go above their normal intensity--helps bring out volumetric shadows
uniform float   directAlphaBoost;

uniform float   directValueBoost;

// Increases the saturation of all lighting
uniform float   directSaturationBoost;
uniform float   environmentSaturationBoost;

uniform float2  textureGridSize;
uniform float2  textureGridInvSize;

uniform float   nearPlaneZ;

// These arrays have a single element because they are GL_POINTS
layout(location = 0) in Point3       wsCenterVertexOutput[];
layout(location = 1) in float3       shapeVertexOutput[];
layout(location = 2) in int4         materialPropertiesVertexOutput[];
layout(location = 3) in float        angleVertexOutput[];

#include "ParticleSurface_helpers.glsl"
out RenderGeometryOutputs geoOutputs;

// Shadow map bias...must be relatively high for particles because
// adjacent particles (and this one!) will be oriented differently in the
// shadow map and can easily self-shadow
const float bias = 0.5; // meters

Point3 project(Vector4 v) {
    return v.xyz * (1.0 / v.w);
}


float3 boostSaturation(float3 color, float boost) {
    if (boost != 1.0) {
        color = RGBtoHSV(color);
        color.y *= boost;
        color = HSVtoRGB(color);
    }
    return color;
}

// Compute average lighting around each vertex
Radiance3 computeLight(Point3 wsPosition, Vector3 normal, inout float alphaBoost, Vector3 wsLookVector, bool receivesShadows) {
    float3 L_in = float3(0);

    // Forward-scattering approximation. (lambertian) 0 <= k <= 1 (forward)
    const float forwardScatterStrength = 0.75;

    // TODO: Could compute only from center vertex to reduce computation if desired
    Vector3 w_o    = normalize(g3d_CameraToWorldMatrix[3].xyz - wsPosition);

    // Environment
#   for (int i = 0; i < NUM_ENVIRONMENT_MAPS; ++i)
    {
        // Uniform evt component
        // Sample the highest MIP-level to approximate Lambertian integration over the hemisphere
        float3 ambientEvt = (textureLod(environmentMap$(i)_buffer, float3(1,1,1), 20).rgb + 
                             textureLod(environmentMap$(i)_buffer, float3(-1,-1,-1), 20).rgb) * 0.5;

        // Directional evt component from front
        float3 directionalEvt = textureLod(environmentMap$(i)_buffer, normal, 10).rgb;
        L_in += boostSaturation((ambientEvt + directionalEvt) * 0.5, environmentSaturationBoost) * environmentMap$(i)_readMultiplyFirst.rgb;
    }
#   endfor

//    L_in *= ENVIRONMENT_SCALE;
    alphaBoost = 1.0;

#   for (int I = 0; I < NUM_LIGHTS; ++I)
    {
        Vector3 w_i = light$(I)_position.xyz - wsPosition;
        float  lightDistance = length(w_i);
        w_i /= lightDistance;

        // Spot light falloff
        float brightness = light$(I)_position.w * spotLightFalloff(w_i, light$(I)_direction, light$(I)_right, light$(I)_up, light$(I)_rectangular, light$(I)_attenuation.w, light$(I)_softnessConstant);

        // Directional light has no falloff
        brightness += 1.0 - light$(I)_position.w;

        if (brightness > 0.0) {
            brightness /= (4.0 * pi * dot(float3(1.0, lightDistance, lightDistance * lightDistance), light$(I)_attenuation.xyz));

            // The light is the same as for any surface up to this point. Now add particle-specific effects

            // Heavily-biased wrap shading to create some directional variation
            //float wrapShading = (dot(light$(I)_direction, normalize(normal * 1.1 + light$(I)_direction)) + WRAP_SHADING_AMOUNT) / (1.0 + WRAP_SHADING_AMOUNT);

            // Boost brightness when backlit
            //float backlit = pow(max(0.0, -dot(light$(I)_direction, w_o)), 50.0) * 1.5;

            //brightness *= (wrapShading + backlit);

            // Choose the normal for shading purposes to be towards the light
            //computeDirectLighting(w_i, w_i, w_o, w_i, 1.0, wsCenterVertexOutput[0] + w_i * bias, float glossyExponent, inout Color3 E_lambertian, inout Color3 E_glossy) {

#           if defined(light$(I)_shadowMap_notNull)
                if (receivesShadows) {

                    // Compute projected shadow coord.
                    vec3 projShadowCoord = project(light$(I)_shadowMap_MVP * vec4(wsPosition + w_i * bias, 1.0));

                    // From external shadows.  Could use fewer samples for more distant smoke or
                    // average the value at the center
                    float visibility = 
                        (texture(light$(I)_shadowMap_buffer, projShadowCoord + vec3(vec2(+1.0, +1.0) * light$(I)_shadowMap_invSize.xy, 0.0)) +
                         texture(light$(I)_shadowMap_buffer, projShadowCoord + vec3(vec2(+1.0, -1.0) * light$(I)_shadowMap_invSize.xy, 0.0)) +
                         texture(light$(I)_shadowMap_buffer, projShadowCoord + vec3(vec2(-1.0, -1.0) * light$(I)_shadowMap_invSize.xy, 0.0)) +
                         texture(light$(I)_shadowMap_buffer, projShadowCoord + vec3(vec2(-1.0, -1.0) * light$(I)_shadowMap_invSize.xy, 0.0))) / 4;
                
                    // Cubing visibility increases the self-shadowing effect (and the impact of 
                    // stochastic transparent shadows on particles) to generally improve the 
                    // appearance of particle systems.
                    visibility *= visibility * visibility;

                    // Boost the opacity of lit particles
                    //alphaBoost = max(alphaBoost, directAlphaBoost * visibility);

                    brightness *= visibility;
                } // receives shadows
#           endif

            float brdf = mix(1.0, pow(max(-dot(w_i, w_o), 0.0), forwardScatterStrength * 60.0) * (forwardScatterStrength * 60.0 + 1.0) / 4.0, forwardScatterStrength); 

            // TODO: Maybe need to divide by pi?
            brightness *= brdf;

//            brightness *= directValueBoost;

            // faux specular highlight at "bright" areas
            //float satBoost = lerp(directSaturationBoost, min(directSaturationBoost, 0.75), clamp(pow(brightness * 0.2, 20.0), 0.0, 1.0));
            // L_in += brightness * boostSaturation(light$(I)_color, satBoost);
            L_in += brightness * light$(I)_color;
        } // in spotlight

    } // for
#   endfor

    return L_in;
}

float alpha = 0.0;


/** Produce a vertex.  Note that x and y are compile-time constants, so most of this arithmetic compiles out. */
void emit(float x, float y, Vector3 normal, Vector3 wsLook, bool receivesShadows, Vector2 csRight, Vector2 csUp, Vector3 wsRight, Vector3 wsUp) {
    Point3 wsPosition = wsCenterVertexOutput[0] + wsRight * x + wsUp * y;

    float alphaBoost;
    geoOutputs.color.rgb = computeLight(wsPosition, normal, alphaBoost, wsLook, receivesShadows);
    geoOutputs.color.a   = min(1.0, alpha * alphaBoost);
    
    int texelWidth = materialPropertiesVertexOutput[0].y;
    geoOutputs.texCoord.xy = ((Point2(x, y) * 0.5) + Vector2(0.5, 0.5)) * float(texelWidth) * material.lambertian.invSize.xy;
    geoOutputs.texCoord.z  = materialPropertiesVertexOutput[0].x;
    geoOutputs.csPosition = Vector3(gl_in[0].gl_Position.xy + csRight * x + csUp * y, gl_in[0].gl_Position.z);
    gl_Position           = g3d_ProjectionMatrix * Vector4(geoOutputs.csPosition, 1.0);
    EmitVertex();
}


void main() {
    float csZ = gl_in[0].gl_Position.z;
    if (csZ >= nearPlaneZ) {
        // Near-plane culled
        return;
    }

    // Read the particle properties
    bool  receivesShadows = bool(materialPropertiesVertexOutput[0].z & RECEIVES_SHADOWS_MASK);
    float radius          = shapeVertexOutput[0].x;
    float angle           = angleVertexOutput[0];
    float coverage        = shapeVertexOutput[0].y;

    // Used for shadow map bias
    Vector3 wsLook = g3d_CameraToWorldMatrix[2].xyz;

    // TODO: Bend normal for each vertex
    Vector3 normal = normalize(g3d_CameraToWorldMatrix[3].xyz - wsCenterVertexOutput[0]);

    // Fade out alpha as the billboard approaches the near plane
    float softParticleFadeRadius = radius * 3.0;
    alpha = min(1.0, coverage * saturate((nearPlaneZ - csZ) / softParticleFadeRadius));

    // Rotate the particle
    Vector2 csRight = Vector2(cos(angle), sin(angle)) * radius;
    Vector2 csUp    = Vector2(-csRight.y, csRight.x);

    Vector3 wsRight = g3d_CameraToWorldMatrix[0].xyz * csRight.x + g3d_CameraToWorldMatrix[1].xyz * csRight.y;
    Vector3 wsUp    = g3d_CameraToWorldMatrix[0].xyz * csUp.x    + g3d_CameraToWorldMatrix[1].xyz * csUp.y;

    // 
    //   C-------D    C-------D
    //   | \     |    | \   / |
    //   |   \   |    |   E   |
    //   |     \ |    | /   \ |
    //   A-------B    A-------B
    //
    //     ABCD       ABEDC AEC
#   if CONSTRUCT_CENTER_VERTEX
        emit(-1, -1, normal, wsLook, receivesShadows, csRight, csUp, wsRight, wsUp); // A
        emit(+1, -1, normal, wsLook, receivesShadows, csRight, csUp, wsRight, wsUp); // B
        emit( 0,  0, normal, wsLook, receivesShadows, csRight, csUp, wsRight, wsUp); // E
        emit(+1, +1, normal, wsLook, receivesShadows, csRight, csUp, wsRight, wsUp); // D
        emit(-1, +1, normal, wsLook, receivesShadows, csRight, csUp, wsRight, wsUp); // C
        EndPrimitive();

        emit(-1, -1, normal, wsLook, receivesShadows, csRight, csUp, wsRight, wsUp); // A
        emit( 0, 0,  normal, wsLook, receivesShadows, csRight, csUp, wsRight, wsUp); // E
        emit(-1, +1, normal, wsLook, receivesShadows, csRight, csUp, wsRight, wsUp); // C
        EndPrimitive();
#   else
        emit(-1, -1, normal, wsLook, receivesShadows, csRight, csUp, wsRight, wsUp); // A
        emit(+1, -1, normal, wsLook, receivesShadows, csRight, csUp, wsRight, wsUp); // B
        emit(-1, +1, normal, wsLook, receivesShadows, csRight, csUp, wsRight, wsUp); // C
        emit(+1, +1, normal, wsLook, receivesShadows, csRight, csUp, wsRight, wsUp); // D
        EndPrimitive();
#   endif
}
