/** \file VRApp.cpp

    G3D Innovation Engine
    Copyright 2000-2016, Morgan McGuire.
    All rights reserved.
*/
#include "G3D/Log.h"
#include "GLG3D/glheaders.h"
#include "GLG3D/Draw.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/UserInput.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/MarkerEntity.h"
#include "GLG3DVR/VRApp.h"

namespace G3D {

/** Called by initOpenVR */
static String getHMDString(vr::IVRSystem* pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError = nullptr) {
	uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, nullptr, 0, peError);
	if (unRequiredBufferLen == 0) {
	    return "";
    }

	char* pchBuffer = new char[unRequiredBufferLen];
	unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
	String sResult = pchBuffer;
	delete[] pchBuffer;

	return sResult;
}


static float getHMDFloat(vr::IVRSystem* pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError = nullptr) {
	return pHmd->GetFloatTrackedDeviceProperty(unDevice, prop, peError);
}


VRApp::VRApp(const GApp::Settings& settings) :
    super(makeFixedSize(settings), nullptr, nullptr, false),
    m_vrSubmitToDisplayMode(SubmitToDisplayMode::MINIMIZE_LATENCY),
    m_highQualityWarping(true),
    m_numSlowFrames(0),
    m_hudEnabled(false),
    m_hudWidth(2.0f),
    m_hudFrame(Point3(0.0f, -0.27f, -1.2f)),
    m_hudBackgroundColor(Color3::black(), 0.15f) {

    const VRApp::Settings* vrSettings = dynamic_cast<const VRApp::Settings*>(&settings);

    if (notNull(vrSettings)) {
        m_vrSettings = vrSettings->vr;
    }
    
    uint32_t hmdWidth = settings.window.width, hmdHeight = settings.window.height;

    ////////////////////////////////////////////////////////////
    // Initialize OpenVR

    try {
	    vr::EVRInitError eError = vr::VRInitError_None;
	    m_hmd = vr::VR_Init(&eError, vr::VRApplication_Scene);

	    if (eError != vr::VRInitError_None) {
            throw vr::VR_GetVRInitErrorAsEnglishDescription(eError);
	    }
    
        if (isNull(m_hmd)) {
            throw "No HMD";
        }

        //get the proper resolution of the hmd
        m_hmd->GetRecommendedRenderTargetSize(&hmdWidth, &hmdHeight);

	    const String& driver = getHMDString(m_hmd, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
	    const String& model  = getHMDString(m_hmd, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_ModelNumber_String);        
	    const String& serial = getHMDString(m_hmd, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);
        const float   freq   = getHMDFloat(m_hmd, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_DisplayFrequency_Float);
        logLazyPrintf("VRApp::m_hmd: %s '%s' #%s (%d x %d @ freq)\n", driver.c_str(), model.c_str(), serial.c_str(), hmdWidth, hmdHeight, freq);

        // Initialize the compositor
        vr::IVRCompositor* compositor = vr::VRCompositor();
	    if (! compositor) {
            vr::VR_Shutdown();
            m_hmd = nullptr;
            throw "OpenVR Compositor initialization failed. See log file for details\n";
	    }
    } catch (const String& error) {
        logLazyPrintf("OpenVR Initialization Error: %s\n", error.c_str());
    }
    /////////////////////////////////////////////////////////////

    // Now initialize OpenGL and RenderDevice
    initializeOpenGL(nullptr, nullptr, true, makeFixedSize(settings));

    // Mark as invalid
    m_previousEyeFrame[0].translation = Vector3::nan();
    m_eyeFrame[0].translation = Vector3::nan();

    if (notNull(m_hmd)) {
        // This will happen to recreate the m_gbuffer, but that is the only way to change its name
        // and affect the underlying textures
        for (int i = 0; i < 2; ++i) {
            m_gbufferArray[i] = GBuffer::create(m_gbufferSpecification, format("m_gbufferArray[%d]", i));
            m_gbufferArray[i]->resize(m_gbuffer->width(), m_gbuffer->height());
        }
        m_gbuffer = m_gbufferArray[0];

        if (notNull(m_hmd)) {
            setSubmitToDisplayMode(SubmitToDisplayMode::MAXIMIZE_THROUGHPUT);
        }
    } else {
        m_gbufferArray[0] = m_gbuffer;
        m_gbufferArray[1] = nullptr;
        m_vrEyeCamera[1]  = nullptr;
        m_hmdDeviceFramebuffer[1] = nullptr;
    }

    // Construct the eye cameras, framebuffers, and head entity
    const ImageFormat* ldrColorFormat = ImageFormat::RGBA8();
    const ImageFormat* hdrColorFormat = GLCaps::firstSupportedTexture(settings.film.preferredColorFormats);
    const ImageFormat* depthFormat = GLCaps::firstSupportedTexture(settings.film.preferredDepthFormats);
    for (int eye = 0; eye < numEyes(); ++eye) {
        static const String NAME[2] = {"VRApp::m_vrEyeCamera[0]", "VRApp::m_vrEyeCamera[1]"};
        m_vrEyeCamera[eye] = Camera::create(NAME[eye]);
        m_vrEyeCamera[eye]->setShouldBeSaved(false);

        m_hmdDeviceFramebuffer[eye] = Framebuffer::create
           (Texture::createEmpty(format("VRApp::m_hmdDeviceFramebuffer[%d]/color", eye), hmdWidth, hmdHeight, ldrColorFormat),
            Texture::createEmpty(format("VRApp::m_hmdDeviceFramebuffer[%d]/depth", eye), hmdWidth, hmdHeight, depthFormat));
        m_hmdDeviceFramebuffer[eye]->setInvertY(true);

        // Share the depth buffer with the LDR device target
        m_hmdFramebuffer[eye] = Framebuffer::create
           (Texture::createEmpty(format("VRApp::m_hmdFramebuffer[%d]/color", eye), hmdWidth, hmdHeight, hdrColorFormat),
            m_hmdDeviceFramebuffer[eye]->texture(Framebuffer::DEPTH));
    }

    // Introduce the head entity
    m_vrHead = MarkerEntity::create("VRApp::m_vrHead");
    m_vrHead->setShouldBeSaved(false);

    m_debugController->setMoveRate(0.3f);
}


const GApp::Settings& VRApp::makeFixedSize(const GApp::Settings& s) {
    const_cast<GApp::Settings&>(s).window.resizable = false;
    return s;
}


void VRApp::onInit() {
    super::onInit();
    m_currentEyeIndex = 0;

    setSubmitToDisplayMode(SubmitToDisplayMode::MAXIMIZE_THROUGHPUT);

    const float freq = isNull(m_hmd) ? 60.0f : getHMDFloat(m_hmd, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_DisplayFrequency_Float);
    setFrameDuration(1.0f / freq);

    // Force the m_film to match the m_hmd's resolution instead of the OSWindow's
    resize(0, 0);

    m_cursorPointerTexture = Texture::fromFile(System::findDataFile("gui/cursor-pointer.png"), ImageFormat::RGBA8());
    sampleTrackingData();
}


/**
 Copied from G3D's minimalOpenVR.h
 */
static void getEyeTransformations
   (vr::IVRSystem*  hmd,
    vr::TrackedDevicePose_t* trackedDevicePose,
    float           nearPlaneZ,
    float           farPlaneZ,
    float*          headToWorldRowMajor4x3,
    float*          ltEyeToHeadRowMajor4x3, 
    float*          rtEyeToHeadRowMajor4x3,
    float*          ltProjectionMatrixRowMajor4x4, 
    float*          rtProjectionMatrixRowMajor4x4) {

    assert(nearPlaneZ < 0.0f && farPlaneZ < nearPlaneZ);

    vr::VRCompositor()->WaitGetPoses(trackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);

#   if defined(_DEBUG) && 0
        fprintf(stderr, "Devices tracked this frame: \n");
        int poseCount = 0;
	    for (int d = 0; d < vr::k_unMaxTrackedDeviceCount; ++d)	{
		    if (trackedDevicePose[d].bPoseIsValid) {
			    ++poseCount;
			    switch (hmd->GetTrackedDeviceClass(d)) {
                case vr::TrackedDeviceClass_Controller:        fprintf(stderr, "   Controller: ["); break;
                case vr::TrackedDeviceClass_HMD:               fprintf(stderr, "   HMD: ["); break;
                case vr::TrackedDeviceClass_Invalid:           fprintf(stderr, "   <invalid>: ["); break;
                case vr::TrackedDeviceClass_Other:             fprintf(stderr, "   Other: ["); break;
                case vr::TrackedDeviceClass_TrackingReference: fprintf(stderr, "   Reference: ["); break;
                default:                                       fprintf(stderr, "   ???: ["); break;
			    }
                for (int r = 0; r < 3; ++r) { 
                    for (int c = 0; c < 4; ++c) {
                        fprintf(stderr, "%g, ", trackedDevicePose[d].mDeviceToAbsoluteTracking.m[r][c]);
                    }
                }
                fprintf(stderr, "]\n");
		    }
	    }
        fprintf(stderr, "\n");
#   endif

    assert(trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid);
    const vr::HmdMatrix34_t head = trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;

    const vr::HmdMatrix34_t& ltMatrix = hmd->GetEyeToHeadTransform(vr::Eye_Left);
    const vr::HmdMatrix34_t& rtMatrix = hmd->GetEyeToHeadTransform(vr::Eye_Right);

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) {
            ltEyeToHeadRowMajor4x3[r * 4 + c] = ltMatrix.m[r][c];
            rtEyeToHeadRowMajor4x3[r * 4 + c] = rtMatrix.m[r][c];
            headToWorldRowMajor4x3[r * 4 + c] = head.m[r][c];
        }
    }

    const vr::HmdMatrix44_t& ltProj = hmd->GetProjectionMatrix(vr::Eye_Left,  -nearPlaneZ, -farPlaneZ, vr::API_OpenGL);
    const vr::HmdMatrix44_t& rtProj = hmd->GetProjectionMatrix(vr::Eye_Right, -nearPlaneZ, -farPlaneZ, vr::API_OpenGL);

    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            ltProjectionMatrixRowMajor4x4[r * 4 + c] = ltProj.m[r][c];
            rtProjectionMatrixRowMajor4x4[r * 4 + c] = rtProj.m[r][c];
        }
    }
}


