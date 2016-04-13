/**
  \file GLG3D/RenderDevice.h

  Graphics hardware abstraction layer (wrapper for OpenGL).

  You can freely mix OpenGL calls with RenderDevice, just make sure you put
  the state back the way you found it or you will confuse RenderDevice.

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
  \created 2001-05-29
  \edited  2014-08-19

  Copyright 2000-2015, Morgan McGuire
*/

#ifndef GLG3D_RenderDevice_h
#define GLG3D_RenderDevice_h

#include "G3D/platform.h"
#include "G3D/constants.h"
#include "G3D/Array.h"
#include "G3D/TextOutput.h"
#include "G3D/MeshAlg.h"
#include "G3D/Stopwatch.h"
#include "G3D/Matrix2.h"
#include "G3D/CullFace.h"
#include "GLG3D/Texture.h"
#include "GLG3D/VertexBuffer.h"
#include "GLG3D/OSWindow.h"
#include "GLG3D/AttributeArray.h"
#include "GLG3D/Framebuffer.h"
#include "GLG3D/Args.h"

namespace G3D {

class AttributeArray;
class Shader;
class Milestone;
class Projection;

/**
 \brief Abstraction of a graphics rendering context (GPU).  
 
 Implemented with an OpenGL context, but designed so that it can
 support other APIs (e.g., OpenGL ES, DirectX) as back ends.

 Unlike OpenGL, in release mode, no RenderDevice call will trigger a
 pipeline flush, and all redundant state calls are automatically
 detected and optimized out.  This includes queries: reading any
 RenderDevice state is instantaneous and does not flush the GPU
 (unlike glGet*).

 In future releases, fixed function state will either be removed
 or isolated in RenderDevice.  Wherever possible, structure your
 code to use Framebuffer, AttributeArray, and Shader instead of fixed-function.

 You must call RenderDevice::init() before using the RenderDevice.
  
 OpenGL is a basically good API with some rough spots.  Three of these
 are addressed by RenderDevice.  First, OpenGL state management is
 both tricky and potentially slow.  Second, OpenGL functions are
 difficult to use because many extensions have led to an evolutionary
 rather than designed API.  For type safety, new enums are introduced
 for values instead of the traditional OpenGL GLenum's, which are just
 ints.  Third, OpenGL intialization is complicated.  This interface
 simplifies it significantly.

 <P> NICEST line and point smoothing is enabled by default (however,
 you need to set your alpha blending mode to see it).

 <P> glEnable(GL_NORMALIZE) is set by default.  glEnable(GL_COLOR_MATERIAL) 
     is enabled by default.  You may be able to get a slight speed increase
     by disabling GL_NORMALIZE or using GL_SCALE_NORMAL instead.

 <p> Fixed-function code is currently being phased out of the G3D API as
 it is deprecated in OpenGL.  Shader and VertexArray code is the preferred
 rendering path.

 <P>
 Example
  <PRE>
   RenderDevice renderDevice = new RenderDevice();
   renderDevice->init(OSWindow::Settings());
  </PRE>


  G3D::RenderDevice supports "X_CURRENT" as an option for most settings.

  <P>
 <B>Stereo Rendering</B>
 <P> For stereo rendering, set <CODE>OSWindow::Settings::stereo = true</CODE>
     and use RenderDevice::setDrawBuffer to switch which eye is being rendered.  Only
     use RenderDevice::beginFrame / RenderDevice::endFrame once per frame,
     but do clear both buffers separately.

  You can render in stereo (on a stereo capable card) by rendering twice,
  once for each eye's buffer:

  <pre>
    void onGraphics(...) {
        rd->setDrawBuffer(RenderDevice::DRAW_LEFT); 
        for (int count = 0; count < 2; ++count) {
           ... (put your normal rendering code here)
           rd->setDrawBuffer(RenderDevice::DRAW_RIGHT);
        }
    }
  </pre>
  
  <B>Multiple displays</B>
  If you are using multiple synchronized displays (e.g. the CAVE),
  see:
  http://www.nvidia.com/object/IO_10753.html
  and
  http://www.cs.unc.edu/Research/stc/FAQs/nVidia/FrameLock-V1.0C.pdf

  GLCaps loads the relevant extensions for you, but you must make
  the synchronizing calls yourself (typically, immediately before
  you call swap buffers).

  <P>
  <B>Raw OpenGL Calls</B>
  RenderDevice allows you to mix your own OpenGL calls with RenderDevice calls.
  It assumes that you restored the OpenGL state after your OpenGL calls, however.
  It is not safe to mix arbitrary OpenGL calls with Shaders, however.  The 
  G3D::Shader API supports more features than OpenGL shaders and does not work
  well with low-level OpenGL.  You may find that the "wrong" shader is bound
  when you execute OpenGL calls.
 */
class RenderDevice {
public:

    /** \sa RenderDevice::setRenderMode */
    enum RenderMode {
        RENDER_SOLID, 
        RENDER_WIREFRAME, 
        RENDER_POINTS, 
        /** preserve whatever the render mode is currently set to.  */
        RENDER_CURRENT
    };
    
    /** Maximum fixed-function lights supported. \deprecated Use a Shader */
    enum {MAX_LIGHTS = 2};

    /** Maximum number of fixed-function texture units RenderDevice can use or
        track with pushed/popped render states.  This affects texture combine,
        matrix and LOD bias as well.

        The maximum number of tracked units may be lower than hardware limits
        due to the cost of tracking and restoring the state. Most users will not
        use more, but increasing this value is the only necessary change.

        \deprecated
     */
    enum {MAX_TRACKED_TEXTURE_UNITS = 2};

    /** Maximum number of programmable pipeline texture image units RenderDevice
        can use or track with pushed/popped states.  These are typically more than
        the fixed-function units.

        The maximum number of tracked units may be lower than hardware limits
        due to the cost of tracking and restoring the state. Most users will not
        use more, but increasing this value is the only necessary change.
     */
    enum {MAX_TRACKED_TEXTURE_IMAGE_UNITS = 2};

private:

    friend class AttributeArray;
    friend class VertexBuffer;
    friend class Milestone;
    friend class UserInput;

    OSWindow*                    m_window;

    /** Should the destructor delete m_window?*/
    bool                         m_deleteWindow;

    void setVideoMode();

    /**
     For counting the number of beginFrame/endFrames.
     */
    int                         m_beginEndFrame;

    bool                        m_swapBuffersAutomatically;

    /** True after endFrame until swapGLBuffers is invoked.
        Used to correctly manage value changes of m_swapBuffersAutomatically
        so that a frame not intended to be seen is never rendered.*/
    bool                        m_swapGLBuffersPending;

    /** Sets the texture matrix without checking to see if it needs to
        be changed.*/
    void forceSetTextureMatrix(int unit, const float* m);
    void forceSetTextureMatrix(int unit, const double* m);

    /** Change the projection matrix without testing to see if it needs to be changed.  
        Allows setFramebuffer to change the invertY matrix. */
    void forceSetProjectionMatrix(const Matrix4& P);
    
