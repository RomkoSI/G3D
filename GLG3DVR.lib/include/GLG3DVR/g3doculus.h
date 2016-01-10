/**
\file g3doculus.h

  Minimal Oculus VR G3D API inclusion by Michael Mara at Stanford University
  and Morgan McGuire at Williams College. This code is in the public domain, 
  except as copyrighted by Oculus VR from their public API. 
*/
#ifndef GLG3DVR_g3doculus_h
#define GLG3DVR_g3doculus_h

#include "OVR/OVR_CAPI_GL.h"
#include "OVR/Extras/OVR_Math.h"
#include "OVRKernel/Kernel/OVR_System.h"
#include "G3D/Quat.h"
#include "G3D/Quat.h"
#include "G3D/ImageFormat.h"
#include "GLG3D/Texture.h"
#include "GLG3D/Framebuffer.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/RenderDevice.h"


inline ovrVector2f toOVR(const G3D::Vector2& vec) {
    ovrVector2f v;
    v.x = vec.x;
    v.y = vec.y;
    return v;
}

inline G3D::Vector2 toG3D(const ovrVector2f& vec) {
    return G3D::Vector2(vec.x, vec.y);
}

inline ovrVector3f toOVR(const G3D::Vector3& vec) {
    ovrVector3f v;
    v.x = vec.x;
    v.y = vec.y;
    v.z = vec.z;
    return v;
}

inline G3D::Vector3 toG3D(const ovrVector3f& vec) {
    return G3D::Vector3(vec.x, vec.y, vec.z);
}

inline ovrQuatf toOVR(const G3D::Quat& quat) {
    ovrQuatf q;
    q.x = quat.x;
    q.y = quat.y;
    q.z = quat.z;
    q.w = quat.w;
    return q;
}

inline G3D::Quat toG3D(const ovrQuatf& quat) {
    return G3D::Quat(quat.x, quat.y, quat.z, quat.w);
}

inline ovrPosef toOVR(const G3D::CFrame& pose) {
    ovrPosef p;
    p.Orientation = toOVR(G3D::Quat(pose.rotation));
    p.Position = toOVR(pose.translation);
    return p;
}

inline ovrPosef toOVR(const G3D::PFrame& pose) {
    ovrPosef p;
    p.Orientation = toOVR(pose.rotation);
    p.Position = toOVR(pose.translation);
    return p;
}

inline G3D::PFrame toG3D(const ovrPosef& pose) {
    return G3D::PFrame(toG3D(pose.Orientation), toG3D(pose.Position));
}


namespace G3D {

/**
 A circular queue of framebuffers. Oculus maintains these so that it can render to one while 
 submitting another.

 This corresponds fairly closely to the OVRFramebufferQueue class in the Oculus sample code.
*/
struct OVRFramebufferQueue {
private:

    ovrSession                      m_hmd;
    ovrSwapTextureSet*              m_colorTextureSet;
    ovrSwapTextureSet*              m_depthTextureSet;

    Array<shared_ptr<Framebuffer>>  m_framebuffer;

public:

    /** \param name For debugging. */
    OVRFramebufferQueue
       (const String&       name, 
        ovrSession          hmd, 
        ovrSizei            size, 
        const ImageFormat*  colorFormat = ImageFormat::RGBA8(), 
        const ImageFormat*  depthFormat = nullptr) : 
        m_hmd(hmd), 
        m_depthTextureSet(nullptr) {

        debugAssert(notNull(hmd));

        ovrResult result = ovr_CreateSwapTextureSetGL(m_hmd, colorFormat->openGLFormat, size.w, size.h, &m_colorTextureSet);
        if (result != ovrSuccess) {
            ovrErrorInfo errorInfo;
            ovr_GetLastErrorInfo(&errorInfo);
            alwaysAssertM(result == ovrSuccess, format("ovr_CreateSwapTextureSetGL failed with error: %s", errorInfo.ErrorString));
        }

        if (notNull(depthFormat)) {
            ovrResult result = ovr_CreateSwapTextureSetGL(m_hmd, depthFormat->openGLFormat, size.w, size.h, &m_depthTextureSet);
            debugAssert(result == ovrSuccess);  (void)result;
        }

        m_framebuffer.resize(m_colorTextureSet->TextureCount);

        for (int i = 0; i < m_colorTextureSet->TextureCount; ++i) {
            const ovrGLTexture* colorTex = (ovrGLTexture*)&m_colorTextureSet->Textures[i];
            glBindTexture(GL_TEXTURE_2D, colorTex->OGL.TexId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            ovrGLTexture* depthTex = NULL;            
            if (notNull(depthFormat)) {
                depthTex = (ovrGLTexture*)&m_depthTextureSet->Textures[i];
                glBindTexture(GL_TEXTURE_2D, depthTex->OGL.TexId);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }

            m_framebuffer[i]  = Framebuffer::create(format("%s->m_framebuffer[%d]", name.c_str(), i));
            m_framebuffer[i]->set(Framebuffer::COLOR0, Texture::fromGLTexture(format("%s->color[%d]", name.c_str(), i), colorTex->OGL.TexId, colorFormat, AlphaHint::ONE, Texture::DIM_2D, false));
            if (notNull(depthTex)) {
                m_framebuffer[i]->set(Framebuffer::DEPTH,  Texture::fromGLTexture(format("%s->depth[%d]", name.c_str(), i), depthTex->OGL.TexId, depthFormat, AlphaHint::ONE, Texture::DIM_2D, false));
            }
        }
    }

    /** Access the underlying swap set. Used only for submitting to an Oculus layer */
    ovrSwapTextureSet* colorTextureSet() const {
        return m_colorTextureSet;
    }

    /** Access the underlying swap set. Used only for submitting to an Oculus layer */
    ovrSwapTextureSet* depthTextureSet() const {
        return m_depthTextureSet;
    }

    virtual ~OVRFramebufferQueue() {
        ovr_DestroySwapTextureSet(m_hmd, m_colorTextureSet);
        if (notNull(m_depthTextureSet)) {
            ovr_DestroySwapTextureSet(m_hmd, m_depthTextureSet);
        }
        m_framebuffer.clear();
    }

    /** Of the frame buffers */
    Vector2 vector2Bounds() const {
        return m_framebuffer[0]->vector2Bounds();
    }

    /** Of the frame buffers */
    int width() const {
        return m_framebuffer[0]->width();
    }

    /** Of the frame buffers */
    int height() const {
        return m_framebuffer[0]->height();
    }

    /** Prepare for the next frame */
    void advance() {
        m_colorTextureSet->CurrentIndex = (m_colorTextureSet->CurrentIndex + 1) % m_colorTextureSet->TextureCount;
        if (notNull(m_depthTextureSet)) {
            m_depthTextureSet->CurrentIndex = (m_depthTextureSet->CurrentIndex + 1) % m_depthTextureSet->TextureCount;
        }
    }

    const shared_ptr<Texture>& currentColorTexture() const {
        return m_framebuffer[m_colorTextureSet->CurrentIndex]->texture(0);
    }

    const shared_ptr<Framebuffer>& currentFramebuffer() const {
        return m_framebuffer[m_colorTextureSet->CurrentIndex];
    }
};


/** Data needed for any VR app */
struct ovrState {
    ovrSession              m_hmd;
    ovrEyeRenderDesc        eyeRenderDesc[2];
    OVRFramebufferQueue*    eyeFramebufferQueue[2];
    OVRFramebufferQueue*    hudFramebufferQueue;   
    ovrHmdDesc              hmdDesc;

    /** If true, create a "mirror" FBO for showing the post-compositing data
    that is sent to the Oculus m_hmd on a normal monitor for debugging. */
    const bool              debugMirrorHMDToScreen;

    shared_ptr<Framebuffer> debugMirrorFramebuffer;

    /** Used only if debugMirrorHMDToScreen == true */
    ovrTexture*             debugMirrorTexture;

    ovrState(bool debugMirrorHMDToScreen, int logMask = OVR::LogMask_All) :
        debugMirrorHMDToScreen(debugMirrorHMDToScreen), debugMirrorTexture(NULL) {
        eyeFramebufferQueue[0] = eyeFramebufferQueue[1] = NULL;
        hudFramebufferQueue = NULL;

        OVR::System::Init(OVR::Log::ConfigureDefaultLog(logMask));
    }