void VRApp::sampleTrackingData() {
    // TODO: handle the no-HMD case by gluing a single eye to the camera
    debugAssert(notNull(m_hmd));

    BEGIN_PROFILER_EVENT("VRApp::sampleTrackingData");

    // Update the G3D VR eye cameras. This is not a reference in case activeCamera() changes.
    const shared_ptr<Camera> bodyCamera = activeCamera();

    // Read the tracking state
    Matrix4 headToBodyRowMajor4x3;
    Matrix4 eyeToHeadRowMajor4x3[2];
    Matrix4 projectionMatrixRowMajor4x4[2];

    getEyeTransformations
       (m_hmd,
        m_trackedDevicePose,
        bodyCamera->nearPlaneZ(),
        bodyCamera->farPlaneZ(),
        headToBodyRowMajor4x3,
        eyeToHeadRowMajor4x3[0], 
        eyeToHeadRowMajor4x3[1],
        projectionMatrixRowMajor4x4[0], 
        projectionMatrixRowMajor4x4[1]);

    const CFrame& headToBody = headToBodyRowMajor4x3.approxCoordinateFrame();

    for (int eye = 0; eye < numEyes(); ++eye) {
        m_previousEyeFrame[eye] = m_eyeFrame[eye];
        m_eyeFrame[eye] = headToBody * eyeToHeadRowMajor4x3[eye].approxCoordinateFrame();

        if (m_previousEyeFrame[eye].translation.isNaN()) {
            // First frame of animation--override with no transform
            m_previousEyeFrame[eye] = m_eyeFrame[eye];
        }

        m_vrEyeCamera[eye]->copyParametersFrom(bodyCamera);
        if (m_vrSettings.overrideMotionBlur) {
            m_vrEyeCamera[eye]->motionBlurSettings() = m_vrSettings.motionBlurSettings;
        }
        if (m_vrSettings.overrideDepthOfField) {
            m_vrEyeCamera[eye]->depthOfFieldSettings() = m_vrSettings.depthOfFieldSettings;
        }

        const Projection& projection = Projection(projectionMatrixRowMajor4x4[eye], m_hmdDeviceFramebuffer[eye]->vector2Bounds());
        m_vrEyeCamera[eye]->setProjection(projection);

        const CFrame previousWorldFrame(maybeRemovePitchAndRoll(bodyCamera->previousFrame()) * m_previousEyeFrame[eye]);
        const CFrame worldFrame(maybeRemovePitchAndRoll(bodyCamera->frame()) * m_eyeFrame[eye]);

        // To get correct motion blur, we need to properly set the previous frame, which we do by 
        // removing any track, setting the frame to the previous frame, simulating, the setting the frame to the current frame.
        //
        // After this process,
        //    eyeCamera->previousFrame() == previousWorldFrame and eyeCamera->frame() == worldFrame;
        m_vrEyeCamera[eye]->setPreviousFrame(previousWorldFrame);
        m_vrEyeCamera[eye]->setFrame(worldFrame);
    }

    // Update the head entity's frame and previous frame
    CFrame frame = m_vrEyeCamera[0]->frame();
    frame.translation = (frame.translation + m_vrEyeCamera[1]->frame().translation) / 2.0f;
    m_vrHead->setFrame(frame);

    frame = m_vrEyeCamera[0]->previousFrame();
    frame.translation = (frame.translation + m_vrEyeCamera[1]->previousFrame().translation) / 2.0f;
    m_vrHead->setPreviousFrame(frame);

    //    m_externalCameraFrame = m_vrHead->frame() * bodySpaceHead.inverse() * bodySpaceTracker;

    END_PROFILER_EVENT();
}