    void forceSetViewport(const Rect2D& v);
   
    /** The area used inside of an indexedPrimitives call. */
    shared_ptr<VertexBuffer>    m_currentVertexBuffer;

    /** Attribute arrays for shader calls on Rect2Ds */
    struct Rect2DAttributeArrays {
        AttributeArray  vertexArray;
        AttributeArray  texCoordArray;
    } m_rect2DAttributeArrays;

    /** 
        The args the last time a shader was called on a Rect2D
        These are checked every Rect2D shader call to avoid writing
        to the attribute array unneccessarily
    */
    struct RectShaderArgs {
        float zCoord;
        Rect2D vertices;
        Rect2D texCoord;
        // If true, used one giant triangle instead of two small ones, and relied on the scissor region to cut out the rectangle
        bool giantTriangle;
        RectShaderArgs() : zCoord(fnan()), vertices(), texCoord() {}
        RectShaderArgs(float z, const Rect2D& rect, const Rect2D& tCoord, bool gTriangle) : zCoord(z), vertices(rect), texCoord(tCoord), giantTriangle(gTriangle) {}

        bool operator==(const RectShaderArgs& other) const {
            bool zEqual = (zCoord == other.zCoord); // VS2012 says: QNAN == 0 is true...
            bool vEqual = (vertices == other.vertices);
            bool tEqual = (texCoord == other.texCoord);
            bool gEqual = (giantTriangle == other.giantTriangle);
            return !isNaN(zCoord) && !isNaN(other.zCoord) && zEqual && vEqual && tEqual && gEqual;
        }

        bool operator!=(const RectShaderArgs& other) const {
            return !(operator==(other));
        }

    } m_previousRectShaderArgs;
    
    String                 cardDescription;

    /** Helper for setXXXArray.  Sets the m_currentVARArea and
        makes some consistency checks.*/
    void setVARAreaFromVAR(const class AttributeArray& v);

    /** Updates the triangle count based on the primitive.
        
        LINE and POINT primitives are given one triangle count each. */
    void countTriangles(PrimitiveType primitive, int numVertices);
    
    /** Called by sendIndices. */
    void internalSendIndices
    (PrimitiveType           primitive,
     int                     indexSize, 
     int                     numIndices, 
     const void*             index,
     int                     numInstances,
     bool                    useInstances);    

    static String dummyString;

    /** Returns true if the frame buffer is complete, or a string in 
        whyIncomplete explaining the problem.
        Potentially slow. */
    bool checkDrawFramebuffer(String& whyIncomplete = dummyString) const;
    bool checkReadFramebuffer(String& whyIncomplete = dummyString) const;

    /** Sets the glDrawBuffersARB to match the currently bound
        drawFramebuffer's capabilities.  Called from clear() and
        beforePrimitive(). */
    void syncDrawBuffer(bool alreadyBound);
    void syncReadBuffer(bool alreadyBound);

public:
    
    /** \brief Reports measured GPU performance and throughput.
        
        "OpenGL state changes" are those that forced underlying OpenGL
        state changes; RenderDevice optimizes away redundant state
        changes so many changes will not affect OpenGL. 
        
        Many programs are limited by the number of GL state
        changes; sorting objects in the render pipeline to minimize this 
        can dramatically increase performance.

        Major state changes are texture, shader, camera, and projection changes.
        These are particularly expensive because they can cause cache misses and
        pipeline stalls.  Graphics cards can handle about
        100 of these in real-time. Minor state changes are color, vertex,
        and test changes.  About 2000 of these can be made per frame in real-time.

        One way to locate the code that causes state changes is to put breakpoints
        in RenderDevice::minGLStateChange and RenderDevice::majGLStateChange
        and look at the call stack.

        Zeroed by beginFrame.*/
    class Stats {
    public:
        uint32            minorStateChanges;
        uint32            minorOpenGLStateChanges;
        
        uint32            majorStateChanges;
        uint32            majorOpenGLStateChanges;
        
        uint32            pushStates;
        
        /** Number of individual primitives (e.g., number of triangles) */
        uint32            primitives;
        
        /** Number of triangles since last beginFrame() */
        uint32            triangles;
        
        /** Exponentially weighted moving average of Stats::triangles.*/
        double           smoothTriangles;
        
        /** Amount of time spent in swapbuffers (when large, indicates 
            that the GPU is blocking the CPU). */
        RealTime        swapbuffersTime;
        
        /** Inverse of beginframe->endframe time.  Accounts for the frame
            timing of the system as a whole, not just graphics.*/
        float            frameRate;
        
        /** Exponentially weighted moving average of frame rate */
        float           smoothFrameRate;
        
        double            triangleRate;
        
        /** Exponentially weighted moving average of triangleRate */
        double          smoothTriangleRate;
        
        Stats();

        void reset();
    };

protected:

    /** Time at which the previous endFrame() was called */
    RealTime                    m_lastTime;

    Stats                       m_stats;

    /** Storage for setVARs.  Cleared by endIndexedPrimitives. */
    Array<AttributeArray>          m_tempVAR;

    class VARState {
    public:
        int                     highestEnabledTexCoord;

        VARState() : highestEnabledTexCoord(-1) {}
    };
    
    /** Note: note backed up by push/pop, since push/pop can't be
        called inside indexed primitives */
    VARState                    m_varState;

public:

    inline const Stats& stats() {
        return m_stats;
    }

    // These are abstracted to make it easy to put breakpoints in them
    /**
       State change to RenderDevice.
       Use to update the state change statistics when raw OpenGL calls are made. */
    inline void majStateChange(int inc = 1) {
       m_stats.majorStateChanges += inc;
    }

    /** 
      State change to RenderDevice.
     Use to update the state change statistics when raw OpenGL calls are made. */
    inline void minStateChange(int inc = 1) {
        m_stats.minorStateChanges += inc;
    }

    /** 
     State change to OpenGL (possibly because of a state change to RenderDevice).
      Use to update the state change statistics when raw OpenGL calls are made. */
    inline void majGLStateChange(int inc = 1) {
        m_stats.majorOpenGLStateChanges += inc;
    }

    /** 
      State change to OpenGL (possibly because of a state change to RenderDevice).
      Use to update the state change statistics when raw OpenGL calls are made. */
    inline void minGLStateChange(int inc = 1) {
        m_stats.minorOpenGLStateChanges += inc;
    }

    /** RenderDevice active on this thread, NULL if there is not one.  By default, G3D creates a single RenderDevice on the main thread.
        If you create a second OpenGL context you should also make a RenderDevice for it .*/
    static __thread RenderDevice*   current;

protected:

    /** Times swapBuffers */
    Stopwatch                       m_swapTimer;

public:

    RenderDevice();

    ~RenderDevice();

    /**
     Prints a human-readable description of this machine
     to the text output stream.  Either argument may be NULL.
     */
    void describeSystem(TextOutput& t);

    void describeSystem(String& s);

