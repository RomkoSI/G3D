/**
 \file GLG3D/Framebuffer.h

 \maintainer Morgan McGuire

 \cite Initial implementation by Daniel Hilferty, djhilferty@users.sourceforge.net
 \cite http://oss.sgi.com/projects/ogl-sample/registry/EXT/framebuffer_object.txt

 \created 2006-01-07
 \edited  2014-07-17

  Copyright 2000-2015, Morgan McGuire
*/

#ifndef GLG3D_Framebuffer_h
#define GLG3D_Framebuffer_h

#include "G3D/platform.h"
#include "G3D/Array.h"
#include "G3D/Table.h"
#include "G3D/Rect2D.h"
#include "GLG3D/Texture.h"
#include "GLG3D/UniformTable.h"
#include <string.h>

namespace G3D {

/**
 \brief Holds a set of G3D::Textures for use as draw targets.
 
 Abstraction of OpenGL's Framebuffer Object.  This is an efficient way
 of rendering to Textures. 

 RenderDevice::setFramebuffer automatically configures the appropriate
 OpenGL draw buffers.  These are maintained even if the frame buffer
 is changed while set on the RenderDevice.  Inside a pixel shader
 gl_FragData[i] is the ith attached buffer, in number order.  For
 example, if there are attachments to buffer0 and buffer2, then
 gl_FragData[0] maps to buffer0 and gl_FragData[1] maps to buffer2.


 Basic Framebuffer Theory:
    Every OpenGL program has at least one Framebuffer.  This framebuffer is
 setup by the windowing system and its image format is that specified by the
 OS.  With the Framebuffer Object extension, OpenGL gives the developer
 the ability to create offscreen framebuffers that can be used to render 
 to textures of any specified format.

    The Framebuffer class is used in conjunction with the RenderDevice to
 set a render target.  The RenderDevice method setFramebuffer performs this
 action.  If a NULL argument is passed to setFramebuffer, the render target
 defaults to the window display framebuffer.

 The following example shows how to create a texture and bind it to Framebuffer
 for rendering.

 Framebuffer Example:

 \code
    // Create Texture
    static shared_ptr<Texture> tex = Texture::createEmpty(256, 256, "Rendered Texture", ImageFormat::RGB8, Texture::CLAMP, Texture::NEAREST_NO_MIPMAP, Texture::DIM_2D);

    // Create a framebuffer that uses this texture as the color buffer
    static shared_ptr<Framebuffer> fb = Framebuffer::create("Offscreen target");
    bool init = false;

    if (! init) {
        fb->set(Framebuffer::COLOR0, tex);
        init = true;
    }

    rd->pushState();
        rd->setFramebuffer(fb);
        rd->push2D(fb->rect2DBounds());

            // Set framebuffer as the render target

            // Draw on the texture
            Draw::rect2D(Rect2D::xywh(0,0,128,256), rd, Color3::white());
            Draw::rect2D(Rect2D::xywh(128,0,128,256), rd, Color3::red());

            // Restore renderdevice state (old frame buffer)
        rd->pop2D();
    rd->popState();

    app->renderDevice->setProjectionAndCameraMatrix(app->debugCamera);

    // Remove the texture from the framebuffer
    //    fb->set(Framebuffer::COLOR0, NULL);

    // Can now render from the texture

    
    // Cyan background
    app->renderDevice->setColorClearValue(Color3(.1f, .5f, 1));
    app->renderDevice->clear();

    Draw::rect2D(Rect2D::xywh(10,10,256,256), app->renderDevice, Color3::white(), tex);
 \endcode

 Note: Not any combination of images may be attached to a Framebuffer.
 OpenGL lays out some restrictions that must be considered:

 <ol>
   <li> In order to render to a Framebuffer, there must be at least
   one image (Texture) attached to an attachment point.
    
   
   <li> All images attached to a COLOR_ATTACHMENT[n] point must have
   the same internal format (RGBA8, RGBA16...etc)
   
   <li> If RenderDevice->setDrawBuffer is used then the specified 
   attachment point must have a bound image.
    
   <li> The combination of internal formats of attached images does not
   violate some implementation-dependent set of restrictions (i.e., Your
   graphics card must completely implement all combinations that you
   plan to use!)
 </ol>

*/
class Framebuffer : public ReferenceCountedObject {
public:
    /**
       Specifies which channels of the framebuffer the texture will 
       define. These mirror
       the OpenGL definition as do their values.
       
       A DEPTH_STENCIL format texture can be attached to either the 
       DEPTH_ATTACHMENT or the STENCIL_ATTACHMENT, or both simultaneously; Framebuffer will
       understand the format and use the appropriate channels.      
    */
    enum AttachmentPoint {
        COLOR0 = GL_COLOR_ATTACHMENT0,  // = 0x8CE0, so all color attachments have lower value than depth
        COLOR1 = GL_COLOR_ATTACHMENT1,  // OpenGL assures these 16 will have sequential values  
        COLOR2 = GL_COLOR_ATTACHMENT2,
        COLOR3 = GL_COLOR_ATTACHMENT3,
        COLOR4 = GL_COLOR_ATTACHMENT4,
        COLOR5 = GL_COLOR_ATTACHMENT5,
        COLOR6 = GL_COLOR_ATTACHMENT6,
        COLOR7 = GL_COLOR_ATTACHMENT7,
        COLOR8 = GL_COLOR_ATTACHMENT8,
        COLOR9 = GL_COLOR_ATTACHMENT9,
        COLOR10 = GL_COLOR_ATTACHMENT10,
        COLOR11 = GL_COLOR_ATTACHMENT11,
        COLOR12 = GL_COLOR_ATTACHMENT12,
        COLOR13 = GL_COLOR_ATTACHMENT13,
        COLOR14 = GL_COLOR_ATTACHMENT14,
        COLOR15 = GL_COLOR_ATTACHMENT15,