void VRApp::swapBuffers() {
    // Intentionally empty...prevent subclasses from accidentally swapping buffers on their own
}


/**
    Returns the same CFrame, but with only yaw preserved */
CFrame VRApp::maybeRemovePitchAndRoll(const CFrame& source) const {
    if (m_vrSettings.trackingOverridesPitch && notNull(dynamic_pointer_cast<FirstPersonManipulator>(m_cameraManipulator))) {
        float x, y, z, yaw, pitch, roll;
        source.getXYZYPRRadians(x, y, z, yaw, pitch, roll);
        return CFrame::fromXYZYPRRadians(x, y, z, yaw, 0.0f, 0.0f);
    } else {
        return source;
    }
}


void VRApp::onBeforeSimulation(RealTime& rdt, SimTime& sdt, SimTime& idt) {
    if (m_vrSettings.trackingOverridesPitch) {
        // Use the pitch and roll from head tracking (which will then be stripped from the body camera by maybeRemovePitchAndRoll())
        // and all other degrees of freedom from the manipulator itself. This will cause the body to always pitch and roll with the
        // head, but to yaw with explicit controls.
        float pitch, roll, ignore;
        m_eyeFrame[0].getXYZYPRRadians(ignore, ignore, ignore, ignore, pitch, roll);
        const shared_ptr<FirstPersonManipulator>& fpm = dynamic_pointer_cast<FirstPersonManipulator>(m_cameraManipulator);
        if (notNull(fpm)) {
            fpm->setPitch(-pitch);
        }
    }
}


