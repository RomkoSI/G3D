/**
 \file GLG3D.lib/source/Sampler.cpp

 \maintainer Michael Mara, http://www.illuminationcodified.com

  \created 2013-10-03
  \edited  2013-10-03
 */

#include "GLG3D/Sampler.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/glcalls.h"
#include "GLG3D/GLSamplerObject.h"
namespace G3D {

Any Sampler::toAny() const {
    Any a(Any::TABLE, "Sampler");
    a["interpolateMode"] = interpolateMode.toAny();
    a["xWrapMode"]       = xWrapMode.toAny();
    a["yWrapMode"]       = yWrapMode.toAny();
    a["maxAnisotropy"]   = maxAnisotropy;
    a["maxMipMap"]       = maxMipMap;
    a["minMipMap"]       = minMipMap;
    a["mipBias"]         = mipBias;
    return a;
}


Sampler::Sampler(const Any& any) {
    *this = Sampler::defaults();
    any.verifyNameBeginsWith("Sampler");
    if (any.type() == Any::TABLE) {
        AnyTableReader r(any);
        r.getIfPresent("maxAnisotropy", maxAnisotropy);
        r.getIfPresent("maxMipMap", maxMipMap);
        r.getIfPresent("minMipMap", minMipMap);
        r.getIfPresent("mipBias", mipBias);
        r.getIfPresent("xWrapMode", xWrapMode);
        if (! r.getIfPresent("yWrapMode", yWrapMode)) {
            yWrapMode = xWrapMode;
        }
        r.getIfPresent("depthReadMode", depthReadMode);
        r.getIfPresent("interpolateMode", interpolateMode);
        r.verifyDone();
    } else {
        any.verifySize(0);
        const String& n = any.name();
        if (n == "Sampler::defaults") {
            // Done!
        } else if (n == "Sampler::buffer") {
            *this = Sampler::buffer();
        } else if (n == "Sampler::cubeMap") {
            *this = Sampler::cubeMap();
        } else if (n == "Sampler::shadow") {
            *this = Sampler::shadow();
        } else if (n == "Sampler::video") {
            *this = Sampler::video();
        } else {
            any.verify(false, "Unrecognized name for Sampler constructor or factory method.");
        }
    }
}


Sampler::Sampler(WrapMode wrapMode, InterpolateMode interpolateMode) : 
    interpolateMode(interpolateMode),
    xWrapMode(wrapMode),
    yWrapMode(wrapMode),
    depthReadMode(DepthReadMode::DEPTH_NORMAL),
    maxAnisotropy(4.0),
    maxMipMap(1000),
    minMipMap(-1000),
    mipBias(0.0f) {
}



Sampler::Sampler(WrapMode x, WrapMode y, InterpolateMode interpolateMode) : 
    interpolateMode(interpolateMode),
    xWrapMode(x),
    yWrapMode(y),
    depthReadMode(DepthReadMode::DEPTH_NORMAL),
    maxAnisotropy(4.0),
    maxMipMap(1000),
    minMipMap(-1000),
    mipBias(0.0f) {

}

const Sampler& Sampler::defaults() {
    static Sampler param;
    static shared_ptr<GLSamplerObject> cachedSamplerObject = GLSamplerObject::create(param);
    return param;
}


const Sampler& Sampler::defaultClamp() {
    static bool initialized = false;
    static Sampler param;
    if (! initialized) {
        param.xWrapMode = WrapMode::CLAMP;
        param.yWrapMode = WrapMode::CLAMP;
    }
    static shared_ptr<GLSamplerObject> cachedSamplerObject = GLSamplerObject::create(param);
    return param;
}


const Sampler& Sampler::video() {

    static bool initialized = false;
    static Sampler param;

    if (! initialized) {
        initialized = true;
        param.interpolateMode = InterpolateMode::BILINEAR_NO_MIPMAP;
        param.xWrapMode = WrapMode::CLAMP;
        param.yWrapMode = WrapMode::CLAMP;
        param.depthReadMode = DepthReadMode::DEPTH_NORMAL;
        param.maxAnisotropy = 1.0;
    }
    
    static shared_ptr<GLSamplerObject> cachedSamplerObject = GLSamplerObject::create(param);
    return param;
}


const Sampler& Sampler::buffer() {

    static bool initialized = false;
    static Sampler param;

    if (! initialized) {
        initialized = true;
        param.interpolateMode = InterpolateMode::NEAREST_NO_MIPMAP;
        param.xWrapMode = WrapMode::CLAMP;
        param.yWrapMode = WrapMode::CLAMP;
        param.depthReadMode = DepthReadMode::DEPTH_NORMAL;
        param.maxAnisotropy = 1.0;
    }
    
    static shared_ptr<GLSamplerObject> cachedSamplerObject = GLSamplerObject::create(param);
    return param;
}


const Sampler& Sampler::visualization() {

    static bool initialized = false;
    static Sampler param;

    if (! initialized) {
        initialized = true;
        param.interpolateMode = InterpolateMode::NEAREST_MIPMAP;
        param.xWrapMode = WrapMode::CLAMP;
        param.yWrapMode = WrapMode::CLAMP;
        param.depthReadMode = DepthReadMode::DEPTH_NORMAL;
        param.maxAnisotropy = 1.0;
    }
    
    static shared_ptr<GLSamplerObject> cachedSamplerObject = GLSamplerObject::create(param);
    return param;
}


const Sampler& Sampler::cubeMap() {

    static bool initialized = false;
    static Sampler param;

    if (! initialized) {
        initialized = true;
        param.interpolateMode = InterpolateMode::BILINEAR_MIPMAP;
        param.xWrapMode = WrapMode::CLAMP;
        param.yWrapMode = WrapMode::CLAMP;
        param.depthReadMode = DepthReadMode::DEPTH_NORMAL;
        param.maxAnisotropy = 1.0;
    }
    
    static shared_ptr<GLSamplerObject> cachedSamplerObject = GLSamplerObject::create(param);
    return param;
}


const Sampler& Sampler::shadow() {

    static bool initialized = false;
    static Sampler param;

    if (! initialized) {
        initialized = true;
        if (GLCaps::enumVendor() == GLCaps::ATI) {
            // ATI cards do not implement PCF for shadow maps
            param.interpolateMode = InterpolateMode::NEAREST_NO_MIPMAP;
        } else {
            param.interpolateMode = InterpolateMode::BILINEAR_NO_MIPMAP;
        }
        param.xWrapMode      = WrapMode::ZERO;
        param.yWrapMode      = WrapMode::ZERO;
        param.depthReadMode = DepthReadMode::DEPTH_LEQUAL;
        param.maxAnisotropy = 1.0;
    }
    
    static shared_ptr<GLSamplerObject> cachedSamplerObject = GLSamplerObject::create(param);
    return param;
}


const Sampler& Sampler::lightMap() {
    static Sampler param;
    static bool initialized = false;
    if (! initialized) {
        initialized = true;
        param.xWrapMode = WrapMode::CLAMP;
        param.yWrapMode = WrapMode::CLAMP;
        param.interpolateMode = InterpolateMode::TRILINEAR_MIPMAP;
        param.maxAnisotropy = 4.0f;
    }

    static shared_ptr<GLSamplerObject> cachedSamplerObject = GLSamplerObject::create(param);
    return param;
}


size_t Sampler::hashCode() const {
    return 
        (uint32)interpolateMode + 
        16 * (uint32)xWrapMode + 
        32 * (uint32)yWrapMode + 
        256 * (uint32)depthReadMode + 
        (uint32)(1024 * maxAnisotropy) +
        (uint32)(16384 * mipBias) +
        (minMipMap ^ (maxMipMap << 16));
}


bool Sampler::operator==(const Sampler& other) const {
    return 
        (interpolateMode    == other.interpolateMode) &&
        (xWrapMode          == other.xWrapMode) &&
        (yWrapMode          == other.yWrapMode) &&
        (depthReadMode      == other.depthReadMode) &&
        (maxAnisotropy      == other.maxAnisotropy) &&
        (mipBias            == other.mipBias) &&
        (maxMipMap          == other.maxMipMap) &&
        (minMipMap          == other.minMipMap);
}

} // G3D
