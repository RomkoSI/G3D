// -*- c++ -*-
/** \file UniversalMaterial/UniversalMaterial.glsl 

 G3D Innovation Engine (http://g3d.cs.williams.edu)
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/
#ifndef UniversalMaterial_glsl
#define UniversalMaterial_glsl

#include <compatibility.glsl>
#include <Texture/Texture.glsl>

/**
 \def uniform_UniversalMaterial

 Declares all material properties. Additional macros will also be bound 
 by UniversalMaterial::setShaderArgs:

 - name##NUM_LIGHTMAP_DIRECTIONS
 - name##HAS_NORMAL_BUMP_MAP
 - name##PARALLAXSTEPS
 
 \param name Include the underscore suffix, if a name is desired

 \sa G3D::UniversalMaterial, G3D::UniversalMaterial::setShaderArgs, G3D::Args, uniform_Texture


 \deprecated
 */
#define uniform_UniversalMaterial(name)\
    uniform_Texture(sampler2D, name##LAMBERTIAN_);              \
    uniform_Texture(sampler2D, name##GLOSSY_);                  \
    uniform_Texture(sampler2D, name##TRANSMISSIVE_);            \
    uniform float              name##etaTransmit;               \
    uniform float              name##etaRatio;                  \
    uniform_Texture(sampler2D, name##EMISSIVE_);                \
    uniform vec4	           name##customConstant;            \
    uniform sampler2D	       name##customMap;                 \
    uniform_Texture(sampler2D, name##lightMap0_);               \
    uniform_Texture(sampler2D, name##lightMap1_);               \
    uniform_Texture(sampler2D, name##lightMap2_);               \
    uniform sampler2D          name##normalBumpMap;             \
    uniform float              name##bumpMapScale;              \
    uniform float              name##bumpMapBias;               \
    uniform_Texture(sampler2D, name##customMap_)

/** 
   \def UniversalMaterial
 \sa G3D::UniversalMaterial, G3D::UniversalMaterial::setShaderArgs, G3D::Args, uniform_Texture

*/
#foreach (dim) in (2D), (3D), (2DArray)
    struct UniversalMaterial$(dim) {
        Texture$(dim)    lambertian;
        Texture$(dim)    glossy;
        Texture$(dim)    transmissive;
        Texture$(dim)    emissive;
        float            etaTransmit;
        //The ratio of the indices of refraction. etaRatio represnts the value (n1 / n2) or (etaReflect / etaTransmit).
        float            etaRatio;
        vec4             customConstant;
        Texture$(dim)    customMap;
        Texture$(dim)    lightMap0;
        Texture$(dim)    lightMap1;
        Texture$(dim)    lightMap2;
        sampler$(dim)    normalBumpMap;
        float            bumpMapScale;
        float            bumpMapBias;
        int              alphaHint;
    };
#endforeach

#endif
