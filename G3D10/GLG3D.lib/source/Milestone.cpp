/**
  \file Milestone.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2003-08-09
  \edited  2011-11-19
*/

#include "GLG3D/Milestone.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/GLCaps.h"

namespace G3D {


Milestone::Milestone(const String& n) : m_glSync(GL_NONE), m_name(n) {
    m_glSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}


Milestone::~Milestone() {
    glDeleteSync(m_glSync);
    m_glSync = GL_NONE;
}


GLenum Milestone::_wait(float timeout) const {
    const uint64 t = iFloor(min(timeout, 100000.0f) * 1000);
    GLenum result = glClientWaitSync(m_glSync, 0, t);
    debugAssertM(result != GL_WAIT_FAILED, "glClientWaitSync failed");

    return result;
}


bool Milestone::completed() const {
    GLenum e = _wait(0);
    return (e == GL_ALREADY_SIGNALED) || (e == GL_CONDITION_SATISFIED);
}


bool Milestone::wait(float timeout) {
    return (_wait(timeout) != GL_TIMEOUT_EXPIRED);
}

} // namespace G3D