void VRApp::onGraphics(RenderDevice* rd, Array<shared_ptr<Surface> >& posed3D, Array<shared_ptr<Surface2D> >& posed2D) {
    debugAssertM(!renderDevice->swapBuffersAutomatically(), "VRApp subclasses must not swap buffers automatically.");
    rd->pushState(); {

        debugAssert(notNull(activeCamera()));

        // Begin VR-specific
        if (m_vrSubmitToDisplayMode == SubmitToDisplayMode::BALANCE) {
            // Submit the PREVIOUS frame
            submitHMDFrame(rd);
        }

        // Render the main display's GUI
        if (! m_hudEnabled) {
            rd->push2D(); {
                onGraphics2D(rd, posed2D);
            } rd->pop2D();
        }

        if ((submitToDisplayMode() == SubmitToDisplayMode::MAXIMIZE_THROUGHPUT) && (! rd->swapBuffersAutomatically())) {
            super::swapBuffers();
        }
        rd->clear();

        if (m_vrSubmitToDisplayMode == SubmitToDisplayMode::MAXIMIZE_THROUGHPUT) {
            // Submit the PREVIOUS frame
            submitHMDFrame(rd);
        }

        // Begin the NEW frame
        sampleTrackingData();

        BEGIN_PROFILER_EVENT("Rendering");

        shared_ptr<Framebuffer> oldFB = m_framebuffer;

        // No reference because we're going to mutate the active camera
        const shared_ptr<Camera> bodyCamera = activeCamera();
        for (m_currentEyeIndex = 0; m_currentEyeIndex < 2; ++m_currentEyeIndex) {
            const int eye = m_currentEyeIndex;

            // Switch to eye render target for the display itself (we assume that onGraphics3D will probably
            // bind its own HDR buffer and then resolve to this one.)
            const shared_ptr<Framebuffer>& vrFB = m_hmdDeviceFramebuffer[eye];

            // Swap out the underlying framebuffer that is "current" on the GApp
            m_framebuffer = m_hmdFramebuffer[eye];
            rd->pushState(vrFB); {

                setActiveCamera(m_vrEyeCamera[eye]);

                m_gbuffer = m_gbufferArray[m_currentEyeIndex];
                //rd->setProjectionAndCameraMatrix(eyeCamera->projection(), eyeCamera->frame());
                onGraphics3D(rd, posed3D);
            } rd->popState();
        }
        setActiveCamera(bodyCamera);

        m_framebuffer = oldFB;

        END_PROFILER_EVENT();

# if 0 // TODO: HUD
        // Increment to use next texture, just before writing
        if (m_hudEnabled) {
            m_hmd->hudFramebufferQueue->advance();
            rd->push2D(m_hmd->hudFramebufferQueue->currentFramebuffer()); {
                rd->setColorClearValue(m_hudBackgroundColor);
                rd->clear();
                onGraphics2D(rd, posed2D);

                // Draw the cursor, if visible
                if (window()->mouseHideCount() < 1) {
                    const Point2 cursorPointerTextureHotspot(1, 1);
                    rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
                    // Clamp to screen so that the mouse is never invisible
                    const Point2 mousePos = clamp(userInput->mouseXY(), Point2(0.0f, 0.0f), Point2(float(window()->width()), float(window()->height())));
                    Draw::rect2D(m_cursorPointerTexture->rect2DBounds() + mousePos - cursorPointerTextureHotspot, rd, Color3::white(), m_cursorPointerTexture);
                }
            } rd->pop2D();
        }

#endif
        
        if (m_vrSubmitToDisplayMode == SubmitToDisplayMode::MINIMIZE_LATENCY) {
            // Submit the CURRENT frame
            submitHMDFrame(rd);
        }

        if (m_vrSettings.debugMirrorMode == DebugMirrorMode::PRE_DISTORTION) {
            // Mirror to the screen
            rd->push2D(m_monitorDeviceFramebuffer); {
                rd->setColorClearValue(Color3::black());
                rd->clear();
                for (int eye = 0; eye < 2; ++eye) {
                    const shared_ptr<Texture>& finalImage = m_hmdDeviceFramebuffer[eye]->texture(0);

                    // Find the scale needed to fit both images on screen
                    const float scale = min(float(rd->width()) * 0.5f / float(finalImage->width()), float(rd->height()) / float(finalImage->height()));
                    const int xShiftDirection = 2 * eye - 1;

                    const float width = finalImage->width()  * scale;
                    const float height = finalImage->height() * scale;

                    const Rect2D& rect = Rect2D::xywh((rd->width() + width * float(xShiftDirection - 1)) * 0.5f, 0.0f, width, height);
                    Draw::rect2D(rect, rd, Color3::white(), finalImage, Sampler::video(), true);
                }
            } rd->pop2D();
        }

        // TODO: is this needed?
        glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);

        // Tell G3D that we unmapped the framebuffer
        rd->setFramebuffer(nullptr);

    } rd->popState();

    maybeAdjustEffects();
}


