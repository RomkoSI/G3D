/**
  \file GLG3D/NativeTriTree.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2009-06-10
  \edited  2016-10-01
*/
#pragma once

#include "G3D/platform.h"

#if defined(G3D_WINDOWS) || defined(G3D_OSX)
#   include "GLG3D/EmbreeTriTree.h"
#else
#   include "GLG3D/NativeTriTree.h"
#endif

namespace G3D {
#if defined(G3D_WINDOWS) || defined(G3D_OSX)
    typedef EmbreeTriTree TriTree;
#else
    typedef NativeTriTree TriTree;
#endif
}
