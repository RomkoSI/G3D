/**
   \file Framebuffer.cpp

   \author Daniel Hilferty, djhilferty@users.sourceforge.net
   \maintainer Morgan McGuire
   
   Notes:
   http://www.opengl.org/registry/specs/ARB/framebuffer_object.txt
      
   \created 2006-01-07
   \edited  2015-09-07
*/

#include "GLG3D/Framebuffer.h"
#include "GLG3D/glcalls.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/RenderDevice.h"

namespace G3D {

Framebuffer::Framebuffer(
    const String&  name, 
    GLuint              framebufferID) : 
    m_name(name),
    m_framebufferID(framebufferID),
    m_noAttachment(false) {

}


Framebuffer::~Framebuffer () {
    if (m_framebufferID != 0) {
        glDeleteFramebuffers(1, &m_framebufferID);
        m_framebufferID = 0;
    }
}


int Framebuffer::stencilBits() const {
    shared_ptr<Attachment> d  = get(Framebuffer::DEPTH);
    shared_ptr<Attachment> s  = get(Framebuffer::STENCIL);
    shared_ptr<Attachment> ds = get(Framebuffer::DEPTH_AND_STENCIL);

    int stencilBits = 0;
    if (d) {
        stencilBits = max(stencilBits, d->texture()->format()->stencilBits);
    }
    if (s) {
        stencilBits = max(stencilBits, s->texture()->format()->stencilBits);
    }
    if (ds) {
        stencilBits = max(stencilBits, ds->texture()->format()->stencilBits);
    }
    return stencilBits;
}


shared_ptr<Framebuffer> Framebuffer::create(const shared_ptr<Texture>& t0, const shared_ptr<Texture>& t1) {
    shared_ptr<Framebuffer> f = create(t0->name() + " framebuffer");

    if (t0) {
        if (t0->format()->depthBits > 0) {
            if (t0->format()->stencilBits > 0) {
                f->set(DEPTH_AND_STENCIL, t0);
            } else {
                f->set(DEPTH, t0);
            }
        } else {
            f->set(COLOR0, t0);
        }
    }

    if (t1) {
        if (t1->format()->depthBits > 0) {
            if (t1->format()->stencilBits > 0) {
                f->set(DEPTH_AND_STENCIL, t1);
            } else {
                f->set(DEPTH, t1);
            }
        } else {
            f->set(COLOR1, t1);
        }
    }

    return f;
}


shared_ptr<Framebuffer> Framebuffer::createWithoutAttachments(const String& _name, Vector2 res, int numLayers, int numSamples, bool fixedSamplesLocation) {
	alwaysAssertM( GLCaps::supports("GL_ARB_framebuffer_no_attachments"), "" );
	alwaysAssertM(numLayers>0, "");
	alwaysAssertM(numSamples>0, "");

    shared_ptr<Framebuffer> f = create(_name);

	GLuint _framebufferID=f->openGLID();
    glNamedFramebufferParameteriEXT(_framebufferID, GL_FRAMEBUFFER_DEFAULT_WIDTH,	(int)res.x);
	glNamedFramebufferParameteriEXT(_framebufferID, GL_FRAMEBUFFER_DEFAULT_HEIGHT,	(int)res.y);
	glNamedFramebufferParameteriEXT(_framebufferID, GL_FRAMEBUFFER_DEFAULT_LAYERS,	numLayers);
	glNamedFramebufferParameteriEXT(_framebufferID, GL_FRAMEBUFFER_DEFAULT_SAMPLES, numSamples);
	glNamedFramebufferParameteriEXT(_framebufferID, GL_FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS, int(fixedSamplesLocation));
	
	f->set( NO_ATTACHMENT, shared_ptr<Attachment>(new Attachment(NO_ATTACHMENT, (int)res.x, (int)res.y, numLayers, numSamples, fixedSamplesLocation)) );

	f->m_noAttachment = true;

    return f;
}

shared_ptr<Framebuffer> Framebuffer::create(const String& _name) {
    GLuint _framebufferID;
    
    // Generate Framebuffer
    glGenFramebuffers(1, &_framebufferID);

    return shared_ptr<Framebuffer>(new Framebuffer(_name, _framebufferID));
}


bool Framebuffer::has(AttachmentPoint ap) const {
    bool found = false;
    find(ap, found);
    return found;
}


int Framebuffer::find(AttachmentPoint ap, bool& found) const {
    for (int i = 0; i < m_desired.size(); ++i) {
        if (m_desired[i]->m_point >= ap) {
            found = (m_desired[i]->m_point == ap);
            return i;
        }
    }

    found = false;
    return m_desired.size();
}


int Framebuffer::findCurrent(AttachmentPoint ap) const {
    for (int i = 0; i < m_current.size(); ++i) {
        if (m_current[i]->m_point >= ap) {
            return i;
        }
    }

    return m_current.size();
}


void Framebuffer::set(AttachmentPoint ap, const void* n) {
    (void)n;
    debugAssert(n == NULL);

    bool found = false;
    int i = find(ap, found);
    if (found) {
        m_desired.remove(i);
        m_currentOutOfSync = true;
    }
}


void Framebuffer::set(AttachmentPoint ap, const shared_ptr<Attachment>& attachment) {
	alwaysAssertM(m_noAttachment==false, "Can't set attachment to a Frambuffer in no-attachment mode");

    shared_ptr<Attachment> dst(new Attachment(*attachment));
    dst->m_point = ap;
    set(dst);
}


void Framebuffer::set(AttachmentPoint ap, const shared_ptr<Texture>& texture) {
	alwaysAssertM(m_noAttachment==false, "Can't set attachment to a Frambuffer in no-attachment mode");

    set(ap, texture, CubeFace::NEG_X, 0);
}

   
void Framebuffer::set(AttachmentPoint ap, const shared_ptr<Texture>& texture, CubeFace face, int mipLevel) {
	alwaysAssertM(m_noAttachment==false, "Can't set attachment to a Frambuffer in no-attachment mode");

    if (isNull(texture)) {
        // We're in the wrong overload due to C++ static type dispatch
        set(ap, NULL);
        return;
    }
    
    shared_ptr<Attachment> a = get(ap);
    if (isNull(a) || ! (a->equals(texture, face, mipLevel))) {
        // This is a change
        set(shared_ptr<Attachment>(new Attachment(ap, texture, face, mipLevel)));
    }
}

void Framebuffer::set(const shared_ptr<Attachment>& a) {
	alwaysAssertM(m_noAttachment==false, "Can't set attachment to a Frambuffer in no-attachment mode");

    bool found = false;
    int i = find(a->point(), found);
    if (found) {
        m_desired[i] = a;
    } else {
        m_desired.insert(i, a);
    }
    m_currentOutOfSync = true;
}


shared_ptr<Framebuffer::Attachment> Framebuffer::get(AttachmentPoint ap) const {
    bool found = false;
    int i = find(ap, found);
    if (! found) {
        return shared_ptr<Attachment>();
    } else {
        return m_desired[i];
    }
}


bool Framebuffer::bind(bool alreadyBound, Mode m) {
    if (! alreadyBound) {
        glBindFramebuffer(GLenum(m), openGLID());
    }

    if (m_currentOutOfSync) {
        sync();
        return true;
    } else {
        return false;
    }
}


void Framebuffer::bindWindowBuffer(Mode m) {
    glBindFramebuffer(GLenum(m), 0);
}


void Framebuffer::clear() {
    m_desired.clear();
    m_currentOutOfSync = true;
}


void Framebuffer::sync() {
    debugAssert(m_currentOutOfSync);
    int d = 0;
    int c = 0;

    // Walk both the desired and current arrays
    while ((d < m_desired.size()) && (c < m_current.size())) {
        // Cannot be references since the code below mutates the
        // m_desired and m_current arrays.
        const shared_ptr<Attachment> da = m_desired[d];
        const shared_ptr<Attachment> ca = m_current[c];

        if (da->equals(ca)) {
            // Matched; nothing to do
            ++d;
            ++c;
        } else {
            // Mismatch
            if (da->point() >= ca->point()) {
                // Remove current.  Don't change 
                // c index because we just shrank m_current.
                detach(ca);
            }
            
            if (da->point() <= ca->point()) {
                // Add desired
                attach(da);
                ++d;

                // Increment c, since we just inserted at it or earlier
                ++c;
            }
        }
    }

    // Only one of the following two loops will execute. This one must be first because
    // it assumes that c is unchanged from the previous loop.
    while (c < m_current.size()) {
        // Don't change c index because we just shrank m_current.
        detach(m_current[c]);
    }

    while (d < m_desired.size()) {
        attach(m_desired[d]);
        ++d;
    }

    // We are now in sync
    m_currentOutOfSync = false;

    debugAssert(m_current.length() == m_desired.length());

    debugAssert(
        (m_colorDrawBufferArray.length() >= m_current.length() - 2) && 
        (m_colorDrawBufferArray.length() <= m_current.length()));
}


void Framebuffer::attach(const shared_ptr<Attachment>& a) {
    const int cIndex = findCurrent(a->point());
    if ((a->point() >= COLOR0) && (a->point() <= COLOR15)) {
        const GLenum e(a->point());
        const int index = int(e) - GL_COLOR_ATTACHMENT0;

        if (m_colorDrawBufferArray.length() <= index) {
            // Resize, clearing the newly revealed values
            const int oldSize = m_colorDrawBufferArray.length();
            m_colorDrawBufferArray.resize(index + 1);
            for (int i = oldSize; i < m_colorDrawBufferArray.size() - 1; ++i) {
                m_colorDrawBufferArray[i] = GL_NONE;
            }
        }

        m_colorDrawBufferArray[index] = e;
    }
    m_current.insert(cIndex, a);
    a->attach();
}


void Framebuffer::detach(shared_ptr<Attachment> a) {
    const GLenum e(a->point());
    const int cIndex = findCurrent(a->point());

    if ((e >= COLOR0) && (e <= COLOR15)) {
        const int index = int(e) - GL_COLOR_ATTACHMENT0;

        // Unmap this element
        m_colorDrawBufferArray[index] = GL_NONE;

        // If we just removed the last element
        if (index == m_colorDrawBufferArray.size() - 1) {
            // Collapse any empty entries to shrink the array to fit
            int newSize = m_colorDrawBufferArray.size();

            while ((newSize > 0) && (m_colorDrawBufferArray[newSize - 1] == GL_NONE)) {
                --newSize;
            }

            m_colorDrawBufferArray.resize(newSize);
        }
    }

    m_current.remove(cIndex);
    a->detach();
}


int Framebuffer::width() const {
    if (m_desired.size() > 0) {
        return m_desired[0]->width();
    } else { 
        return 0;
    }
}
 

int Framebuffer::height() const {
    if (m_desired.size() > 0) {
        return m_desired[0]->height();
    } else { 
        return 0;
    }
}

    
Rect2D Framebuffer::rect2DBounds() const {
    if (m_desired.size() > 0) {
        return Rect2D::xywh(Vector2::zero(), m_desired[0]->vector2Bounds());
    } else {
        return Rect2D::xywh(0,0,0,0);
    }
}

    
Vector2 Framebuffer::vector2Bounds() const {
    if (m_desired.size() > 0) {
        return m_desired[0]->vector2Bounds();
    } else {
        return Vector2::zero();
    }
}


void Framebuffer::resize(Framebuffer::AttachmentPoint ap, int w, int h){
	get(ap)->resize(w, h);

	if (ap == NO_ATTACHMENT) {
		glNamedFramebufferParameteriEXT(m_framebufferID, GL_FRAMEBUFFER_DEFAULT_WIDTH,	w);
		glNamedFramebufferParameteriEXT(m_framebufferID, GL_FRAMEBUFFER_DEFAULT_HEIGHT,	h);
	}
}


void Framebuffer::resize(int w, int h){
    for (int i = 0; i < m_desired.size(); ++i) {
        resize(m_desired[i]->m_point, w, h);
    }
}


void Framebuffer::blitTo
   (RenderDevice* rd,
    const shared_ptr<Framebuffer>& dst,
    bool invertY,
    bool linearInterpolation,
    bool blitDepth,
    bool blitStencil,
    bool blitColor) const {

    const int w = width();
    const int h = height();

    const shared_ptr<Framebuffer>& oldDrawFramebuffer = rd->drawFramebuffer();
    const shared_ptr<Framebuffer>& oldReadFramebuffer = rd->readFramebuffer();
    const Rect2D oldClip = rd->clip2D();

    rd->setDrawFramebuffer(dst);
    rd->setClip2D(Rect2D::inf());
    rd->setReadFramebuffer(dynamic_pointer_cast<Framebuffer>(const_cast<Framebuffer*>(this)->shared_from_this()));

    GLenum flags = 0;
    if (blitDepth)   { flags |= GL_DEPTH_BUFFER_BIT;   }
    if (blitStencil) { flags |= GL_STENCIL_BUFFER_BIT; }
    if (blitColor)   { flags |= GL_COLOR_BUFFER_BIT;   }

    debugAssertM(!(linearInterpolation && (blitDepth || blitStencil)), "if blit depth or stencil is enabled Nearest inpterpolation must be used");
    if (notNull(dst)) {
        debugAssertM((! blitDepth) || (dst->has(Framebuffer::DEPTH) && has(Framebuffer::DEPTH)), "To perform a depth blit both the source and destination Framebuffers must have a depth buffer attached");
        debugAssertM((! blitStencil) || (dst->has(Framebuffer::STENCIL) && has(Framebuffer::STENCIL)), "To perform a stencil blit both the source and destination Framebuffers must have a stencil buffer attached");
        debugAssertM((! blitColor) || (dst->has(Framebuffer::COLOR0) && has(Framebuffer::COLOR0)), "To perform a color blit both the source and destination Framebuffers must have a color buffer attached");
    }

    glBlitFramebuffer(0, invertY ? h : 0, w, invertY ? 0 : h, 0, 0, w, h, flags, linearInterpolation ? GL_LINEAR : GL_NEAREST);
    debugAssertGLOk();

    rd->setReadFramebuffer(oldReadFramebuffer);
    rd->setDrawFramebuffer(oldDrawFramebuffer);
    rd->setClip2D(oldClip);
}

/*void Framebuffer::resize(int w, int h){
	
	for (int i = 0; i < m_desired.size(); ++i) {
		m_desired[i]->resize(w, h);
	}

	if(m_noAttachment){
		glNamedFramebufferParameteriEXT(m_framebufferID, GL_FRAMEBUFFER_DEFAULT_WIDTH,	w);
		glNamedFramebufferParameteriEXT(m_framebufferID, GL_FRAMEBUFFER_DEFAULT_HEIGHT,	h);
	} 
		
	m_currentOutOfSync = true;
}*/

////////////////////////////////////////////////////////////////////

Framebuffer::Attachment::Attachment(AttachmentPoint ap, const shared_ptr<Texture>& r, CubeFace c, int m) : 
    m_clearValue(0.0f, 0.0f, 0.0f),
    m_type(TEXTURE), 
    m_point(ap),
    m_texture(r),
    m_cubeFace(c),
    m_mipLevel(m) {

    alwaysAssertM(ap != DEPTH || ap != DEPTH_AND_STENCIL 
                    || r->format()->depthBits > 0, 
                    "Cannot attach a texture without any depth bits to DEPTH or DEPTH_AND_STENCIL");

}

Framebuffer::Attachment::Attachment(AttachmentPoint ap, int width, int height, int numLayers, int numSamples, bool fixedSamplesLocation) : 
    m_type(DUMMY),
    m_point(ap),
    m_cubeFace(CubeFace::NEG_X),
    m_mipLevel(0),
	m_width(width),
	m_height(height),
	m_numLayers(numLayers),
	m_numSamples(numSamples),
	m_fixedSamplesLocation(fixedSamplesLocation){
        alwaysAssertM( ap == NO_ATTACHMENT , 
                    "Dummy attachment has to be attached to NO_ATTACHMENT");
}
 
Vector2 Framebuffer::Attachment::vector2Bounds() const {
    if (m_type == TEXTURE) {
        return m_texture->vector2Bounds();
    } else {
		return Vector2((float)m_width, (float)m_height);
	}
}


int Framebuffer::Attachment::width() const {
    if (m_type == TEXTURE) {
        return m_texture->width();
    } else {
		return m_width;
	}
}
    

int Framebuffer::Attachment::height() const {
    if (m_type == TEXTURE) {
        return m_texture->height();
    } else {
		return m_height;
	}
}

void Framebuffer::Attachment::resize(int w, int h){
	if (m_type == TEXTURE) {
        m_texture->resize(w, h);
    } else  {
        m_width=w;
		m_height=h;
    }
}

bool Framebuffer::Attachment::equals(const shared_ptr<Texture>& t, CubeFace c, int L) const {
    return
        (m_type == TEXTURE) &&
        (m_texture == t) &&
        (m_cubeFace == c) &&
        (m_mipLevel == L);
}


bool Framebuffer::Attachment::equals(const shared_ptr<Attachment>& other) const {
    return
        (m_type == other->m_type) &&
        (m_point == other->m_point) &&
        (m_texture == other->m_texture) &&
        (m_cubeFace == other->m_cubeFace) &&
        (m_mipLevel == other->m_mipLevel);
}


void Framebuffer::Attachment::attach() const {
	debugAssertGLOk();

    if (m_type == TEXTURE) {
        const bool isCubeMap = (m_texture->dimension() == Texture::DIM_CUBE_MAP);
        const bool is2DArray = (m_texture->dimension() == Texture::DIM_2D_ARRAY);
        if (isCubeMap) {
            glFramebufferTexture2D
                (GL_FRAMEBUFFER, GLenum(m_point),
                 GL_TEXTURE_CUBE_MAP_POSITIVE_X + (int)m_cubeFace, m_texture->openGLID(), m_mipLevel);
        } else if (is2DArray) {
            glFramebufferTexture
                (GL_FRAMEBUFFER, GLenum(m_point), 
                m_texture->openGLID(), m_mipLevel);
        } else {
            glFramebufferTexture2D
                (GL_FRAMEBUFFER, GLenum(m_point),
                 m_texture->openGLTextureTarget(), 
                 m_texture->openGLID(), m_mipLevel);
        }
    } 

	debugAssertGLOk();
}


void Framebuffer::Attachment::detach() const {
    if (m_type == TEXTURE) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GLenum(m_point), GL_TEXTURE_2D, 0, 0);
    } 
}


const ImageFormat* Framebuffer::Attachment::format() const {
    if (m_type == TEXTURE) {
        return m_texture->format();
    } else {
        alwaysAssertM( false, "No format available for zero attachment framebuffers" );
        return NULL;
    }
}

}// G3D
