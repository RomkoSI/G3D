/** 
 \file initGLG3D.cpp
 G3D Library http://g3d.cs.williams.edu
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/
#include "G3D/platform.h"
#include "G3D/WeakCache.h"
#include "GLG3D/AudioDevice.h"
#include "GLG3D/UniversalMaterial.h"
#include "GLG3D/GLPixelTransferBuffer.h"

namespace G3D {

class GFont;
class GuiTheme;

namespace _internal {
    WeakCache< String, shared_ptr<GFont> >& fontCache();
    WeakCache< String, shared_ptr<GuiTheme> >& themeCache();
    WeakCache<UniversalMaterial::Specification, shared_ptr<UniversalMaterial> >& materialCache();
}


static void GLG3DCleanupHook() {
    _internal::materialCache().clear();
    _internal::themeCache().clear();
    _internal::fontCache().clear();
    GLPixelTransferBuffer::deleteAllBuffers();

#   ifndef G3D_NO_FMOD
	delete AudioDevice::instance;
	AudioDevice::instance = NULL;
#   endif
}


// Forward declaration.  See G3D.h
void initG3D(const G3DSpecification& spec = G3DSpecification());

void initGLG3D(const G3DSpecification& spec) {
    static bool initialized = false;
    if (! initialized) {
        initG3D(spec);
#       ifndef G3D_NO_FMOD
            AudioDevice::instance = new AudioDevice();
            AudioDevice::instance->init(spec.audio);
#       endif
        atexit(&GLG3DCleanupHook);
        initialized = true;
    }
}

} // namespace
