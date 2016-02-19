// -*- c++ -*-
/** \file Camera/Camera.glsl 

 G3D Innovation Engine (http://g3d.codeplex.com)
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/

#ifndef Camera_glsl
#define Camera_glsl

#include <compatibility.glsl>

/**
 \def uniform_Camera

 Declares frame (CameraToWorld matrix), previousFrame, projectToPixelMatrix, clipInfo, and projInfo.
 On the host, invoke Camera::setShaderArgs
 to pass these values. Unused variables in the device 
 shader will be removed by the compiler.

 \param name Include the underscore suffix, if a name is desired

 \sa G3D::Camera, G3D::Camera::setShaderArgs, G3D::Args, uniform_GBuffer

 \deprecated
 */
#define uniform_Camera(name)\
    uniform mat4x3 name##frame;\
    uniform mat4x3 name##previousFrame;\
    uniform mat4x4 name##projectToPixelMatrix;\
    uniform float3 name##clipInfo;\
    uniform float4 name##projInfo

/**  
  Important properties of a G3D::Camera

  Bind this from C++ by calling camera->setShaderArgs("camera.");

  \sa G3D::Camera, G3D::Camera::setShaderArgs, G3D::Args, uniform_GBuffer */
struct Camera {
    mat4x3 frame;
    mat4x3 invFrame;
    mat4x3 previousFrame;
    mat4x4 projectToPixelMatrix;
    float3 clipInfo;
    float4 projInfo;
};
    
#endif
