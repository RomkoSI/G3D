/** \file VRApp.cpp.h

This is packaged as a header file so that it will not be compiled 
by the default G3D build. That is necessary because VRApp depends
on the Oculus SDK, which is not distributed with G3D.

G3D Innovation Engine
Copyright 2000-2015, Morgan McGuire.
All rights reserved.
*/
#include "GLG3D/glheaders.h"
#include "GLG3D/Draw.h"
#include "GLG3D/UserInput.h"
#include "GLG3D/MarkerEntity.h"
#include "GLG3DVR/g3doculus.h"
#include "GLG3DVR/VRApp.h"

namespace G3D {

VRApp::VRApp(const GApp::Settings& settings) :
    GApp(makeFixedSize(settings)),
    m_vrSubmitToDisplayMode(SubmitToDisplayMode::BALANCE),
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

    m_hmd = new ovrState(m_vrSettings.debugMirrorMode == DebugMirrorMode::POST_DISTORTION);
    try {
        m_hmd->init();
    } catch (const char* error) {
        // Most probable cause of failure: the Oculus Runtime is not installed!
        alwaysAssertM(false, error);
        exit(-1);
    }

    // Mark as invalid
    m_previousEyeFrame[0].translation = Vector3::nan();
    m_eyeFrame[0].translation = Vector3::nan();

    // This will happen to recreate the m_gbuffer, but that is the only way to change its name
    // and affect the underlying textures
    for (int i = 0; i < 2; ++i) {
        m_gbufferArray[i] = GBuffer::create(m_gbufferSpecification, format("m_gbufferArray[%d]", i));
        m_gbufferArray[i]->resize(m_gbuffer->width(), m_gbuffer->height());
    }
    m_gbuffer = m_gbufferArray[0];

    setSubmitToDisplayMode(SubmitToDisplayMode::MAXIMIZE_THROUGHPUT);

