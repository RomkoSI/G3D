/**
  \file G3D/FrameName.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2014-02-05
  \edited  2014-02-05

  Copyright 2000-2015, Morgan McGuire.
  All rights reserved.
*/

#ifndef G3D_FrameName_h
#define G3D_FrameName_h

#include "G3D/platform.h"
#include "G3D/enumclass.h"

namespace G3D {

/** \def FrameName

Logical reference frame.

NONE: Does not apply or unknown

WORLD: Scene's reference frame
        
OBJECT: A.k.a. body. For VR, this is the player's body

CAMERA: Camera's object space. For VR, this is the eye space

LIGHT: Light's object space 

TANGENT: Surface space. Texture space embedded in object space, i.e., relative to a unique 3D space that
    conforms to the object's surface with the Z axis pointing along the surface normal,
    and with a unique translation for every object space location.

TEXTURE: The object's 2D "uv" parameterization surface space. 

SCREEN: Pixels, with upper-left = (0, 0)
*/
G3D_DECLARE_ENUM_CLASS(FrameName, NONE, WORLD, OBJECT, CAMERA, LIGHT, TANGENT, TEXTURE, SCREEN);
}

G3D_DECLARE_ENUM_CLASS_HASHCODE(G3D::FrameName);

#endif