void VRApp::maybeAdjustEffects() {
    const RealTime frameTime = 1.0 / renderDevice->stats().frameRate;
    const RealTime targetTime = realTimeTargetDuration();

    // Allow 5% overhead for roundoff
    if (m_vrSettings.disablePostEffectsIfTooSlow && (frameTime > targetTime * 1.05f)) {
        ++m_numSlowFrames;
        if (m_numSlowFrames > MAX_SLOW_FRAMES) {
            m_numSlowFrames = 0;
            decreaseEffects();
        }
    } // Over time
}


void VRApp::decreaseEffects() {
    // Disable some effects
    if (activeCamera()->filmSettings().bloomStrength() > 0) {
        // Turn off bloom
        activeCamera()->filmSettings().setBloomStrength(0.0f);
        debugPrintf("VRApp::decreaseEffects() Disabled bloom to increase performance.\n");

    } else if (scene()->lightingEnvironment().ambientOcclusionSettings.enabled && scene()->lightingEnvironment().ambientOcclusionSettings.useDepthPeelBuffer) {
        // Use faster AO
        scene()->lightingEnvironment().ambientOcclusionSettings.useDepthPeelBuffer = false;
        debugPrintf("VRApp::decreaseEffects() Disabled depth-peeled AO to increase performance.\n");

    } else if (scene()->lightingEnvironment().ambientOcclusionSettings.enabled) {
        // Disable AO
        scene()->lightingEnvironment().ambientOcclusionSettings.enabled = false;
        debugPrintf("VRApp::decreaseEffects() Disabled AO to increase performance.\n");

    } else if (activeCamera()->filmSettings().antialiasingHighQuality()) {
        // Disable high-quality FXAA
        activeCamera()->filmSettings().setAntialiasingHighQuality(false);
        debugPrintf("VRApp::decreaseEffects() Disabled high-quality antialiasing to increase performance.\n");

    } else if (activeCamera()->filmSettings().antialiasingEnabled()) {
        // Disable FXAA
        activeCamera()->filmSettings().setAntialiasingEnabled(false);
        debugPrintf("VRApp::decreaseEffects() Disabled antialiasing to increase performance.\n");
    }
}


