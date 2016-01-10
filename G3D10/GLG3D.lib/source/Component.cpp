/**
 @file   Component.cpp
 @author Morgan McGuire, http://graphics.cs.williams.edu
 @date   2009-02-19

 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */

#include "GLG3D/Component.h"

namespace G3D {

const ImageFormat* ImageUtils::to8(const ImageFormat* f) {
    switch (f->code) {
    case ImageFormat::CODE_L32F:
        return ImageFormat::L8();

    case ImageFormat::CODE_R32F:
        return ImageFormat::R8();

    case ImageFormat::CODE_RGB32F:
        return ImageFormat::RGB8();

    case ImageFormat::CODE_RGBA32F:
        return ImageFormat::RGBA8();

    default:
        alwaysAssertM(false, "Unsupported format");
        return f;
    }
}

}
