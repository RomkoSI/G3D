/**
 @file G3D/glheaders.h

 #includes the OpenGL headers

 @maintainer Morgan McGuire, http://graphics.cs.williams.edu

 @created 2002-08-07
 @edited  2015-01-10

 G3D Library
 Copyright 2002-2015, Morgan McGuire.
 All rights reserved.
*/

#ifndef G3D_glHeaders_h
#define G3D_glHeaders_h

#include "G3D/platform.h"
#include "GL/glew.h"

#ifdef G3D_WINDOWS
#   include "GL/wglew.h"
#elif defined(G3D_OSX) || defined(G3D_FREEBSD) || defined(G3D_LINUX)
#   include "GL/glxew.h"
#endif

#ifdef G3D_OSX
#    include <OpenGL/glu.h>
#    include <OpenGL/OpenGL.h>
#endif

// needed for OS X header
#ifdef Status
#undef Status
#endif

#endif  // G3D_glHeaders_h
