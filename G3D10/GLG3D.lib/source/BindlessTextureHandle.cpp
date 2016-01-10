/**
\file GLG3D/BindlessHandle.cpp

\maintainer Michael Mara, http://www.illuminationcodified.com

\created 2015-07-13
\edited  2015-07-13

Copyright 2000-2015, Morgan McGuire.
All rights reserved.
*/
#include "GLG3D/BindlessTextureHandle.h"
#include "GLG3D/Texture.h"
#include "GLG3D/GLSamplerObject.h"
#include "GLG3D/glcalls.h"
#include "GLG3D/GLCaps.h"

namespace G3D {

    BindlessTextureHandle::BindlessTextureHandle() : m_GLHandle(0) {}
    BindlessTextureHandle::~BindlessTextureHandle() {
        if (isValid() && isResident()) {
            makeNonResident();
        }
    }
    BindlessTextureHandle::BindlessTextureHandle(shared_ptr<Texture> tex, const Sampler& sampler) : m_GLHandle(0) {
        set(tex, sampler);
    }

    void BindlessTextureHandle::set(shared_ptr<Texture> tex, const Sampler& sampler) {
        debugAssertM(GLCaps::supports("GL_ARB_bindless_texture"), "GL_ARB_bindless_texture not supported, cannot use BindlessTextureHandle");
        if (isValid() && isResident()) {
            makeNonResident();
        }
        m_texture = tex;
        m_samplerObject = GLSamplerObject::create(sampler);
        m_GLHandle = glGetTextureSamplerHandleARB(tex->openGLID(), m_samplerObject->openGLID());
        debugAssertGLOk();
        alwaysAssertM(m_GLHandle != 0, "BindlessTextureHandle was unable to create a proper handle");
        makeResident();
    }


    bool BindlessTextureHandle::isResident() const {
        return isValid() && glIsTextureHandleResidentARB(m_GLHandle);
    }

    void BindlessTextureHandle::makeResident() {
        alwaysAssertM(isValid(), "Attempted to makeResident BindlessTextureHandle");
        if (!isResident()) {
            glMakeTextureHandleResidentARB(m_GLHandle);
            debugAssertGLOk();
        }
    }

    void BindlessTextureHandle::makeNonResident() {
        if (isValid() && isResident()) {
            glMakeTextureHandleNonResidentARB(m_GLHandle);
            debugAssertGLOk();
        }
    }

}