        DEPTH = GL_DEPTH_ATTACHMENT,

        STENCIL = GL_STENCIL_ATTACHMENT,
        
        DEPTH_AND_STENCIL = GL_DEPTH_STENCIL_ATTACHMENT,

		NO_ATTACHMENT,				// ARB_framebuffer_no_attachments, not yet in core
    };

    /** \sa bind() */
    enum Mode {
        MODE_READ      = GL_READ_FRAMEBUFFER,
        MODE_DRAW      = GL_DRAW_FRAMEBUFFER,
        MODE_READ_DRAW = GL_FRAMEBUFFER
    };

    class Attachment : public ReferenceCountedObject {
    public:
        friend class Framebuffer;

        enum Type {TEXTURE,
            /** DUMMY attachment is used as a proxy for 
				framebuffer parameters (resolution, num layers, 
				etc.) when using no-attachment FBO.
            */
            DUMMY};	

    private:

        Color4                      m_clearValue;

        Type                        m_type;

        AttachmentPoint             m_point;

        shared_ptr<Texture>         m_texture;

        /** If texture is a CubeFace::MAP, this is the face that
            is attached. */
        CubeFace                    m_cubeFace;
        
        /** Mip level being rendered to */
        int                         m_mipLevel;

		/** These parameters are used only for DUMMY attachment, which 
			is used when the framebuffer is in no-attachment mode. 
			Dummy attahcment do not have any texture 
			associated, and thus have to keep parameters here. */
		int                         m_width;
		int                         m_height;
		int							m_numLayers;
		int							m_numSamples;
		bool						m_fixedSamplesLocation;

        Attachment(AttachmentPoint ap, const shared_ptr<Texture>& r, CubeFace c, int miplevel);
        
		/** Dummy attachment */
		Attachment(AttachmentPoint ap, int width, int height, int numLayers, int numSamples, bool fixedSamplesLocation);

        /** Assumes the point is correct */
        bool equals(const shared_ptr<Texture>& t, CubeFace f, int miplevel) const;

        bool equals(const shared_ptr<Attachment>& other) const;

        /** Called from sync() to actually force this to be attached
            at the OpenGL level.  Assumes the framebuffer is already
            bound.*/
        void attach() const;

        /** Called from sync() to actually force this to be detached
            at the OpenGL level.  Assumes the framebuffer is already
            bound.*/
        void detach() const;

    public:
       
        inline Type type() const {
            return m_type;
        } 

        inline AttachmentPoint point() const {
            return m_point;
        }

        inline const shared_ptr<Texture>& texture() const {
            return m_texture;
        }

        inline CubeFace cubeFace() const {
            return m_cubeFace;
        }

        inline int mipLevel() const {
            return m_mipLevel; 
        }