    /**
     \brief Checkmarks all RenderDevice state (anything that can be
     set using RenderDevice methods) except for the currently bound
     vertex arrays.

     If you are using some other OpenGL state that is not covered by
     any of the above (e.g., the glReadBuffer and other buffer
     options), you can call glPushAttrib(GL_ALL_ATTRIB_BITS)
     immediately before pushState to ensure that it is pushed as well.
     */
    void pushState();

    /** \brief Pushes the current state, then set the specified frame
        buffer and matches the viewport to it.*/
    void pushState(const shared_ptr<Framebuffer>& fb);

    /**
     Sets all state to a clean rendering environment.
     */
    void resetState();

    /**
     Restores all state to whatever was pushed previously.  Push and 
     pop must be used in matching pairs.
     */
    void popState();

    /** To clear the alpha portion of the color buffer, remember to
        enable alpha write */
    void clear(bool clearColor, bool clearDepth, bool clearStencil);

    /** Clears the drawbuffer to the specified clear values set in each attachment.*/
    void clearFramebuffer(bool clearColor = true, bool clearDepthAndStencil = true);

    /** Clears color, depth, and stencil. */
    inline void clear() {
        clear(true, true, true);
    }

    /** \sa drawBuffer() */
    enum DrawBuffer {
        DRAW_NONE = GL_NONE, 
        DRAW_FRONT_LEFT = GL_FRONT_LEFT, 
        DRAW_FRONT_RIGHT = GL_FRONT_RIGHT, 
        DRAW_BACK_LEFT = GL_BACK_LEFT, 
        DRAW_BACK_RIGHT = GL_BACK_RIGHT, 
        DRAW_FRONT = GL_FRONT, 
        DRAW_BACK = GL_BACK, 
        DRAW_LEFT = GL_LEFT, 
        DRAW_RIGHT = GL_RIGHT, 
        DRAW_FRONT_AND_BACK = GL_FRONT_AND_BACK, 
        DRAW_AUX0 = GL_AUX0, 
        DRAW_AUX1 = GL_AUX1, 
        DRAW_AUX2 = GL_AUX2, 
        DRAW_AUX3 = GL_AUX3,

        DRAW_COLOR0 = GL_COLOR_ATTACHMENT0, 
        DRAW_COLOR1 = GL_COLOR_ATTACHMENT1, 
        DRAW_COLOR2 = GL_COLOR_ATTACHMENT2, 
        DRAW_COLOR3 = GL_COLOR_ATTACHMENT3, 
        DRAW_COLOR4 = GL_COLOR_ATTACHMENT4, 
        DRAW_COLOR5 = GL_COLOR_ATTACHMENT5, 
        DRAW_COLOR6 = GL_COLOR_ATTACHMENT6, 
        DRAW_COLOR7 = GL_COLOR_ATTACHMENT7, 
        DRAW_COLOR8 = GL_COLOR_ATTACHMENT8, 
        DRAW_COLOR9 = GL_COLOR_ATTACHMENT9, 
        DRAW_COLOR10 = GL_COLOR_ATTACHMENT10, 
        DRAW_COLOR11 = GL_COLOR_ATTACHMENT11, 
        DRAW_COLOR12 = GL_COLOR_ATTACHMENT12, 
        DRAW_COLOR13 = GL_COLOR_ATTACHMENT13, 
        DRAW_COLOR14 = GL_COLOR_ATTACHMENT14, 
        DRAW_COLOR15 = GL_COLOR_ATTACHMENT15,

        DRAW_CURRENT
    };

    /** The constants that correspond to DrawBuffer have the same
        value, so that you can safely cast between them. All have
        the corresponding OpenGL constant.
      
        \sa readBuffer()
    */
    enum ReadBuffer {
        READ_FRONT_LEFT = GL_FRONT_LEFT, 
        READ_FRONT_RIGHT = GL_FRONT_RIGHT, 
        READ_BACK_LEFT = GL_BACK_LEFT,
        READ_BACK_RIGHT = GL_BACK_RIGHT,
        READ_FRONT = GL_FRONT, 
        READ_BACK = GL_BACK, 
        READ_LEFT = GL_LEFT,
        READ_RIGHT = GL_RIGHT,

        READ_COLOR0 = GL_COLOR_ATTACHMENT0,
        READ_COLOR1 = GL_COLOR_ATTACHMENT1,
        READ_COLOR2 = GL_COLOR_ATTACHMENT2,
        READ_COLOR3 = GL_COLOR_ATTACHMENT3,
        READ_COLOR4 = GL_COLOR_ATTACHMENT4,
        READ_COLOR5 = GL_COLOR_ATTACHMENT5,
        READ_COLOR6 = GL_COLOR_ATTACHMENT6,
        READ_COLOR7 = GL_COLOR_ATTACHMENT7,
        READ_COLOR8 = GL_COLOR_ATTACHMENT8,
        READ_COLOR9 = GL_COLOR_ATTACHMENT9,
        READ_COLOR10 = GL_COLOR_ATTACHMENT10,
        READ_COLOR11 = GL_COLOR_ATTACHMENT11,
        READ_COLOR12 = GL_COLOR_ATTACHMENT12,
        READ_COLOR13 = GL_COLOR_ATTACHMENT13,
        READ_COLOR14 = GL_COLOR_ATTACHMENT14,
        READ_COLOR15 = GL_COLOR_ATTACHMENT15,

        READ_DEPTH = GL_DEPTH_ATTACHMENT,
        READ_STENCIL = GL_STENCIL_ATTACHMENT,

        READ_CURRENT
    };
    
    enum DepthTest {
        DEPTH_GREATER     = GL_GREATER,
        DEPTH_LESS        = GL_LESS,
        DEPTH_GEQUAL      = GL_GEQUAL,  
        DEPTH_LEQUAL      = GL_LEQUAL, 
        DEPTH_NOTEQUAL    = GL_NOTEQUAL, 
        DEPTH_EQUAL       = GL_EQUAL,   
        DEPTH_ALWAYS_PASS = GL_ALWAYS,
        DEPTH_NEVER_PASS  = GL_NEVER, 
        DEPTH_CURRENT
    };

    /** This is provided for backwards compatibility. 
        \sa OSWindow::Settings::enableAlphaTesting
      */
    enum AlphaTest {
        ALPHA_GREATER       = GL_GREATER,
        ALPHA_LESS          = GL_LESS,
        ALPHA_GEQUAL        = GL_GEQUAL,  
        ALPHA_LEQUAL        = GL_LEQUAL,
        ALPHA_NOTEQUAL      = GL_NOTEQUAL,
        ALPHA_EQUAL         = GL_EQUAL,  
        ALPHA_ALWAYS_PASS   = GL_ALWAYS, 
        ALPHA_NEVER_PASS    = GL_NEVER,
        ALPHA_CURRENT
    };

    enum StencilTest {
        STENCIL_GREATER     = GL_GREATER,  
        STENCIL_LESS        = GL_LESS, 
        STENCIL_GEQUAL      = GL_GEQUAL,
        STENCIL_LEQUAL      = GL_LEQUAL, 
        STENCIL_NOTEQUAL    = GL_NOTEQUAL,
        STENCIL_EQUAL       = GL_EQUAL, 
        STENCIL_ALWAYS_PASS = GL_ALWAYS, 
        STENCIL_NEVER_PASS  = GL_NEVER,
        STENCIL_CURRENT
    };

