#version 330 // -*- c++ -*-
#include <compatibility.glsl>
#include <g3dmath.glsl>

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

// needed to make the bump map code compile on AMD GPUs,
// which don't eliminate the dead code before compiling it for
// this GS profile
#define dFdx(g) ((g) * 0.0)   
#define dFdy(g) ((g) * 0.0)   

#include <UniversalMaterial/UniversalMaterial.glsl>
uniform UniversalMaterial2DArray material;

const float WRAP_SHADING_AMOUNT = 8.0;
const float ENVIRONMENT_SCALE   = max(0.0, 1.0 - WRAP_SHADING_AMOUNT / 20.0);

const int  RECEIVES_SHADOWS = 2;

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

float3 wsRight, wsUp;
float2 csRight, csUp;
Point3 wsPosition;

// Shadow map bias...must be relatively high for particles because
// adjacent particles (and this one!) will be oriented differently in the
// shadow map and can easily self-shadow
const float bias = 0.5; // meters

Point3 project(Vector4 v) {
    return v.xyz * (1.0 / v.w);
}

// Vector to the eye
Vector3 w_o    = normalize(wsCenterVertexOutput[0] - Point3(g3d_CameraToWorldMatrix[3][0],
                                                            g3d_CameraToWorldMatrix[3][1],
                                                            g3d_CameraToWorldMatrix[3][2]));

float3 boostSaturation(float3 color, float boost) {
    if (boost != 1.0) {
        color = RGBtoHSV(color);
        color.y *= boost;
        color = HSVtoRGB(color);
    }
    return color;
}

bool receivesShadows = false;

// Compute average lighting around each vertex
Radiance3 computeLight(Point3 wsPosition, Vector3 normal, inout float alphaBoost, Vector3 wsEyeVector) {
    float3 L_in = float3(0);

    // Environment
#   for (int i = 0; i < NUM_ENVIRONMENT_MAPS; ++i)
    {
        // Sample the highest MIP-level to approximate Lambertian integration over the hemisphere
        // (note that with a seamless texture extension we can perform this with a single texture fetch!)

        // Uniform evt component
        float3 ambientEvt = (textureLod(environmentMap$(i)_buffer, float3(0,1,0), 20).rgb + 
                             textureLod(environmentMap$(i)_buffer, float3(0,-1,0), 20).rgb) * 0.5;

        // directional evt component
        float3 directionalEvt = textureLod(environmentMap$(i)_buffer, normal, 20).rgb;
        L_in += boostSaturation((ambientEvt + directionalEvt) * 0.5, environmentSaturationBoost) * (environmentMap$(i)_readMultiplyFirst.rgb * pi);
    }
#   endfor

    L_in *= ENVIRONMENT_SCALE;
L_in *= 0.2; // TODO: remove. This temporarily turns off environment light
    alphaBoost = 1.0;

#   for (int I = 0; I < NUM_LIGHTS; ++I)
    {
        float brightness = 1.0;

        Vector3 w_i = light$(I)_position.xyz - wsPosition;
        float  lightDistance = length(w_i);
        w_i /= lightDistance;

        if (inLightFieldOfView(w_i, light$(I)_direction, light$(I)_right, light$(I)_up, light$(I)_rectangular, light$(I)_attenuation.w)) {
            brightness /= (4.0 * pi * dot(float3(1.0, lightDistance, lightDistance * lightDistance), light$(I)_attenuation.xyz));

            // Heavily-biased wrap shading to create some directional variation
            float wrapShading = (dot(light$(I)_direction, normalize(normal * 1.1 + light$(I)_direction)) + WRAP_SHADING_AMOUNT) / (1.0 + WRAP_SHADING_AMOUNT);

            // Boost brightness when backlit
            float backlit = pow(max(0.0, -dot(light$(I)_direction, w_o)), 50.0) * 1.5;

            brightness *= (wrapShading + backlit);
#           if defined(light$(I)_shadowMap_notNull)
                if (receivesShadows) {
                    // Compute projected shadow coord.
                    vec3 projShadowCoord = project(light$(I)_shadowMap_MVP * vec4(wsCenterVertexOutput[0] // wsPosition  TODO: morgan switched temporarily to the center to stabilize stochastic shadowing
                        + wsEyeVector * bias, 1.0));

                    // From external shadows.  Could use fewer samples for more distant smoke or
                    // average the value at the center
                    float visibility = 
                        (texture(light$(I)_shadowMap_buffer, projShadowCoord + vec3(vec2(+1.0, +1.0) * light$(I)_shadowMap_invSize.xy, 0.0)) +
                         texture(light$(I)_shadowMap_buffer, projShadowCoord + vec3(vec2(+1.0, -1.0) * light$(I)_shadowMap_invSize.xy, 0.0)) +
                         texture(light$(I)_shadowMap_buffer, projShadowCoord + vec3(vec2(-1.0, -1.0) * light$(I)_shadowMap_invSize.xy, 0.0)) +
                         texture(light$(I)_shadowMap_buffer, projShadowCoord + vec3(vec2(-1.0, -1.0) * light$(I)_shadowMap_invSize.xy, 0.0))) / 4;
                

                    alphaBoost = max(alphaBoost, directAlphaBoost * visibility);

                    // visibility = saturate(sqrt(visibility) * 1.5);

                    // Cubing visibility increases the self-shadowing effect (and the impact of 
                    // stochastic transparent shadows on particles) to generally improve the 
                    // appearance of particle systems.
                    brightness *= visibility * visibility * visibility;
                }
#           endif

            brightness *= directValueBoost;

            // faux specular highlight at "bright" areas
            float satBoost = lerp(directSaturationBoost, min(directSaturationBoost, 0.75), clamp(pow(brightness * 0.2, 20.0), 0.0, 1.0));
            L_in += brightness * boostSaturation(light$(I)_color, satBoost);
        }

    }
#   endfor

    return L_in;
}