    // Construct the eye cameras and head entity
    for (int eye = 0; eye < 2; ++eye) {
        static const String NAME[2] = {"VRApp::m_vrEyeCamera[0]", "VRApp::m_vrEyeCamera[1]"};
        if (isNull(m_vrEyeCamera[eye])) {
            m_vrEyeCamera[eye] = Camera::create(NAME[eye]);
            m_vrEyeCamera[eye]->setShouldBeSaved(false);
        }
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
    GApp::onInit();
    m_currentEyeIndex = 0;
    m_hmd->initRenderBuffers(window()->width(), window()->height());

    // Ask the Oculus SDK to let us submit work ahead of the framerate
    // to keep ovrHmd_SubmitFrame from blocking.
    ovr_SetBool(m_hmd->m_hmd, "QueueAheadEnabled", true);

    setSubmitToDisplayMode(SubmitToDisplayMode::MAXIMIZE_THROUGHPUT);
    setFrameDuration(1.0f / 75.0f);

    // Force the m_film to match the m_hmd's resolution instead of the OSWindow's
    resize(0, 0);

    m_cursorPointerTexture = Texture::fromFile(System::findDataFile("gui/cursor-pointer.png"), ImageFormat::RGBA8());
    sampleTrackingData();
}


void VRApp::sampleTrackingData() {
    BEGIN_PROFILER_EVENT("VRApp::sampleTrackingData");
    // Read data from OCulus
    m_viewOffset[0] = m_hmd->eyeRenderDesc[0].HmdToEyeViewOffset;
    m_viewOffset[1] = m_hmd->eyeRenderDesc[1].HmdToEyeViewOffset;
    m_hmdTrackingState = ovr_GetTrackingState(m_hmd->m_hmd, ovr_GetPredictedDisplayTime(m_hmd->m_hmd, 0), ovrTrue);

    ovr_CalcEyePoses(m_hmdTrackingState.HeadPose.ThePose, m_viewOffset, m_eyeRenderPose);

    // Position of the tracker relative to the body. Oculus has the camera looking along +Z, so we have to rotate 180 yaw in place
    const CFrame& bodySpaceTracker = toG3D(m_hmdTrackingState.CameraPose) * CFrame::fromXYZYPRDegrees(0, 0, 0, 180);

    // Player's head relative to the body (rest position in the real world)
    const CFrame& bodySpaceHead = toG3D(m_hmdTrackingState.HeadPose.ThePose);

    // Update the G3D VR eye cameras. This is not a pointer in case activeCamera() changes.
    const shared_ptr<Camera> bodyCamera = activeCamera();
    for (int eye = 0; eye < 2; ++eye) {
        ovrSizei hmdTextureResolution;
        hmdTextureResolution.w = m_hmd->eyeFramebufferQueue[eye]->width();
        hmdTextureResolution.h = m_hmd->eyeFramebufferQueue[eye]->height();

        const Vector2int16 framebufferResolution(m_framebuffer->vector2Bounds());
        debugAssert((framebufferResolution.x == hmdTextureResolution.w) && (framebufferResolution.y == hmdTextureResolution.h));

        // Construct a virtual eye camera and attach it to the G3D body camera using the tracking data
        // Load the Oculus-computed projection matrix for the eye
        const shared_ptr<Camera>& bodyCamera = activeCamera();

        const unsigned int projectionFlags = ovrProjection_RightHanded |ovrProjection_ClipRangeOpenGL;
        const ovrMatrix4f proj = ovrMatrix4f_Projection(m_hmd->hmdDesc.DefaultEyeFov[eye], -bodyCamera->nearPlaneZ(), -bodyCamera->farPlaneZ(), projectionFlags);

        if (eye == 0) {
            // Used for depth timewarp
            m_posTimewarpProjectionDesc = ovrTimewarpProjectionDesc_FromProjection(proj, projectionFlags);
        }

        Matrix4 projectionMatrix(proj.M[0]);
        const Projection projection(projectionMatrix);

        Matrix4 eyeMatrix;
        m_hmd->getEyeMatrix(m_eyeRenderPose[eye], eye, reinterpret_cast<float*>(&eyeMatrix));

        // The matrix we get back is the transpose of our Matrix4s
        eyeMatrix = eyeMatrix.transpose();

        // The eye matrix is the world-to-camera and we need camera-to-world
        m_previousEyeFrame[eye] = m_eyeFrame[eye];
        m_eyeFrame[eye] = CFrame(eyeMatrix.upper3x3(), eyeMatrix.column(3).xyz()).inverse();

        if (m_previousEyeFrame[eye].translation.isNaN()) {
            // First frame of animation--override with no transform
            m_previousEyeFrame[eye] = m_eyeFrame[eye];
        }

        // The camera is relative to the bodyCamera, which is probably in the center of the avatar's chest. 
        // If you wish to know the player's eye height when standing, look at:
        const float playerEyeHeight = -ovr_GetFloat(m_hmd->m_hmd, OVR_KEY_EYE_HEIGHT, OVR_DEFAULT_EYE_HEIGHT);

        m_vrEyeCamera[eye]->copyParametersFrom(bodyCamera);
        if (m_vrSettings.overrideMotionBlur) {
            m_vrEyeCamera[eye]->motionBlurSettings() = m_vrSettings.motionBlurSettings;
        }
        if (m_vrSettings.overrideDepthOfField) {
            m_vrEyeCamera[eye]->depthOfFieldSettings() = m_vrSettings.depthOfFieldSettings;
        }
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

    m_externalCameraFrame = m_vrHead->frame() * bodySpaceHead.inverse() * bodySpaceTracker;

    END_PROFILER_EVENT();
}


void VRApp::resize(int w, int h) {
    // Size the framebuffer to the m_hmd texture resolution
    // Ignore the resolution of the physical window.
    GApp::resize(m_hmd->eyeFramebufferQueue[0]->width(), m_hmd->eyeFramebufferQueue[0]->height());
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
        if (! m_hudEnabled || (m_vrSettings.debugMirrorMode != DebugMirrorMode::POST_DISTORTION)) {
            rd->push2D(); {
                onGraphics2D(rd, posed2D);
            } rd->pop2D();
        }

        if ((submitToDisplayMode() == SubmitToDisplayMode::MAXIMIZE_THROUGHPUT) && (!renderDevice->swapBuffersAutomatically())) {
            GApp::swapBuffers();
        }
        rd->clear();

        if (m_vrSubmitToDisplayMode == SubmitToDisplayMode::MAXIMIZE_THROUGHPUT) {
            // Submit the PREVIOUS frame
            submitHMDFrame(rd);
        }

        // Begin the NEW frame
        sampleTrackingData();

        BEGIN_PROFILER_EVENT("Rendering");

        // No reference because we're going to mutate the active camera
        const shared_ptr<Camera> bodyCamera = activeCamera();
        for (m_currentEyeIndex = 0; m_currentEyeIndex < 2; ++m_currentEyeIndex) {
            const int eye = m_currentEyeIndex;

            // Increment to use next texture, just before writing
            m_hmd->eyeFramebufferQueue[eye]->advance();

            // Switch to eye render target
            const shared_ptr<Framebuffer>& vrFB = m_hmd->eyeFramebufferQueue[eye]->currentFramebuffer();
            rd->pushState(vrFB); {
                // Make a G3D wrapper for the raw OpenGL eye texture so that we can pass it to G3D routines
                m_currentEyeTexture = vrFB->texture(0);

                setActiveCamera(m_vrEyeCamera[eye]);

                m_gbuffer = m_gbufferArray[m_currentEyeIndex];
                //rd->setProjectionAndCameraMatrix(eyeCamera->projection(), eyeCamera->frame());
                onGraphics3D(rd, posed3D);
            } rd->popState();
        }
        setActiveCamera(bodyCamera);

        END_PROFILER_EVENT();

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

        if (m_vrSettings.debugMirrorMode == DebugMirrorMode::PRE_DISTORTION) {
            // Access the hardware frame buffer
            rd->push2D(shared_ptr<Framebuffer>()); {
                rd->setColorClearValue(Color3::black());
                rd->clear();
                for (int eye = 0; eye < 2; ++eye) {
                    const shared_ptr<Texture>& finalImage = m_hmd->eyeFramebufferQueue[eye]->currentColorTexture();

                    // Find the scale needed to fit both images on screen
                    const float scale = min(float(rd->width()) * 0.5f / float(finalImage->width()), float(rd->height()) / float(finalImage->height()));
                    const int xShiftDirection = 2 * eye - 1;

                    const float width = finalImage->width()  * scale;
                    const float height = finalImage->height() * scale;

                    const Rect2D& rect = Rect2D::xywh((rd->width() + width * float(xShiftDirection - 1)) * 0.5f, 0.0f, width, height);
                    Draw::rect2D(rect, rd, Color3::white(), finalImage);
                }
            } rd->pop2D();
        }

        if (m_vrSubmitToDisplayMode == SubmitToDisplayMode::MINIMIZE_LATENCY) {
            // Submit the CURRENT frame
            submitHMDFrame(rd);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);

        // Tell G3D that we unmapped the framebuffer
        rd->setFramebuffer(shared_ptr<Framebuffer>());

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

#   if 0 // Disabled because this appears to sometimes flip the vertical axis
    } else if (m_highQualityWarping) {
        m_highQualityWarping = false;
        debugPrintf("VRApp::decreaseEffects() Disabled high-quality HMD warping to increase performance.\n");
#   endif

    } else if (activeCamera()->filmSettings().antialiasingEnabled()) {
        // Disable FXAA
        activeCamera()->filmSettings().setAntialiasingEnabled(false);
        debugPrintf("VRApp::decreaseEffects() Disabled antialiasing to increase performance.\n");
    }
}


void VRApp::submitHMDFrame(RenderDevice* rd) {
    // Do distortion rendering, Present and flush/sync
    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    // Tell G3D that we disabled the framebuffer
    rd->setFramebuffer(shared_ptr<Framebuffer>());

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
        m_hmd->debugMirrorFramebuffer->blitTo(rd, shared_ptr<Framebuffer>(), true);
    }
}


void VRApp::onCleanup() {
    // Called after the application loop ends.  Place a majority of cleanup code
    // here instead of in the constructor so that exceptions can be caught.
    m_hmd->cleanup();
    delete m_hmd;
    m_hmd = NULL;
}


void VRApp::onAfterLoadScene(const Any& any, const String& sceneName) {
    GApp::onAfterLoadScene(any, sceneName);

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
    if (GApp::onEvent(event)) { return true; }

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
        const Point2& size = m_hmd->hudFramebufferQueue->currentFramebuffer()->vector2Bounds() - Vector2::one(); 
        if ((p.x < 0) || (p.y < 0) || (p.x > size.x) || (p.y > size.y)) {
            window()->setRelativeMousePosition(p.clamp(Vector2::zero(), size));
        }
        return false;
    }

    return false;
}


} // namespace