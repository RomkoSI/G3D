// -*- c++ -*-
/** \file LightingEnvironment/LightingEnvironment_LightUniforms.glsl */

#ifndef LightingEnvironment_LightUniforms_glsl
#define LightingEnvironment_LightUniforms_glsl

#include <g3dmath.glsl>
#include <Light/Light.glsl>
#include <compatibility.glsl>
#include <Texture/Texture.glsl>

#expect NUM_LIGHTS "Integer number of direct light sources (and shadow maps)"

#for (int I = 0; I < NUM_LIGHTS; ++I) 
    /** World space light position */
    uniform vec4        light$(I)_position;

    /** Power of the light */
    uniform vec3        light$(I)_color;

    uniform bool        light$(I)_stochasticShadows;

    /** Spot light facing direction (unit length) */
    uniform vec3        light$(I)_direction;

    /** w element is the spotlight cutoff angle.*/
    uniform vec4        light$(I)_attenuation;

    uniform float       light$(I)_softnessConstant;

    /** Is this spotlight's field of view rectangular (instead of round)? */
    uniform bool        light$(I)_rectangular;

    uniform vec3        light$(I)_up;

    uniform vec3        light$(I)_right;

    /** Radius of the light bulb itself; no relation to the light's effect sphere */
    uniform float       light$(I)_radius;

#   if defined(light$(I)_shadowMap_notNull)
        /** Modelview projection matrix used for the light's shadow map and VSM (they have the same projection) */
        uniform mat4    light$(I)_shadowMap_MVP;
        uniform float   light$(I)_shadowMap_bias;

        uniform_Texture(sampler2DShadow,   light$(I)_shadowMap_);
#       if defined(light$(I)_shadowMap_variance_notNull)
            uniform_Texture(sampler2D, light$(I)_shadowMap_variance_);

            uniform float light$(I)_shadowMap_variance_lightBleedReduction;
#       endif
#   endif
#endfor

/** Used by computeDirectLighting for lights that do 
    not have shadow maps on them. We don't pass dummy
    shadow maps per-Light because OpenGL has a small,
    limited number of samplers (when not using bindless
    texture), and that would cause scenes with many lights
    to exhaust the available samplers unnecessarily. */
uniform sampler2D       dummyLightSampler2D;
uniform sampler2DShadow dummyLightSampler2DShadow;



/**
 Uses global variables:

  light$(I)_position
  light$(I)_attenuation
  light$(I)_softnessConstant 
  light$(I)_direction
  light$(I)_up
  light$(I)_right
  light$(I)_rectangular
  light$(I)_radius
  light$(I)_color
  light$(I)_shadowMap_variance_notNull
  light$(I)_shadowMap_variance_buffer
  light$(I)_shadowMap_notNull
  light$(I)_shadowMap_invSize
  light$(I)_shadowMap_buffer

  \param glossyN A separate normal to use with glossy [surface, vs. subsurface] reflection terms (simulates a clear coat with a different orientation)
 */
void computeDirectLighting(Vector3 n, Vector3 glossyN, Vector3 w_o, Vector3 n_face, float backside, Point3 wsPosition, float glossyExponent, inout Color3 E_lambertian, inout Color3 E_glossy) {
    vec3 w_i;
#   for (int I = 0; I < NUM_LIGHTS; ++I)
    {    
        addLightContribution
           (n,
            glossyN,
            w_o,
            wsPosition,
            glossyExponent,
            light$(I)_position,
            light$(I)_attenuation,
            light$(I)_softnessConstant,
            light$(I)_direction,
            light$(I)_up,
            light$(I)_right,
            light$(I)_rectangular,
            light$(I)_radius,
            light$(I)_color,
            light$(I)_stochasticShadows,

#           ifdef light$(I)_shadowMap_notNull
                light$(I)_shadowMap_bias,
                light$(I)_shadowMap_MVP,

                light$(I)_shadowMap_buffer,
                light$(I)_shadowMap_notNull != 0,
                light$(I)_shadowMap_invSize.xy,
#           else
                // Pass dummy shadow map args
                0.0,
                Matrix4(),
                dummyLightSampler2DShadow,
                false,
                vec2(1.0),
#           endif

#           if defined(light$(I)_shadowMap_notNull) && defined(light$(I)_shadowMap_variance_notNull)
                light$(I)_shadowMap_variance_buffer,
                light$(I)_shadowMap_variance_notNull != 0,
                light$(I)_shadowMap_variance_lightBleedReduction,
#           else
                // Pass dummy VSM args
                dummyLightSampler2D,
                false,
                0.0,
#           endif

            n_face,
            backside,
            E_lambertian,
            E_glossy,
            w_i);       
    }
#   endfor
}

#endif