    enum BlendFunc {
        BLEND_SRC_ALPHA = GL_SRC_ALPHA,
        BLEND_ONE_MINUS_SRC_ALPHA = GL_ONE_MINUS_SRC_ALPHA, 
        BLEND_DST_ALPHA = GL_DST_ALPHA,
        BLEND_ONE_MINUS_DST_ALPHA = GL_ONE_MINUS_DST_ALPHA,
        BLEND_ONE = GL_ONE,
        BLEND_ZERO = GL_ZERO,
        BLEND_SRC_COLOR = GL_SRC_COLOR, 
        BLEND_DST_COLOR = GL_DST_COLOR,  
        BLEND_ONE_MINUS_SRC_COLOR = GL_ONE_MINUS_SRC_COLOR, 
        BLEND_ONE_MINUS_DST_COLOR = GL_ONE_MINUS_DST_COLOR,
        BLEND_CONSTANT_COLOR = GL_CONSTANT_COLOR, 
        BLEND_ONE_MINUS_CONSTANT_COLOR = GL_ONE_MINUS_CONSTANT_COLOR,
        BLEND_CONSTANT_ALPHA = GL_CONSTANT_ALPHA, 
        BLEND_ONE_MINUS_CONSTANT_ALPHA = GL_ONE_MINUS_CONSTANT_ALPHA,
        /** Only legal for use in setBlendFunc */
        BLEND_CURRENT,
        BLEND_SAME_AS_RGB
    };
    
    enum BlendEq {
        BLENDEQ_MIN = GL_MIN, 
        BLENDEQ_MAX = GL_MAX,
        BLENDEQ_ADD = GL_FUNC_ADD,
        BLENDEQ_SUBTRACT = GL_FUNC_SUBTRACT,
        BLENDEQ_REVERSE_SUBTRACT = GL_FUNC_REVERSE_SUBTRACT,
        /** Only legal for use in setBlendFunc */
        BLENDEQ_CURRENT,
        BLENDEQ_SAME_AS_RGB
    };

    enum StencilOp {
        STENCIL_INCR_WRAP = GL_INCR_WRAP, 
        STENCIL_DECR_WRAP = GL_DECR_WRAP,
        STENCIL_KEEP = GL_KEEP, 
        STENCIL_INCR = GL_INCR,
        STENCIL_DECR = GL_DECR,
        STENCIL_REPLACE = GL_REPLACE,
        STENCIL_ZERO = GL_ZERO,  
        STENCIL_INVERT = GL_INVERT, 
        STENCILOP_CURRENT
    };

    enum LogicOp {
        LOGIC_CLEAR = GL_CLEAR,
        LOGIC_AND = GL_AND,
        LOGIC_AND_REVERSE = GL_AND_REVERSE,
        LOGIC_COPY = GL_COPY,
        LOGIC_AND_INVERTED = GL_AND_INVERTED,
        LOGIC_NOOP = GL_NOOP,
        LOGIC_XOR = GL_XOR,
        LOGIC_OR = GL_OR,
        LOGIC_NOR = GL_NOR,
        LOGIC_EQUIV = GL_EQUIV,
        LOGIC_INVERT = GL_INVERT,
        LOGIC_OR_REVERSE = GL_OR_REVERSE,
        LOGIC_COPY_INVERTED = GL_COPY_INVERTED,
        LOGIC_OR_INVERTED = GL_OR_INVERTED,
        LOGIC_NAND = GL_NAND,
        LOGIC_SET = GL_SET,
        LOGICOP_CURRENT
    };
    
    enum ShadeMode {
        SHADE_FLAT = GL_FLAT,  
        SHADE_SMOOTH = GL_SMOOTH,
        SHADE_CURRENT
    };

    /**
     Call to begin the rendering frame.
     */
    void beginFrame();

    /**
     Call to end the current frame and schedules a OSWindow::swapGLBuffers call
     to occur some time before beginFrame.  Because that swapGLBuffers might not
     actually occur until the next beginFrame, there is up to one frame of latency
     on the image displayed.  This allows the CPU to execute while 
     the GPU is still rendering, providing net higher performance.
     */
    void endFrame();

    inline bool swapBuffersAutomatically() const {
        return m_swapBuffersAutomatically;
    }

    /** Manually swap the front and back buffers.  Using swapBuffersAutomatically() is recommended
        instead of manually swapping because it has higher performance.*/
    void swapBuffers();

    /** 
        By default, OSWindow::swapGLBuffers is invoked automatically
        between RenderDevice::endFrame and the following RenderDevice::beginFrame
        to update the front buffer (what the user sees) from the back buffer (where
        rendering commands occur).  You may want to suppress this behavior, for example,
        in order
        to render to the back buffer and capture the result in a texture.
    
        The state of swapBuffersAutomatically is <B>not</B> stored by
        RenderDevice::pushState because it is usually invoked outside
        of RenderDevice::beginFrame / endFrame. 
    */
    void setSwapBuffersAutomatically(bool b);

    /** Measures the amount of time spent in swapBuffers.  If high, indicates that
        the CPU and GPU are not working in parallel*/
    const Stopwatch& swapBufferTimer() const {
        return m_swapTimer;
    }

    /** \beta Set OSWindow::Settings::debugContext = true and then set
        this to true to enable OpenGL debugging output. */
    void setDebugOutput(bool b);

    /**
     Use ALWAYS_PASS to shut off testing.
     */
    void setDepthTest(DepthTest test);
    void setStencilTest(StencilTest test);
    StencilTest stencilTest() const;

    /** If the alpha test is ALPHA_CURRENT, the reference is ignored.
        Illegal unless OSWindow::Settings::allowAlphaTest is true. */
    void setAlphaTest(AlphaTest test, float reference);

    AlphaTest alphaTest() const;

    float alphaTestReference() const;

    /** \param format If NULL, defaults to texture->format() */
    void copyTextureFromScreen(const shared_ptr<Texture>& texture, const Rect2D& rect, const ImageFormat* format = NULL, int mipLevel = 0, CubeFace face = CubeFace::POS_X);

    void setRenderMode(RenderMode mode);
    RenderMode renderMode() const;

    /**
     Sets the constant used in the stencil test and operation (if op == STENCIL_REPLACE)
     */
    void setStencilConstant(int reference);
	
    /**
     Sets the frame buffer that is written to.  Used to intentionally
     draw to the front buffer and for stereo rendering.  Its operation
     is sensitive to the current framebuffer being written to. If the
     framebuffer is the primary display then only visible buffers may
     be specified.  If framebuffer() is non-NULL then the drawBuffer
     is stored for later use but the actual draw buffers remain the
     ones specified by the framebuffer.

     When using a G3D::Framebuffer it is not necessary to explicitly set the
     draw buffer; that is handled automatically by beforePrimitive().
     */
    void setDrawBuffer(DrawBuffer drawBuffer);