float alpha = 0.0;

/** Produce a vertex.  Note that x and y are compile-time constants, so
most of this arithmetic compiles out. */
void constructOutputs(float x, float y, Vector3 normal, Vector3 wsEyeVector, out Vector4 outPosition, out RenderGeometryOutputs attributes) {

    attributes.csPosition = Vector3(gl_in[0].gl_Position.xy + csRight * x + csUp * y, gl_in[0].gl_Position.z);

    outPosition = g3d_ProjectionMatrix * float4(attributes.csPosition, 1.0);
    wsPosition = wsCenterVertexOutput[0] + wsRight * x + wsUp * y;

    float alphaBoost;
    attributes.color.rgb = computeLight(wsPosition, normal, alphaBoost, wsEyeVector);
    // Debugging code
    //    colorGeometryOut.rgb = -wsEyeVector * 0.5 + 0.5;    alphaBoost = 1.0;
    attributes.color.a = min(1.0, alpha * alphaBoost);

    int texelWidth = materialPropertiesVertexOutput[0].y;
    attributes.texCoord.xy = ((Point2(x, y) * 0.5) + Vector2(0.5, 0.5)) * float(texelWidth) * material.lambertian.invSize.xy;
    attributes.texCoord.z = materialPropertiesVertexOutput[0].x;
}

void emit(RenderGeometryOutputs attributes, Vector4 position) {
    geoOutputs = attributes;
    gl_Position = position;
    EmitVertex();
}


void emit(float x, float y, Vector3 normal, Vector3 wsEyeVector) {
    RenderGeometryOutputs attributes;
    Vector4 outPos;
    constructOutputs(x, y, normal, wsEyeVector, outPos, attributes);
    emit(attributes, outPos);
}




void main() {
    float csZ = gl_in[0].gl_Position.z;
    if (csZ >= nearPlaneZ) {
        // Near-plane culled
        return;
    }

    receivesShadows = bool(materialPropertiesVertexOutput[0].z & RECEIVES_SHADOWS);

    Vector3 wsEyeVector = float3(g3d_CameraToWorldMatrix[2][0], g3d_CameraToWorldMatrix[2][1], g3d_CameraToWorldMatrix[2][2]);
    Vector3 normal = wsEyeVector;

    float angle  = angleVertexOutput[0];
    // Fade out alpha as the billboard approaches the near plane
    float fadeRadius = 3.0;
    alpha = min(1.0, shapeVertexOutput[0].y * saturate((nearPlaneZ - csZ) / fadeRadius));

    // Rotate the particle
    float radius = shapeVertexOutput[0].x;
    csRight = Vector2(cos(angle), sin(angle)) * radius;
    csUp    = Vector2(-csRight.y, csRight.x);

    wsRight = g3d_CameraToWorldMatrix[0].xyz * csRight.x + g3d_CameraToWorldMatrix[1].xyz * csRight.y;
    wsUp    = g3d_CameraToWorldMatrix[0].xyz * csUp.x    + g3d_CameraToWorldMatrix[1].xyz * csUp.y;

// 
//   C-------D    C-------D
//   | \     |    | \   / |
//   |   \   |    |   E   |
//   |     \ |    | /   \ |
//   A-------B    A-------B
//
//     ABCD       ABEDC AEC
#if CONSTRUCT_CENTER_VERTEX
    emit(-1, -1, normal, wsEyeVector); // A
    emit(+1, -1, normal, wsEyeVector); // B
    emit( 0,  0, normal, wsEyeVector); // E
    emit(+1, +1, normal, wsEyeVector); // D
    emit(-1, +1, normal, wsEyeVector); // C
    EndPrimitive();

    emit(-1, -1, normal, wsEyeVector); // A
    emit( 0, 0, normal, wsEyeVector); // E
    emit(-1, +1, normal, wsEyeVector); // C
    EndPrimitive();
#else
    emit(-1, -1, normal, wsEyeVector); // A
    emit(+1, -1, normal, wsEyeVector); // B
    emit(-1, +1, normal, wsEyeVector); // C
    emit(+1, +1, normal, wsEyeVector); // D
    EndPrimitive();
#endif
  
    
}