        const ImageFormat* format() const;

        Vector2 vector2Bounds() const;

        int width() const;
        int height() const;

		void resize(int w, int h);
    };
    
protected:

    /** Framebuffer name */
    String                          m_name;

    /** True when desiredAttachment != m_currentAttachment. 
        Set to true by set().  Set to false by sync(), which is
        called by RenderDevice::sync().
     */
    bool                            m_currentOutOfSync;

    /** What should be attached on this framebuffer, according to
        set() calls that have been made. */
    Array< shared_ptr<Attachment> > m_desired;

    /** What is actually attached on this framebuffer as far as OpenGL
        is concerned. */
    Array< shared_ptr<Attachment> > m_current;

    /** The GL buffer names of the m_currentAttachment.*/
    Array<GLenum>					m_colorDrawBufferArray;

    /** OpenGL Object ID */
    GLuint                          m_framebufferID;
    
	/** This a a special framebuffer with no attachment */
	bool							m_noAttachment;

    /** Adds \a a to m_desired. */
    void set(const shared_ptr<Attachment>& a);

    Framebuffer(const String& name, GLuint framebufferID);
    
    /** Returns the index in m_desired where ap is, or
        where it should be inserted if it is not present.*/
    int find(AttachmentPoint ap, bool& found) const;

    int findCurrent(AttachmentPoint ap) const;

    /** Executes the synchronization portion of bind() */
    void sync();

    /** Called from sync() to actually force \a a to be attached
        at the OpenGL level.  Assumes the framebuffer is already
        bound.
    */
    void attach(const shared_ptr<Attachment>& a);

    /** Called from sync() to actually force \a a to be detached
        at the OpenGL level.  Assumes the framebuffer is already
        bound.

        Argument can't be a & reference because it is being removed 
        from the only place holding it.
    */
    void detach(shared_ptr<Attachment> a);

public:

    /** Arguments that RenderDevice will appended to the Shader G3D::Args before
        invoking any draw call that targets this frame buffer. */
    UniformTable    uniformTable;

    /** Returns the number of stencil bits for currently attached STENCIL and DEPTH_AND_STENCIL attachments.*/
    int stencilBits() const;

    /** Creates a Framebuffer object.
       
       \param name Name of framebuffer, for debugging purposes. */
    static shared_ptr<Framebuffer> create(const String& name);

    /** Creates a Framebuffer object and binds the argument to
        Framebuffer::DEPTH_AND_STENCIL if it has depth bits or
        Framebuffer::COLOR0 otherwise.
     */
    static shared_ptr<Framebuffer> create(const shared_ptr<Texture>& tex0, const shared_ptr<Texture>& tex1 = shared_ptr<Texture>());

	/** Creates a Framebuffer object with no attachments.
		Requires ARB_framebuffer_no_attachments.
     */
	static shared_ptr<Framebuffer> createWithoutAttachments(const String& _name, Vector2 res, int numLayers, int numSamples=1, bool fixedSamplesLocation=true);

    /** Bind this framebuffer and force all of its attachments to
        actually be attached at the OpenGL level.  The latter step is
        needed because set() is lazy.

        <b>Primarily used by RenderDevice.  Developers should not need
        to explicitly call this method or glDrawBuffers.</b>

        After binding, you also have to set the glDrawBuffers to match
        the capabilities of the Framebuffer that is currently bound.

        \param alreadyBound If true, do not bother binding the FBO
        itself, just sync any out of date attachments.

        \return True if openGLDrawArray() was changed by this call
    */
    bool bind(bool alreadyBound = false, Mode m = MODE_READ_DRAW);

    /** Bind the current context's default Framebuffer, instead of an
        application-created one. 

        <b>Primarily used by RenderDevice.  Developers should not need
        to explicitly call this method or glDrawBuffers.</b>
    */
    static void bindWindowBuffer(Mode m = MODE_READ_DRAW);

    /** Returns the attachment currently at ap, or NULL if there is
     not one.  \sa has()*/
    shared_ptr<Attachment> get(AttachmentPoint ap) const;

    /**
     Number of currently bound attachments.  When this hits zero we can
     add attachments with new sizes.
     */
    inline int numAttachments() const {
        return m_desired.size();
    }