    inline DrawBuffer drawBuffer() const {
        return m_state.drawBuffer;
    }

    void setReadBuffer(ReadBuffer readBuffer);

    inline ReadBuffer readBuffer() const {
        return m_state.readBuffer;
    }

    inline void setDepthRange(float low, float high);

    /** Color writing is on by default.  Disabling color write allows a program to 
        render to the depth and stencil buffers without creating a visible image in the frame buffer.  
        This is useful for occlusion culling, shadow rendering, and some computational solid geometry algorithms.  
        Rendering may be significantly accelerated when color write is disabled. */
    inline void setColorWrite(bool b);

    /** Returns true if colorWrite is enabled */
    bool colorWrite() const;

    /** The frame buffer may optionally have an alpha channel for each pixel, depending on how
        the G3D::OSWindow was initialized (see G3D::RenderDevice::init, and G3D::OSWindow::Settings).
        When the alpha channel is present, rendering to the screen also renders to the alpha 
        channel by default.  Alpha writing is used for render-to-texture and deferred lighting effects.
    
        Rendering to the alpha channel does not produce transparency effects--this is an alpha
        output, not an alpha input.  See RenderDevice::setBlendFunc for a discussion of blending.*/
    inline void setAlphaWrite(bool b);

    /** Defaults to true. */
    inline void setDepthWrite(bool b);

    /** Returns true if depthWrite is enabled */
    bool depthWrite() const;

    /** Returns true if alphaWrite is enabled */
    bool alphaWrite() const;

    /**
     If wrapping is not supported on the device, the nearest mode is
     selected.  Unlike OpenGL, stencil writing and testing are
     independent. You do not need to enable the stencil test to use
     the stencil op.

     Use KEEP, KEEP, KEEP to disable stencil writing.  Equivalent to a
     combination of glStencilTest, glStencilFunc, and glStencilOp.


     If there is no depth buffer, the depth test always passes.  If there
     is no stencil buffer, the stencil test always passes.
     */
    void setStencilOp(
        StencilOp                       fail,
        StencilOp                       zfail,
        StencilOp                       zpass);

    /**
     When GLCaps::GL_ARB_stencil_two_side is true, separate
     stencil operations can be used for front and back faces.  This
     is useful for rendering shadow volumes.
     */
    void setStencilOp(
        StencilOp                       frontStencilFail,
        StencilOp                       frontZFail,
        StencilOp                       frontZPass,
        StencilOp                       backStencilFail,
        StencilOp                       backZFail,
        StencilOp                       backZPass);

    /** 
        Equivalent to glLogicOp call. 
     */
    void setLogicOp(const LogicOp op);

    /**
     Equivalent to <A HREF="http://developer.3dlabs.com/GLmanpages/glblendfunc.htm">glBlendFunc</A>
     and <A HREF="http://developer.apple.com/documentation/Darwin/Reference/ManPages/man3/glBlendEquation.3.html">glBlendEquation</A>.

     Use 
       <CODE>setBlendFunc(Framebuffer::COLOR0, RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO, RenderDevice::BLENDEQ_ADD)</CODE>
     to shut off blending.
     
     Use 
       <CODE>setBlendFunc(Framebuffer::COLOR0, RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA, RenderDevice::BLENDEQ_ADD)</CODE>
     for unmultiplied alpha blending and
       <CODE>setBlendFunc(Framebuffer::COLOR0, RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA, RenderDevice::BLENDEQ_ADD)</CODE>
     for premultiplied alpha.

     %Draw your objects from back to front, objects with alpha last. Objects with alpha only get drawn properly if the things
     they're occluding have been drawn before the alpha'd objects. 

     Generally, turn alpha on, draw your alpha-blended things, then turn alpha off.
     Separate functions can be passed for RGB and Alpha channels: for example, by default, eqA is
     BLENDEQ_SAME_AS_RGB: this will force eqA to be the same function as eqRGB during this call.
     This interacts with BLEND_EQ current in the following way:
     If eqRGB = BLENDEQ_CURRENT and eqA = BLENDEQ_SAME_AS_RGB, then eqA will be 
     equal to the current m_state.blendEqRGB,NOT the current m_state.blendEqA
   
     */
    void setBlendFunc(
        Framebuffer::AttachmentPoint    buf,
        BlendFunc                       srcRGB,
        BlendFunc                       dstRGB,
        BlendEq                         eqRGB = BLENDEQ_ADD,
        BlendFunc                       srcA  = BLEND_SAME_AS_RGB,
        BlendFunc                       dstA  = BLEND_SAME_AS_RGB,
        BlendEq                         eqA   = BLENDEQ_SAME_AS_RGB);

    /**
        @deprecated overload for backwards compatability, use version above instead
    */
    void setBlendFunc(
        BlendFunc                       src,
        BlendFunc                       dst,
        BlendEq                         eqRGB = BLENDEQ_ADD,
        BlendEq                         eqA   = BLENDEQ_SAME_AS_RGB,
        Framebuffer::AttachmentPoint    buf   = Framebuffer::COLOR0);

    void getBlendFunc(
        Framebuffer::AttachmentPoint    buf,
        BlendFunc&                      srcRGB,
        BlendFunc&                      dstRGB,
        BlendEq&                        eqRGB,        
        BlendFunc&                      srcA,
        BlendFunc&                      dstA,
        BlendEq&                        eqA) {
            int i = buf - Framebuffer::COLOR0;
            srcRGB = m_state.srcBlendFuncRGB[i];
            dstRGB = m_state.dstBlendFuncRGB[i];
            eqRGB  = m_state.blendEqRGB[i];
            srcA = m_state.srcBlendFuncA[i];
            dstA = m_state.dstBlendFuncA[i];
            eqA    = m_state.blendEqA[i];
    }

    /** 
      Sets a 2D clipping region (OpenGL scissor region) relative to the current window
      dimensions (not the viewport). Prevents rendering outside the clip region.

      Pushing a new Framebuffer (but not setting one explicitly) resets the clip2D state
      to the ful screen.

      Set to Rect2D::inf() to disable. Default is disabled.

      Note that the clip uses G3D 2D coordinates, where the upper-left of the window
      is (0, 0).

      \sa setGuardBandClip2D, intersectClip2D, clip2D
      */
    void setClip2D(const Rect2D& clip);

    /**
      Intersects the current clipping (scissor) region with this one, which is specified in window
      coordinates.
      \sa setGuardBandClip2D, setClip2D, clip2D
     */
    void intersectClip2D(const Rect2D& clip) {
        setClip2D(clip.intersect(clip2D()));
    }

    /** Sets a clip2D region that is inset from the current framebuffer's boundaries by \a thickness */
    void setGuardBandClip2D(const Vector2int16 thickness) {
        setClip2D(Rect2D::xyxy(Vector2(thickness), Vector2(width() - float(thickness.x), height() - float(thickness.y))));
    }

    /** If enabled, returns the current clip region, othrwise the viewport. */
    Rect2D clip2D() const;

