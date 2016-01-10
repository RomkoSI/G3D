// -*- c++ -*-
/** \file Texture/Texture.glsl 

 G3D Innovation Engine (http://g3d.codeplex.com)
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/

#ifndef Texture_glsl
#define Texture_glsl

#extension GL_ARB_texture_cube_map_array : require

#ifndef vec1
#define vec1 float
#endif

/**
 \def uniform_Texture

 Declares a uniform sampler and the float4 readMultiplyFirst and readAddSecond variables.
 The host Texture::setShaderArgs call will also bind a macro name##notNull if the arguments
 are bound on the host side. If they are not bound and the device code does not use them, then 
 GLSL will silently ignore the uniform declarations.

 \param samplerType sampler2D, sampler2DShadow, sampler3D, samplerRect, or samplerCube
 \param name Include the underscore suffix

 \sa G3D::Texture, G3D::Texture::setShaderArgs, G3D::Args
 */
#define uniform_Texture(samplerType, name)\
    uniform samplerType  name##buffer;\
    uniform vec3         name##size;\
    uniform vec3         name##invSize;\
    uniform vec4         name##readMultiplyFirst;\
    uniform vec4         name##readAddSecond

/**
 \def Texture2D

 Bind with Texture::setShaderArgs("texture.")

 \sa G3D::Texture, G3D::Texture::setShaderArgs, G3D::Args
*/
#foreach (dim, n) in (1D, 1), (2D, 2), (3D, 3), (Cube, 2), (2DRect, 2), (1DArray, 2), (2DArray, 3), (CubeArray, 2), (Buffer, 1), (2DMS, 2), (2DMSArray, 3), (1DShadow, 1), (2DShadow, 2), (CubeShadow, 2), (2DRectShadow, 2), (1DArrayShadow, 2), (2DArrayShadow, 3), (CubeArrayShadow, 3)
    struct Texture$(dim) {
        sampler$(dim) sampler;
        vec$(n)       size;
        vec$(n)       invSize;
        vec4          readMultiplyFirst;
        vec4          readAddSecond;

        /** false if the underlying texture was nullptr when bound */
        bool          notNull;
    };
#endforeach

vec4 sampleTexture(Texture2D tex, vec2 coord) {
    return texture(tex.sampler, coord) * tex.readMultiplyFirst + tex.readAddSecond;
}

vec4 sampleTexture(Texture2DArray tex, vec3 coord) {
    return texture(tex.sampler, coord) * tex.readMultiplyFirst + tex.readAddSecond;
}

#endif