    /** The draw array for use with glDrawBuffers. This is not up to
        date until bind() is invoked.

        Note that DEPTH, STENCIL, and DEPTH_AND_STENCIL are never
        included in this list.

        RenderDevice automatically uses this.*/
    inline const Array<GLenum>& openGLDrawArray() const {
        return m_colorDrawBufferArray;
    }
    
    /** Reclaims OpenGL ID.  All buffers/textures are automatically
        detatched on destruction. */
    ~Framebuffer();
       
    /** Overload used when setting attachment points to NULL */
    void set(AttachmentPoint ap, const void* n);
    
    /** Sets the texture to be the output render target with \a location = \a ap.*/
    void set(AttachmentPoint ap, const shared_ptr<Texture>& texture);

    /** Used for copying Attachment%s from another Framebuffer.
        Ignores the \a attachment.point. */
    void set(AttachmentPoint ap, const shared_ptr<Attachment>& attachment);
    
    /**
       Set one of the attachment points to reference a Texture.  Set
       to NULL or call clear() to unset.  Auto-mipmap will
       automatically be disabled on set.
       
       Results are undefined if a texture that is bound to the
       <b>current</b> Framebuffer as a source texture while it is
       being read from.  In general, create one Framebuffer per set of
       textures you wish to render to and just leave them bound at all
       times.

       All set() calls are lazy because OpenGL provides no mechanism
       for efficiently pushing and popping the Framebuffer. Thus all
       calls to actually set attachments must be delayed until the
       bind() call, when this Framebuffer is guaranteed to be bound.
       
       \param texture     Texture to bind to the Framebuffer.
       \param ap     Attachment point to bind texture to.
       \param mipLevel   Target MIP-map level to render to.
    */
    void set(AttachmentPoint ap, const shared_ptr<Texture>& texture, 
             CubeFace face, int mipLevel = 0);
    
    
    /** Returns true if this attachment is currently non-NULL.*/
    bool has(AttachmentPoint ap) const;
    
    /**
       The OpenGL ID of the underlying framebuffer object.
    */
    inline unsigned int openGLID() const {
        return m_framebufferID;
    }
    
    /** Read from the first attachment every time called. */
    int width() const;
    
    /** Read from the first attachment every time called. */
    int height() const;
    
    Rect2D rect2DBounds() const;
    
    Vector2 vector2Bounds() const;

    inline const String& name() const {
        return m_name;
    }

    /** Detach all attachments.  This is lazy; see set() for discussion.*/
    void clear();

	/** Resize this one attachment. */
	void resize(AttachmentPoint ap, int w, int h);

	/** Resize all attachments. */
	void resize(int w, int h);

    /** Shorthand for getting the texture for attachment point x */
	const shared_ptr<Texture>& texture(AttachmentPoint x) {
        const shared_ptr<Attachment>& a = get(x);
        if (notNull(a)) {
    		return a->texture();
        } else {
            static const shared_ptr<Texture> undefined;
            return undefined;
        }
    }

    /** Shorthand for getting the texture for color attachment point x*/
	const shared_ptr<Texture>& texture(const uint8 x) {
        debugAssertM( x < 16, format("Invalid attachment index: %d", x));
        return texture(AttachmentPoint(x + COLOR0));
	}

	/** For STENCIL, the integer in the r channel is used. For DEPTH, the float in the r channel is used. For DEPTH_AND_STENCIL, the 
	    float in r clears depth and the int in g clears stencil. */
    void setClearValue(AttachmentPoint x, const Color4& clearValue) {
        alwaysAssertM(has(x), "Cannot set the clear value of a unbound attachment point");
        get(x)->m_clearValue = clearValue;
    }

    const Color4 getClearValue(const AttachmentPoint x) const {
        alwaysAssertM(has(x), "The attachment point is not valid");
        return get(x)->m_clearValue;
    }

	/** Blits the contents of this framebuffer to \param dst, or the back buffer if dst is null. 
        If invertY is true, flip the image vertically while blitting, useful for blitting to the back buffer. */
    void blitTo
        (RenderDevice* rd,
        const shared_ptr<Framebuffer>& dst = shared_ptr<Framebuffer>(),
        bool invertY = false,
        bool linearInterpolation = false,
        bool blitDepth = true,
        bool blitStencil = true,
        bool blitColor = true) const;

}; // class Framebuffer 

} //  G3D

#endif // GLG3D_Framebuffer_h

