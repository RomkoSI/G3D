/**
  \file GLG3D/NativeTriTree.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2009-06-10
  \edited  2016-09-14
*/
#pragma once

#include "G3D/platform.h"

#ifdef G3D_WINDOWS
#   include "GLG3D/EmbreeTriTree.h"
#else
#   include "GLG3D/NativeTriTree.h"
#endif

namespace G3D {
#ifdef G3D_WINDOWS
    typedef EmbreeTriTree TriTree;
#else
    typedef NativeTriTree TriTree;
#endif
}
