/**
 @file OSWindow.cpp
  
 @maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
 @created 2004-11-16
 @edited  2016-03-23
 */

#include "GLG3D/OSWindow.h"
#include "GLG3D/GApp.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/RenderDevice.h"
#include "G3D/ImageFormat.h"
#include "GLG3D/Framebuffer.h"


#ifdef G3D_WINDOWS
#   include "GLG3D/Win32Window.h"
#endif

#include "GLG3D/GLFWWindow.h"


//#define WINDOWS_CLASS Win32Window
#define WINDOWS_CLASS GLFWWindow


namespace G3D {

OSWindow::Settings::Settings() :
    width(960),
    height(600),
    x(0),
    y(0),
    center(true),
    rgbBits(8),
    alphaBits(0),
    depthBits(24),
    stencilBits(8),
    msaaSamples(1),
    hardware(true),
    fullScreen(false),
    asynchronous(true),
    stereo(false),
    refreshRate(85),
    resizable(false),
    sharedContext(false),
    allowMaximize(true),
    framed(true),
    visible(true),
    debugContext(false),
    alwaysOnTop(false),
    allowAlphaTest(false),
#ifdef G3D_OSX
    majorGLVersion(3),
    minorGLVersion(2), // This works around an OSX bug...it actually produces a 3.3 context
    forwardCompatibilityMode(true),
    coreContext(true), // Required on OS X for 3.3
#else
    majorGLVersion(1), // This is the "latest" context on Windows
    minorGLVersion(0),
    forwardCompatibilityMode(false),
    coreContext(false),
#endif
    caption("3D") {

#   ifdef G3D_OSX
        defaultIconFilename = System::findDataFile("G3D-128.png", false);
        alwaysAssertM(! allowAlphaTest, "allowAlphaTest not allowed on OS X");
#   else
        defaultIconFilename = System::findDataFile("G3D-64.png", false);
#   endif
}

const shared_ptr<Framebuffer>& OSWindow::framebuffer() const {
    if (isNull(m_framebuffer)) {
        m_framebuffer = shared_ptr<Framebuffer>(new Framebuffer("OpenGL Hardware Framebuffer", GL_ZERO));
        m_framebuffer->m_window = const_cast<OSWindow*>(this);
    }

    return m_framebuffer;
}


// See http://msdn.microsoft.com/en-us/library/ms724385(VS.85).aspx
Vector2 OSWindow::primaryDisplaySize() {
#   ifdef G3D_WINDOWS
        return WINDOWS_CLASS::primaryDisplaySize();
#   else
        return GLFWWindow::primaryDisplaySize();
#   endif
}

Vector2 OSWindow::virtualDisplaySize() {
#   ifdef G3D_WINDOWS
        return WINDOWS_CLASS::virtualDisplaySize();
#   else
	return GLFWWindow::virtualDisplaySize();
#   endif
}

Vector2int32 OSWindow::primaryDisplayWindowSize() {
#   ifdef G3D_WINDOWS
        return WINDOWS_CLASS::primaryDisplayWindowSize();
#   else
        return GLFWWindow::primaryDisplayWindowSize();
#   endif
}

int OSWindow::numDisplays() {
#   ifdef G3D_WINDOWS
        return WINDOWS_CLASS::numDisplays();
#   else
        return GLFWWindow::numDisplays();
#   endif
}

OSWindow* OSWindow::create(const OSWindow::Settings& s) {
#   ifdef G3D_WINDOWS
        return WINDOWS_CLASS::create(s);
#   else
	return GLFWWindow::create(s);
#   endif
}

const OSWindow* OSWindow::m_current = NULL;

void OSWindow::handleResize(int width, int height) {
    if (m_settings.width != width || m_settings.height != height) {
        // update settings
        m_settings.width = width;
        m_settings.height = height;

        // update viewport
        Rect2D newViewport = Rect2D::xywh(0.0f, 0.0f, (float)width, (float)height);
        m_renderDevice->setViewport(newViewport);

        // force swap buffers
        m_renderDevice->swapBuffers();
    }
}

void OSWindow::fireEvent(const GEvent& event) {
    m_eventQueue.pushBack(event);
}


void OSWindow::getOSEvents(Queue<GEvent>& events) {
    // no events added
    (void)events;
}


bool OSWindow::pollEvent(GEvent& e) {

    // Extract all pending events and put them on the queue.
    getOSEvents(m_eventQueue);

    // Return the first pending event
    if (m_eventQueue.size() > 0) {
        e = m_eventQueue.popFront();
        return true;
    } else {
        return false;
    }
}

void OSWindow::executeLoopBody() {
    if (notDone()) {
        if (m_loopBodyStack.last().isGApp) {
            m_loopBodyStack.last().app->oneFrame();
        } else {                
            m_loopBodyStack.last().func(m_loopBodyStack.last().arg);
        }
    }
}


void OSWindow::pushLoopBody(GApp* app) {
    m_loopBodyStack.push(LoopBody(app));
    app->beginRun();
}


void OSWindow::popLoopBody() {
    if (m_loopBodyStack.size() > 0) {
        if (m_loopBodyStack.last().isGApp) {
            m_loopBodyStack.last().app->endRun();
        }
        m_loopBodyStack.pop();
    }
}


const ImageFormat* OSWindow::Settings::colorFormat() const {
    switch (rgbBits) {
    case 5:
        if (alphaBits == 0) {
            return ImageFormat::RGB5();
        } else {
            return ImageFormat::RGB5A1();
        }

    case 8:
        if (alphaBits > 0) {
            return ImageFormat::RGBA8();
        } else {
            return ImageFormat::RGB8();
        }

    case 10:
        if (alphaBits > 0) {
            return ImageFormat::RGB10A2();
        } else {
            return ImageFormat::RGB10();
        }

    case 16:
        if (alphaBits > 0) {
            return ImageFormat::RGBA16();
        } else {
            return ImageFormat::RGB16();
        }
    default:;
    }

    return ImageFormat::RGB8();
}


String OSWindow::clipboardText() {
    if (notNull(current())) {
        return current()->_clipboardText();
    } else {
        return "";
    }
}


void OSWindow::setClipboardText(const String& text) {
    if (notNull(current())) {
        current()->_setClipboardText(text);
    }
}

void OSWindow::getFullScreenResolutions(Array<Vector2int32>& array) {
#   if defined(G3D_WINDOWS)
        //GLFWWindow::getFullScreenResolutions(array);
        array.fastClear();
#   else
        array.fastClear();
#   endif
}


void OSWindow::getGameControllerState(unsigned int stickNum, Array<float>& axis, Array<bool>& button) const {
    // Different operating system drivers map the Xbox360 controller differently
#   ifdef G3D_WINDOWS
        enum { RIGHT_X_AXIS = 4, RIGHT_Y_AXIS = 3, LEFT_X_AXIS = 0, LEFT_Y_AXIS = 1, TRIGGER_X_AXIS = 2, TRIGGER_Y_AXIS = 2, NUM_AXES = 5};
        static const int buttonRemap[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, -1};
#   else
        // Currently set based on OSX driver, which has inverted axes
        enum { RIGHT_X_AXIS = 2, RIGHT_Y_AXIS = 3, LEFT_X_AXIS = 0, LEFT_Y_AXIS = 1, TRIGGER_X_AXIS = 4, TRIGGER_Y_AXIS = 5, NUM_AXES = 6};
        static const int buttonRemap[] = {11, 12, 13, 14, 8, 9, 5, 4, 6, 7, 0, 3, 1, 2, 10};
#   endif

    static Array<float> originalAxis;
    static Array<bool>  originalButton;

    originalAxis.fastClear();
    originalButton.fastClear();
    getJoystickState(stickNum, originalAxis, originalButton);

    axis.resize(6);
    button.resize(14);

    axis[0] = originalAxis[LEFT_X_AXIS];
    axis[1] = originalAxis[LEFT_Y_AXIS];

    axis[2] = originalAxis[RIGHT_X_AXIS];
    axis[3] = originalAxis[RIGHT_Y_AXIS];

    axis[4] = originalAxis[TRIGGER_X_AXIS];
    axis[5] = originalAxis[TRIGGER_Y_AXIS];

    for (int b = 0; b < 14; ++b) {
        const int index = buttonRemap[b];
        button[b] = (index >= 0) ? originalButton[index] : false;
    }
}

}
