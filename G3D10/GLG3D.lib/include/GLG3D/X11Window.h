/**
 @file X11Window.h
  
 A OSWindow that uses the X11 API.

 @created       2005-06-04
 @edited        2005-06-04
    
 G3D Innovation Engine
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/

#ifndef G3D_X11WINDOW_H
#define G3D_X11WWINDOW_H

#include <G3D/platform.h>

#ifndef G3D_LINUX
#error X11Window only supported on Linux
#endif

// Current implementation: X11Window *is* SDLWindow
#include "GLG3D/SDLWindow.h"

#define X11Window SDLWindow

#endif