    /**
     Equivalent to glPointSize.
     */
    void setPointSize(
        float                          diameter);

    /**
     This is not the OpenGL MODELVIEW matrix: it is a matrix that maps
     object space to world space.  The actual MODELVIEW matrix
     is cameraToWorld.inverse() * objectToWorld.  You can retrieve it
     with getModelViewMatrix.
     */
    void setObjectToWorldMatrix(
        const CoordinateFrame&          cFrame);

    const CoordinateFrame& objectToWorldMatrix() const;

    /**
     See RenderDevice::setObjectToWorldMatrix.
     */
    void setCameraToWorldMatrix(
        const CoordinateFrame&          cFrame);

    const CoordinateFrame& cameraToWorldMatrix() const;

    const CoordinateFrame& worldToCameraMatrix() const;

    /** \brief True if the Y-axis has been flipped from the G3D convention, which occurs when the framebuffer is NULL.
    
        By default, RenderDevice conventions assume that you are rendering to a 
        G3D::Texture in a G3D::Framebuffer, so it configures the projection matrix
        and polygon winding direction so that the upper-left corner of the framebuffer
        is texel (0, 0).  

        When rendering directly to the screen, the opposite convention is needed.  In this
        case, G3D applies an extra invertYMatrix() that flips the Y axis and internally
        inverts the winding conventions.*/
    bool invertY() const;

    /** If G3D::RenderDevice::invertY() is true, this is the matrix is applied after the
        projection matrix to flip the y-axis.  Otherwise it is the identity matrix.*/
    const Matrix4& invertYMatrix() const;

    /** The G3D projection matrix.  Does not include the invertYMatrix().
        Note that this is not equal to the GLSL gl_ProjectionMatrix.
        
        \sa G3D::Projection
     */
    Matrix4 projectionMatrix() const;
    
    /**
     cameraToWorldMatrix().inverse() * objectToWorldMatrix()
     */
    CoordinateFrame modelViewMatrix() const;

    /**
     projectionMatrix() * cameraToWorldMatrix().inverse() * objectToWorldMatrix() * invertYMatrix().
     */
    Matrix4 modelViewProjectionMatrix() const;

    /**
     invertYMatrix() * projectionMatrix() * objectToWorldMatrix()
     */
    Matrix4 objectToScreenMatrix() const;
    /**
       To set a typical 3D perspective matrix, use either

       <code> renderDevice->setProjectionMatrix(Matrix4::perspectiveProjection(...)) </code>

       or call setProjectionAndCameraMatrix.
     */
    void setProjectionMatrix(const Matrix4& P);
    void setProjectionMatrix(const class Projection& P);
	
    /**
     Equivalent to glPolygonOffset
     */
    void setPolygonOffset(
        float                     offset);

    /**
     Equivalent to glCullFace.

     RenderDevice will internally swap the OpenGL cull direction when rendering to a Framebuffer in order
     to be consistent with the inverted Y axis.
     */
    void setCullFace(CullFace f);

    inline CullFace cullFace() const {
        return m_state.cullFace;
    }

    /** By default, opengl does not covert writes to an sRGB texture into sRGB color space. 
        Set this to true to force such a conversion. */
    void setSRGBConversion(bool b);

    inline bool sRGBConversion() const {
        return m_state.sRGBConversion;
    }

    /**
     (0, 0) is the <B>upper</B>-left corner of the screen.
     */
    void setViewport(const Rect2D& v);
    const Rect2D& viewport() const;

    /** Setting both simultaneously minimizes OpenGL state changes*/
    void setProjectionAndCameraMatrix(const Projection& p, const CFrame& c);


    void beginIndexedPrimitives();
    void endIndexedPrimitives();


    /** Returns the OSWindow used by this RenderDevice */
    OSWindow* window() const;

    /** Sets the OSWindow used by this RenderDevice. Useful if using a single RenderDevice for multiple windows */
    void setWindow(OSWindow* window);

    /**
     Vertex attributes are a generalization of the various per-vertex
     attributes that relaxes the format restrictions.  There are at least
     16 attributes on any card (some allow more).  During the days of fixed-function,
     these attributes had special meaning, but G3D no longer supports the fixed function pipeline.
    */
    void setVertexAttribArray(unsigned int attribNum, const class AttributeArray& v);

    /**
        Counterpart to setVertexAttribArray.
    */
    void unsetVertexAttribArray(unsigned int attribNum);

    /**
     Draws the specified kind of primitive from the current vertex array.

     \deprecated Use sendIndices(PrimitiveType, const AttributeArray&)
     */
    template<class T>
    void sendIndices
    (PrimitiveType primitive, int numIndices, const T* index) {
        debugAssertM(currentDrawFramebufferComplete(), "Incomplete Framebuffer");
        internalSendIndices(primitive, sizeof(T), numIndices, index, 1, false);
                
        countTriangles(primitive, numIndices);
    }

    /** \beta Draws the specified kind of primitive using each one of the indexStreams */
    void sendMultidrawIndices(PrimitiveType primitive, const Array<IndexStream>& indexStreams, int numInstances, bool useInstances);

    /** \beta Draws the specified kind of primitive using sequential indices */
    void sendMultidrawSequentialIndices(PrimitiveType primitive, const Array<int>& indexCounts, const Array<int>& indexOffsets);

    /** Send indices from an index buffer stored inside a vertex
     buffer. This is faster than sending from main
     memory on most GPUs.
    */
    void sendIndices
    (PrimitiveType primitive, const IndexStream& indexStream);

    /** Send indices from an index buffer stored inside a vertex
     buffer. This is faster than sending from main
     memory on most GPUs.

     Inside the vertex shader, gl_InstanceID is
     bound to the number of the instance.  The first instance is 0,
     the last is \a numInstances - 1.

     (Equivalent to glDrawElementsInstanced)

     \param numInstances number of instances of these indices to send
    */
    void sendIndicesInstanced
    (PrimitiveType          primitive, 
     const IndexStream&     indexStream,
     int                    numInstances);

private:

    /** Called by both sendIndices and sendIndicesInstanced */
    void sendIndices
    (PrimitiveType primitive, 
     const IndexStream&      indexStream,
     int                     numInstances, 
     bool                    drawInstanced);

public:

    /**
     Renders sequential vertices from the current vertex array.
     (Equivalent to glDrawArrays)
     */
    void sendSequentialIndices
    (PrimitiveType primitive, 
     int                     numVertices,
     int                     startVertex = 0);

    /**
     Renders sequential vertices from the current vertex array for
     multiple instances. 

     Inside the vertex shader, gl_InstanceID is
     bound to the number of the instance.  The first instance is 0,
     the last is \a numInstances - 1.

     (Equivalent to glDrawArraysInstanced)

     \param numInstances number of instances of these indices to send
     */
    void sendSequentialIndicesInstanced
    (PrimitiveType primitive, int numVertices, int numInstances);

    /**
     Draws the specified kind of primitive from the current vertex array.
     
     @deprecated Use sendIndices(PrimitiveType, const AttributeArray&)
     */
    template<class T>
    void sendIndices(PrimitiveType primitive, 
                     const Array<T>& index) {
        sendIndices(primitive, index.size(), index.getCArray());
    }
    