void VRApp::submitHMDFrame(RenderDevice* rd) {
    // G3D::Film already converted to linear
    const vr::EColorSpace colorSpace = vr::ColorSpace_Linear;

    for (int eye = 0; eye < 2; ++eye) {
        const vr::Texture_t tex = { reinterpret_cast<void*>(intptr_t(m_hmdDeviceFramebuffer[eye]->texture(0)->openGLID())), vr::API_OpenGL, colorSpace };
        vr::VRCompositor()->Submit(vr::EVREye(eye), &tex);
    }

    // TODO: HUD overlay

    // Tell the compositor to begin work immediately instead of waiting for 
    // the next WaitGetPoses() call
    vr::VRCompositor()->PostPresentHandoff();

#if 0
    // Do distortion rendering, Present and flush/sync
    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    // Tell G3D that we disabled the framebuffer
    rd->setFramebuffer(nullptr);

    ovrViewScaleDesc viewScaleDesc;
    ovrLayer_Union   eyeLayer;

    viewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;
    viewScaleDesc.HmdToEyeViewOffset[0] = m_viewOffset[0];
    viewScaleDesc.HmdToEyeViewOffset[1] = m_viewOffset[1];

    eyeLayer.Header.Type = ovrLayerType_EyeFovDepth;
    if (m_highQualityWarping) {
        eyeLayer.Header.Flags = ovrLayerFlag_HighQuality;
    }

    // G3D uses the DirectX texture convention, so doesn't need 
    // ovrLayerFlag_TextureOriginAtBottomLeft here

    // 3D
    for (int eye = 0; eye < 2; ++eye) {
        ovrSizei size;
        size.w = m_hmd->eyeFramebufferQueue[eye]->width();
        size.h = m_hmd->eyeFramebufferQueue[eye]->height();

        eyeLayer.EyeFov.Viewport[eye]          = OVR::Recti(size);
        eyeLayer.EyeFov.Fov[eye]               = m_hmd->hmdDesc.DefaultEyeFov[eye];
        eyeLayer.EyeFov.RenderPose[eye]        = m_eyeRenderPose[eye];

        eyeLayer.EyeFov.ColorTexture[eye]      = m_hmd->eyeFramebufferQueue[eye]->colorTextureSet();
        eyeLayer.EyeFovDepth.DepthTexture[eye] = m_hmd->eyeFramebufferQueue[eye]->depthTextureSet();
        eyeLayer.EyeFovDepth.ProjectionDesc    = m_posTimewarpProjectionDesc;
    }

    // HUD
    ovrLayerQuad hudLayer;
    switch (m_vrSettings.hudSpace) {
    case FrameName::CAMERA:
        hudLayer.Header.Type = ovrLayerType_Quad;
        hudLayer.Header.Flags = ovrLayerFlag_HeadLocked;
        hudLayer.QuadPoseCenter = toOVR(m_hudFrame);
        break;

    case FrameName::WORLD:
        hudLayer.Header.Type = ovrLayerType_Quad;
        hudLayer.Header.Flags = 0;
        // This position is in body space
        hudLayer.QuadPoseCenter = toOVR(m_hudFrame * maybeRemovePitchAndRoll(activeCamera()->frame()).inverse());
        break;

    case FrameName::OBJECT: // Body
    default:
        debugAssertM(m_vrSettings.hudSpace == FrameName::OBJECT, "The only allowed values for hudSpace are CAMERA (eye), OBJECT (body), and WORLD");
        hudLayer.Header.Type = ovrLayerType_Quad;
        hudLayer.Header.Flags = 0;
        // This position is in body space because Oculus doesn't know about our world space
        hudLayer.QuadPoseCenter = toOVR(m_hudFrame);
        break;    
    }

    if (m_highQualityWarping) {
        hudLayer.Header.Flags = ovrLayerFlag_HighQuality;
    }

    hudLayer.ColorTexture = m_hmd->hudFramebufferQueue->colorTextureSet();
   
    const float hudMetersPerPixel = m_hudWidth * 1.0f / m_hmd->hudFramebufferQueue->width();
    // In meters
    hudLayer.QuadSize.x     = m_hmd->hudFramebufferQueue->width() * hudMetersPerPixel;
    hudLayer.QuadSize.y     = m_hmd->hudFramebufferQueue->height() * hudMetersPerPixel;
    
    // In pixels
    ovrSizei size;
    size.w = m_hmd->hudFramebufferQueue->width();
    size.h = m_hmd->hudFramebufferQueue->height();
    hudLayer.Viewport       = OVR::Recti(size);

    // Actual submission
    ovrLayerHeader*  layerPtrList[2] = { &eyeLayer.Header, &hudLayer.Header };
    const int layerCount = m_hudEnabled ? 2 : 1;
    BEGIN_PROFILER_EVENT("ovrHmd_SubmitFrame");
    // This will cause the screen to go black on NVIDIA GPUs for a moment the first time that it is called
    ovrResult        result = ovr_SubmitFrame(m_hmd->m_hmd, 0, &viewScaleDesc, layerPtrList, layerCount);
    debugAssert(result == ovrSuccess);
    END_PROFILER_EVENT();

    if (m_hmd->debugMirrorHMDToScreen) {
        alwaysAssertM(m_vrSettings.debugMirrorMode == DebugMirrorMode::POST_DISTORTION,
            "Cannot change debugMirrorMode after initialization.");
        m_hmd->debugMirrorFramebuffer->blitTo(rd, nullptr, true);
    }
#endif
}