    /** Call before initializing OpenGL. Returns 0 on success, throws a string on error. */
    void init() {
        // Initialise rift
        if (ovr_Initialize(nullptr) != ovrSuccess) { throw "Unable to initialize libOVR."; }

        ovrGraphicsLuid luid;
        ovrResult result = ovr_Create(&m_hmd, &luid);
        if (result != ovrSuccess) { ovr_Shutdown(); throw "Oculus Rift not detected."; }

        // Did we end up with a debug HMD?
        bool usingDebugHmd = (ovr_GetEnabledCaps(m_hmd) & ovrHmdCap_DebugDevice) != 0;
       
        hmdDesc = ovr_GetHmdDesc(m_hmd);

        eyeRenderDesc[0] = ovr_GetRenderDesc(m_hmd, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
        eyeRenderDesc[1] = ovr_GetRenderDesc(m_hmd, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);

        // Start the sensor (not needed in SDK 0.8
        // result = ovr_ConfigureTracking(m_hmd, ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection | ovrTrackingCap_Position, 0);
        //alwaysAssertM(result == ovrSuccess, "Could not configure tracking");
    }

    /** Call after OpenGL is initialized. windowWidth and windowHeight are the size of the mirror screen display,
    and are independent of the m_hmd resolution.  These are only needed if debugMirrorHMDToScreen == true */
    void initRenderBuffers(int windowWidth, int windowHeight) {
        const ImageFormat* colorFormat = ImageFormat::RGBA8();
        const ImageFormat* depthFormat = GLCaps::supportsTexture(ImageFormat::DEPTH32F()) ? ImageFormat::DEPTH32F() : ImageFormat::DEPTH24();

        // Make eye render buffers
        for (int i = 0; i < 2; ++i) {
            const ovrSizei size = ovr_GetFovTextureSize(m_hmd, (ovrEyeType)i, hmdDesc.DefaultEyeFov[i], 1);
            eyeFramebufferQueue[i] = new OVRFramebufferQueue(format("hmd.eyeFramebufferQueue[%d]", i), m_hmd, size, colorFormat, depthFormat);
        }

        {
            ovrSizei size;
            size.w = windowWidth;
            size.h = windowHeight;
            hudFramebufferQueue = new OVRFramebufferQueue("hmd.hudFramebufferQueue", m_hmd, size);
        }

        // Request SDK 0.6.0.1-level queuing, which lets frame submission run slightly ahead of the display
        ovr_SetBool(m_hmd, "QueueAheadEnabled", true);

        if (debugMirrorHMDToScreen) {
            // Create mirror texture and an FBO used to copy mirror texture to back buffer
            ovrResult result = ovr_CreateMirrorTextureGL(m_hmd, GL_RGBA, windowWidth, windowHeight, &debugMirrorTexture);
            alwaysAssertM(result == ovrSuccess, "Could not allocate Oculus mirror texture");

            // Configure the mirror read buffer. This is only for debugging.
            const shared_ptr<Texture>& debugMirrorG3DTexture = Texture::fromGLTexture("VRApp Debug Mirror Texture",
                reinterpret_cast<ovrGLTexture*>(debugMirrorTexture)->OGL.TexId,
                ImageFormat::RGBA8(),
                AlphaHint::ONE,
                Texture::DIM_2D,
                false);

            debugMirrorFramebuffer = Framebuffer::create(debugMirrorG3DTexture);
            debugMirrorFramebuffer->bind();
        }
    }

    /** Call at application end, before OpenGL is shut down */
    void cleanup() {
        // Cleanup
        if (debugMirrorHMDToScreen) {
            ovr_DestroyMirrorTexture(m_hmd, debugMirrorTexture);
            debugMirrorTexture = NULL;
        }
    }


    /** Call after OpenGL is shut down */
    ~ovrState() {
        ovr_Destroy(m_hmd);
        ovr_Shutdown();
        OVR::System::Destroy();
    }

    static void quaternionToMatrix(const float* quat, float* mat) {
        mat[0] = 1.0f - 2.0f * quat[1] * quat[1] - 2.0f * quat[2] * quat[2];
        mat[4] = 2.0f * quat[0] * quat[1] + 2.0f * quat[3] * quat[2];
        mat[8] = 2.0f * quat[2] * quat[0] - 2.0f * quat[3] * quat[1];
        mat[12] = 0.0f;

        mat[1] = 2.0f * quat[0] * quat[1] - 2.0f * quat[3] * quat[2];
        mat[5] = 1.0f - 2.0f * quat[0] * quat[0] - 2.0f * quat[2] * quat[2];
        mat[9] = 2.0f * quat[1] * quat[2] + 2.0f * quat[3] * quat[0];
        mat[13] = 0.0f;

        mat[2] = 2.0f * quat[2] * quat[0] + 2.0f * quat[3] * quat[1];
        mat[6] = 2.0f * quat[1] * quat[2] - 2.0f * quat[3] * quat[0];
        mat[10] = 1.0f - 2.0f * quat[0] * quat[0] - 2.0f * quat[1] * quat[1];
        mat[14] = 0.0f;

        mat[3] = mat[7] = mat[11] = 0.0f;
        mat[15] = 1.0f;
    }

    /** Returns the OpenGL GL_MODELVIEW matrix for the specified eye and head, relative to the calibration-center head position.

    Use ovrHmd_GetFloat(oculusHMD.m_hmd, OVR_KEY_EYE_HEIGHT, OVR_DEFAULT_EYE_HEIGHT), 0.0f) to get the vertical (y-axis) default
    eye height.

    Use ovrMatrix4f_Projection to get the projection matrix.
    */
    void getEyeMatrix(const ovrPosef& eyeRenderPose, int eye, float* matrix) const {
        quaternionToMatrix(&eyeRenderPose.Orientation.x, matrix);

        // Multiply the rotation matrix by the head translation matrix
        const float* headOffsetVector = &eyeRenderPose.Position.x;
        for (int r = 0; r < 3; ++r) {
            // Matrix-vector product. We only need the upper 3x3 rotation times the vector.
            // Since we're computing the INVERSE of the camera matrix, we use the negative
            // vector.
            for (int c = 0; c < 3; ++c) {
                matrix[4 * 3 + r] += matrix[4 * c + r] * -headOffsetVector[c];
            }
        }

    }
};

} // namespace
#endif