    void setStencilClearValue(int s);
    void setDepthClearValue(float d);
    void setColorClearValue(const Color4& c);

    void modifyArgsForRectModeApply(Args& args);

    void apply(const shared_ptr<class Shader>& s, Args& args);

    void apply(const shared_ptr<class Shader>& s) {
        Args args;
        apply(s, args);
    }

    /**
      Reads a depth buffer value (1 @ far plane, 0 @ near plane) from
      the given screen coordinates (x, y) where (0,0) is the top left
      corner of the width x height screen.  Result is undefined for x, y not
      on screen.

      The result is sensitive to the projection and camera to world matrices.

      If you need to read back the entire depth buffer, use OpenGL glReadPixels
      calls instead of many calls to getDepthBufferValue.
     */
    double getDepthBufferValue
       (int                 x,
        int                 y) const;


    /**
     Description of the graphics card and driver version.
     */
    const String& getCardDescription() const;

    /** Automatically called immediately before a primitive group.  User code should only
        call this if making raw OpenGL calls (i.e., "glBegin"), in which case it should be
        called immediately before the glBegin and afterPrimitve should be called immediately
        after the glEnd.
      */
    void beforePrimitive();

    /** Automatically called immediately after a primitive group. See also beforePrimitive.*/
    void afterPrimitive();

private:

    /**
     For performance, we don't actually unbind a texture when
     turning off a texture unit, we just disable it.  If it 
     is enabled with the same texture, we've saved a swap.
    */
    int currentlyBoundTextures[MAX_TRACKED_TEXTURE_IMAGE_UNITS];

    /**
     Snapshot of the state maintained by the render device.
     */
    // WARNING: if you add state, you must initialize it in
    // the constructor and RenderDevice::init and set it in
    // setState().
    class RenderState {
    public:
        friend class RenderDevice;
        
        class Stencil {
        public:
            StencilTest                 stencilTest;
            int                         stencilReference;
            int                         stencilClear;
            StencilOp                   frontStencilFail;
            StencilOp                   frontStencilZFail;
            StencilOp                   frontStencilZPass;
            StencilOp                   backStencilFail;
            StencilOp                   backStencilZFail;
            StencilOp                   backStencilZPass;
          
            bool operator==(const Stencil& other) const;

            inline bool operator!=(const Stencil& other) const {
                return !(*this == other);
            }
        };

        class Matrices {
        public:
            CoordinateFrame             objectToWorldMatrix;
            CoordinateFrame             cameraToWorldMatrix;
            CoordinateFrame             cameraToWorldMatrixInverse;
            Matrix4                     projectionMatrix;

            /** True when inverting from the G3D coordinate system to the OpenGL one.
                Set automatically in RenderDevice::setDrawFramebuffer(). */
            bool                        invertY;
            bool                        changed;

            inline Matrices() : invertY(true), changed(true) {}

            bool operator==(const Matrices& other) const;

            inline bool operator!=(const Matrices& other) const {
                return !(*this == other);
            }
        };
        
        Rect2D                      viewport;
        Rect2D                      clip2D;
        bool                        useClip2D;

        bool                        depthWrite;
        bool                        colorWrite;
        bool                        alphaWrite;

        DrawBuffer                  drawBuffer;
        ReadBuffer                  readBuffer;

        shared_ptr<Framebuffer>     drawFramebuffer;
        shared_ptr<Framebuffer>     readFramebuffer;
        
        DepthTest                   depthTest;
        AlphaTest                   alphaTest;
        float                       alphaReference;

        float                       depthClear;
        Color4                      colorClear;

        CullFace                    cullFace;

        bool                        sRGBConversion;

        Stencil                     stencil;

        LogicOp                     logicOp;
        
        SmallArray<BlendFunc, 16>   srcBlendFuncRGB;
        SmallArray<BlendFunc, 16>   srcBlendFuncA;
        SmallArray<BlendFunc, 16>   dstBlendFuncRGB;
        SmallArray<BlendFunc, 16>   dstBlendFuncA;
        SmallArray<BlendEq,   16>   blendEqRGB;
        SmallArray<BlendEq,   16>   blendEqA;
        
        ShadeMode                   shadeMode;
    
        float                       polygonOffset;

        RenderMode                  renderMode;
        float                       lowDepthRange;
        float                       highDepthRange;

        float                       pointSize;
        Matrices                    matrices;

        RenderState(int width = 1, int height = 1);

        //bool operator==(const RenderState& other) const;
    };

    /** If invertY() is false, maps GL_FRONT -> GL_BACK and GL_BACK -> GL_FRONT, 
        otherwise retains the normal ordering.
    */
    GLenum applyWinding(GLenum f) const;

    GLint toGLStencilOp(RenderDevice::StencilOp op) const;


    /** Has beginOpenGL been called? */
    bool                            m_inRawOpenGL;

    bool                            m_inIndexedPrimitive;

    int                             m_numTextureUnits;

    int                             m_numTextures;

    int                             m_numTextureCoords;
    
    /**
     Current render state.
     */
    RenderState                     m_state;

    /**
     Old render states
     */
    Array<RenderState>              m_stateStack;

    /** Only called from pushState */
    void setState(const RenderState& newState);

    bool                            m_initialized;
    bool                            m_cleanedup;

    /** Cache of values supplied to supportsImageFormat.
        Works on pointers since there is no way for users
        to construct their own ImageFormats.
     */
    Table<const ImageFormat*, bool>      _supportedImageFormat;

    void push2D(const shared_ptr<Framebuffer>& fb, const Rect2D& viewport);

    /** Sets the stencil test using the two-sided extension appropriate to this machine.*/
    void _setStencilTest(StencilTest test, int reference);

public:

    /** Wrapper for glMemoryBarrier(), type = GL_ALL_BARRIER_BITS */
    void issueMemoryBarrier(GLbitfield type);

    
    /**
     Sets the framebuffer to render to.  Use NULL to set the desired rendering 
     target to the windowing system display.

     Note that if the new framebuffer has different dimensions than
     the current one the projectionMatrix and viewport will likely be
     incorrect. Call RenderDevice::setProjectionAndCamera matrix again
     to update them.

     See RenderDevice::pushState and push2D for a way to set the frame buffer and viewport
     simultaneously.

     @param fbo Framebuffer to render to.
    */
    void setDrawFramebuffer(const shared_ptr<Framebuffer> &fbo);

    /*
     If the current readBuffer() is not legal for \a fbo, it will be changed to READ_COLOR0
     or READ_NONE if there is no color0 attachment.
    */
    void setReadFramebuffer(const shared_ptr<Framebuffer> &fbo);

    /** \brief Sets both the draw and read framebuffers */
    void setFramebuffer(const shared_ptr<Framebuffer> &fbo) {
        setDrawFramebuffer(fbo);
        setReadFramebuffer(fbo);
    }