void VRApp::onCleanup() {
    // Called after the application loop ends.  Place a majority of cleanup code
    // here instead of in the constructor so that exceptions can be caught.
    if (notNull(m_hmd)) {
        vr::VR_Shutdown();
        m_hmd = nullptr;
    }

    GApp::onCleanup();
}


void VRApp::onAfterLoadScene(const Any& any, const String& sceneName) {
    super::onAfterLoadScene(any, sceneName);

    // Give a grace period for initialization
    m_numSlowFrames = -30;

    // Default to good warping
    m_highQualityWarping = true;

    // Add the head and eyes to the scene
    scene()->insert(m_vrHead);
    scene()->insert(m_vrEyeCamera[0]);
    scene()->insert(m_vrEyeCamera[1]);
}


bool VRApp::onEvent(const GEvent& event) {
    // Handle super-class events
    if (super::onEvent(event)) { return true; }

    // HUD toggle
    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::TAB)) { 
        m_hudEnabled = ! m_hudEnabled;
        if (m_hudEnabled) {
            // Capture the mouse to the window
            window()->incInputCaptureCount();
        } else {
            window()->decInputCaptureCount();
        }
        return true; 
    } else if ((event.type == GEventType::MOUSE_MOTION) && m_hudEnabled) {
        // If the mouse moved outside of the allowed bounds, move it back
        const Point2& p = event.mousePosition();
        const Point2& size = m_hmdDeviceFramebuffer[0]->vector2Bounds() - Vector2::one(); 
        if ((p.x < 0) || (p.y < 0) || (p.x > size.x) || (p.y > size.y)) {
            window()->setRelativeMousePosition(p.clamp(Vector2::zero(), size));
        }
        return false;
    }

    return false;
}


} // namespace