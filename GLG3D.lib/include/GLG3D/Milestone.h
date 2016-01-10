/**
  \file GLG3D/Milestone.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2003-08-09
  \edited  2012-11-19
*/

#ifndef GLG3D_Milestone_h
#define GLG3D_Milestone_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "G3D/Array.h"
#include "GLG3D/glheaders.h"

namespace G3D {

/**
 Used to set a marker in the GPU instruction stream that
 the CPU can later query <i>or</i> block on.

 Abstracts the OpenGL ARB_sync/OpenGL 3.2 API 
 (http://www.opengl.org/registry/specs/ARB/sync.txt)
 that replaced the older NV_fence (http://oss.sgi.com/projects/ogl-sample/registry/NV/fence.txt) API.

 G3D names this class 'milestone' instead of 'fence' 
 to clarify that it is not a barrier--the CPU and GPU can
 both pass a milestone without blocking unless the CPU explicitly
 requests a block.
 */
class Milestone : public ReferenceCountedObject {
private:

    friend class RenderDevice;
    
    GLsync                  m_glSync;
    String             m_name;
            
    // The methods are private because in the future
    // we may want to move parts of the implementation
    // into RenderDevice.

    Milestone(const String& n);
    
    /** Wait for it to be reached. 
    \returnglClientWaitSync result */
    GLenum _wait(float timeout) const;

public:
    /**
    Create a new Milestone (GL fence) and insert it into the GPU execution queue.
    This must be called on a thread that has the current OpenGL Context */
    static shared_ptr<Milestone> create(const String& name) {
        return  shared_ptr<Milestone>(new Milestone(name));
    }

    ~Milestone();

    inline const String& name() const {
        return m_name;
    }

    /**
     Stalls the CPU until the GPU has finished the milestone.

     \return false if timed out, true if the milestone completed.

     \sa  completed
     */
    bool wait(float timeout = finf());

    /** Returns true if this milestone has completed execution.
        \sa  wait */
    bool completed() const;
};


}

#endif