    /** \deprecated Use drawFramebuffer() or readFramebuffer() */
    shared_ptr<Framebuffer> framebuffer() const {
        debugAssertM(m_state.drawFramebuffer == m_state.readFramebuffer, 
                      "Invoked deprecated framebuffer() method with different draw and read buffers bound.");
        return m_state.drawFramebuffer;
    }

    /** Returns the framebuffer currently bound for drawing. */
    shared_ptr<Framebuffer> drawFramebuffer() const {
        return m_state.drawFramebuffer;
    }

    /** Returns the framebuffer currently bound for reading. */
    shared_ptr<Framebuffer> readFramebuffer() const {
        return m_state.readFramebuffer;
    }

    /**
     Checks to ensure that the currently bound drawing framebuffer is complete and error free.  

     \return false On Incomplete Framebuffer Error
     \return true On Complete Framebuffer
    */
    inline bool currentDrawFramebufferComplete(String& whyIncomplete = dummyString) const {
        return isNull(m_state.drawFramebuffer) || 
            checkDrawFramebuffer(whyIncomplete);
    }

    inline bool currentReadFramebufferComplete(String& whyIncomplete = dummyString) const {
        return isNull(m_state.readFramebuffer) || 
            checkReadFramebuffer(whyIncomplete);
    }

    void push2D();

    /** 
        Pushes all state, switches to the new framebuffer, and resizes
        the viewport and projection matrix accordingly.
    */
    void push2D(const shared_ptr<Framebuffer>& drawFramebuffer);

    /**
     Set up for traditional 2D rendering (origin = upper left, y increases downwards).
     
     Note: the viewport will range up to the number of pixels (e.g., (0,0)-(640,480)), as
     recommended in http://msdn.microsoft.com/library/default.asp?url=/library/en-us/opengl/apptips_7wqb.asp .
     Push2D also translates (sets the cameraToWorldMatrix) by (0.375, 0.375, 0) as recommended
     in the OpenGL manual.  This helps avoid rasterization holes due to float-to-int roundoff.

     */
    void push2D(const Rect2D& viewport);
    void pop2D();
 
    /**
     Automatically constructs a Win32Window (on Win32), X11Window (on Linux) or
     GLFWWindow (OS X) then calls the other init
     routine (provided for backwards compatibility).  The constructed
     window is deleted on shutdown.
     */
    void init(const OSWindow::Settings& settings = OSWindow::Settings());

    /**
     The renderDevice will <B>not</B> delete the window on cleanup.
     */
    void init(OSWindow* window);

    /** Returns true after RenderDevice::init has been called. */
    bool initialized() const;
	
    inline ShadeMode shadeMode() const {
        return m_state.shadeMode;
    }

    /**
       Shuts down the rendering context.  This should be the last call you make.
    */
    void cleanup();

    /** Returns the format of the backbuffer/COLOR0 buffer (NULL if there isn't such a buffer). */
    const ImageFormat* colorFormat() const;

    /**
     Takes a JPG screenshot of the front buffer and saves it to a file.
     Returns the name of the file that was written.
     Example: renderDevice->screenshot("screens/"); 

     Pressing the "movie" icon in the GApp developer HUD or pressing F4 also
     allows direct screenshot capture.
     */
    String screenshot(const String& filepath="./") const;

    /**
       @brief Takes a screenshot.

       Reads from the current read buffer; use setReadBuffer(RenderDevice::READ_FRONT)
       to explicitly read from the front buffer, which is substantially faster than
       reading from the back buffer.

       @param getAlpha If true, the alpha channel of the frame buffer is also read back.

       @param invertY It is faster to read back images upside down
       because that is how OpenGL stores them.  Set invertY=false for
       this fast but upsidedown result.
     */
    shared_ptr<Image> screenshotPic(bool getAlpha = false, bool invertY = true) const;

    /**
     Pixel dimensions of the OpenGL window interior
     */
    int width() const;

    /**
     Pixel dimensions of the OpenGL window interior
     */
    int height() const;

    /** Begin a section of raw OpenGL calls.  All RenderDevice state is 
        synchronized with OpenGL and backed up.
        
        You can always make raw OpenGL calls with G3D, however in some cases
        RenderDevice makes lazy state changes and you'll be surprised by the outcome.
        This lets you switch to a pure OpenGL mode.  Do not make other RenderDevice
        calls while in beginOpenGL...endOpenGL.
        */
    void beginOpenGL();

    /** The state of the previous beginOpenGL is restored.*/
    void endOpenGL();

    /**
     Multiplies v by the current object to world and world to camera matrices,
     then by the projection matrix to obtain a 2D point and z-value.  
     
     The result is the 2D position to which the 3D point v corresponds.  You
     can use this to make results rendered with push2D() line up with those
     rendered with a 3D transformation.

     The value returned is relative to the current viewport.

     See G3D::glToScreen
     */
    Vector4 project(const Vector4& v) const;
    Vector4 project(const Vector3& v) const;
	
    /** 
        \brief Override the invertY() flag, which is normally set automatically by calls to setFramebuffer.
        \beta */
    void setInvertY(bool i) {
        m_state.matrices.invertY = i;
        forceSetCullFace(m_state.cullFace);
    }

#   ifdef G3D_WINDOWS
        HDC getWindowHDC() const;
#   endif

private:

    void forceSetCullFace(CullFace f);

    void forceSetStencilOp
       (StencilOp                       frontStencilFail,
        StencilOp                       frontZFail,
        StencilOp                       frontZPass,
        StencilOp                       backStencilFail,
        StencilOp                       backZFail,
        StencilOp                       backZPass);
};


inline void RenderDevice::setAlphaWrite(bool a) {
    
    minStateChange();
    if (m_state.alphaWrite != a) {
        minGLStateChange();
        GLboolean c = m_state.colorWrite ? GL_TRUE : GL_FALSE;
        m_state.alphaWrite = a;
        glColorMask(c, c, c, m_state.alphaWrite ? GL_TRUE : GL_FALSE);
    }
}


inline void RenderDevice::setColorWrite(bool a) {
    
    minStateChange();
    if (m_state.colorWrite != a) {
        minGLStateChange();
        GLboolean c = a ? GL_TRUE : GL_FALSE;
        m_state.colorWrite = a;
        glColorMask(c, c, c, m_state.alphaWrite ? GL_TRUE : GL_FALSE);
    }
}


inline void RenderDevice::setDepthWrite(bool a) {
    
    minStateChange();
    if (m_state.depthWrite != a) {
        minGLStateChange();
        glDepthMask(a);
        m_state.depthWrite = a;
        if (m_state.depthTest == DEPTH_ALWAYS_PASS) {
            setDepthTest(m_state.depthTest);
        }
    }
}


inline void RenderDevice::setDepthRange(
    float              low,
    float              high) {

    majStateChange();

    if ((m_state.lowDepthRange != low) ||
        (m_state.highDepthRange != high)) {
        glDepthRange(low, high);
        m_state.lowDepthRange = low;
        m_state.highDepthRange = high;

        minGLStateChange();
    }
}




} // namespace

#endif
