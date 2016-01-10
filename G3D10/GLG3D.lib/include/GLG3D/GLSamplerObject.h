/**
  \file GLG3D/GLSampler.h

  \maintainer Michael Mara, http://www.illuminationcodified.com

  \created 2013-10-03
  \edited  2013-10-10
*/

#ifndef GLG3D_GLSamplerObject_h
#define GLG3D_GLSamplerObject_h

#include "G3D/WrapMode.h"
#include "G3D/InterpolateMode.h"
#include "G3D/DepthReadMode.h"
#include "G3D/WeakCache.h"
#include "GLG3D/glheaders.h"
#include "GLG3D/Sampler.h"

namespace G3D {

class Shader;
class Texture;
/**
 \brief A class holding all of the parameters one would want to use to when accessing a Texture, and an
 associated OpenGL Sampler Object

 Abstraction of OpenGL Sampler Objects.  This class can be used with raw OpenGL, 
 without RenderDevice. 

 \sa Texture
 */
class GLSamplerObject {
 friend class Shader;
private:
    /** Used to avoid duplicate GLSamplerObjects */
    static WeakCache<Sampler, shared_ptr<GLSamplerObject> > s_cache;

    /** Contains all of the settings currently associated with the underlying OpenGl Sampler Object */
    Sampler m_sampler;

    /** OpenGL sampler ID */
    GLuint  m_glSamplerID;

    GLSamplerObject(const Sampler& settings);

    /** Set all of the OpenGL parameters to those specified in settings */
    void setParameters(const Sampler& settings);

    static void setDepthSamplerParameters(GLuint samplerID, DepthReadMode depthReadMode);

public:
    
    ~GLSamplerObject();

    static void clearCache();

    static shared_ptr<GLSamplerObject> create(const Sampler& s);

    inline unsigned int openGLID() const {
        return m_glSamplerID;
    }

    const Sampler& sampler() const {
        return m_sampler;
    }

};

} //G3D


#endif
