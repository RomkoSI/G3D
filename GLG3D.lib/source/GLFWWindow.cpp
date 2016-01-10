#include "G3D/platform.h"
#include "GLG3D/GLCaps.h"

extern "C" {

#   include "../../glfw.lib/include/GLFW/glfw3.h"
#   if defined(G3D_WINDOWS)
    #   define GLFW_EXPOSE_NATIVE_WIN32
    #   define GLFW_EXPOSE_NATIVE_WGL
    #   include "../../glfw.lib/include/GLFW/glfw3native.h"
#   endif
}

#   if defined(G3D_OSX)
#       include "GLG3D/CocoaInterface.h"
#   endif

#include "GLG3D/GLFWWindow.h"
#include "G3D/Vector2.h"
#include "G3D/Rect2D.h"
#include "G3D/CPUPixelTransferBuffer.h"
#include "G3D/ImageConvert.h"
#include "G3D/FileSystem.h"
#include "GLG3D/glcalls.h"

namespace G3D {

static GLFWWindow* s_currentWindow;

void GLFWWindow::setCurrentWindowPtr(GLFWWindow* window){
  s_currentWindow = window;
}


GLFWWindow* GLFWWindow::getCurrentWindowPtr(){
  return s_currentWindow;
}


static void errorCallback(int code, const char* error) {
  debugPrintf("GLFW Error: %s\n", error);
}


static void windowCloseCallback( GLFWwindow* w ){
    GLFWWindow* window = GLFWWindow::getCurrentWindowPtr();
    GEvent e;
    e.type = GEventType::QUIT;
    window->fireEvent(e);
}

static void windowRefreshCallback( GLFWwindow* w ){
  //fprintf(stderr, "GLFWWindow Just received a Window Refresh Callback. What do?\n");
}

static void windowIconifyCallback(GLFWwindow* w, int code){
    GLFWWindow* window = GLFWWindow::getCurrentWindowPtr();
    // toggle whether the window is iconified, as this method is called both on minimizing and un-minimizing windows
    window->setIconified(!window->isIconified());
}

static void cursorEnterCallback( GLFWwindow* w, int x){
    GLFWWindow* window = GLFWWindow::getCurrentWindowPtr();
    
    window->handleCursorEnter(x);
}


static void filesDroppedCallback(GLFWwindow* w, int fileCount, const char** fileList) {
    GLFWWindow* window = GLFWWindow::getCurrentWindowPtr();
    window->clearDroppedFileList();
    for(int i = 0; i < fileCount; ++i) { 
        window->appendFileToFileList(fileList[i]);
    }
    GEvent e;
    e.type = GEventType::FILE_DROP;
    
    window->getMousePosition(e.drop.x, e.drop.y);
    
    window->fireEvent(e);
}


static void windowSizeCallback(GLFWwindow* w, int width, int height) {
  GLFWWindow* window = GLFWWindow::getCurrentWindowPtr();

  GEvent e;
  e.type = GEventType::VIDEO_RESIZE;
  e.resize.w = width;
  e.resize.h = height;
  window->fireEvent(e);
  window->handleResizeFromCallback(width, height);
  //debugPrintf("Window Size Callback, value: (%d,%d)\n", width, height);
}


static void windowFocusCallback(GLFWwindow* w, int code){
  GLFWWindow* window = GLFWWindow::getCurrentWindowPtr();

  GEvent e;
  e.type = GEventType::FOCUS;
  e.focus.hasFocus = (code != 0); // TODO: find out if this is right
  window->fireEvent(e);
}


static void cursorPosCallback( GLFWwindow* w, double x, double y){
  GLFWWindow* window = GLFWWindow::getCurrentWindowPtr();

  if (window->cursorActive()) {
      GEvent e;
      e.motion.type  = GEventType::MOUSE_MOTION;
      e.motion.which = 0; //Always 0, since we have no way of distinguishing mice
      uint8 mouseButtons;
      window->getMouseButtonState(mouseButtons);
      e.motion.state = mouseButtons;
      e.motion.x = (uint16) x;
      e.motion.y = (uint16) y;
      static uint16 oldX = e.motion.x, oldY = e.motion.y;
      e.motion.xrel = e.motion.x - oldX;
      e.motion.yrel = e.motion.y - oldY;
      oldX = e.motion.x;
      oldY = e.motion.y;
      window->fireEvent(e);
  }
  //  debugPrintf("Cursor Pos Callback, value: (%d,%d)\n", x, y);
}


static void mouseButtonCallback(GLFWwindow* w, int button, int action, int modBits) {
  GLFWWindow* window = GLFWWindow::getCurrentWindowPtr();
  
  GEvent e;
  e.button.which = 0; // No way to choose other index
  int x,y;
  window->getMousePosition(x,y);
  e.button.x = x;
  e.button.y = y;

  // GLFW maps middle button to 2 and right to 1, G3D uses the reversed convention
  switch (button) {
  case 1:
      e.button.button = 2;
      break;

  case 2:
      e.button.button = 1;
      break;

  default:
      e.button.button = button;
      break;
  }

  e.button.controlKeyIsDown = ((modBits & GLFW_MOD_CONTROL) != 0);

  if (action == GLFW_RELEASE) {
      e.type = GEventType::MOUSE_BUTTON_UP;
      e.button.state = GButtonState::RELEASED;
  } else if (action == GLFW_PRESS){
      e.type = GEventType::MOUSE_BUTTON_DOWN;
      e.button.state = GButtonState::PRESSED;
  } else {
      alwaysAssertM(false, format("GLFUnknown Mouse action (%d) taken with button %d", action, button));
  }

  window->fireEvent(e);
  if (e.type == GEventType::MOUSE_BUTTON_UP) {
      e.type = GEventType::MOUSE_BUTTON_CLICK;
      e.button.numClicks = 1;
      window->fireEvent(e);
  }
}


static void charCallback(GLFWwindow* w, unsigned int unicode) {
  GLFWWindow* window = GLFWWindow::getCurrentWindowPtr();
  //A non-control character key press (TODO: Find out why
  // GLFW creates character events for arrow keys (in the 60k range of unicode values
  if (unicode > 31 && unicode < 127) {
    GEvent charInputEvent;
    charInputEvent.type = GEventType::CHAR_INPUT;
    charInputEvent.character.unicode = unicode;
    window->fireEvent(charInputEvent);
    //    debugPrintf("Char callback %d\n", unicode);
  }
}

/*! @brief The function signature for keyboard key callbacks.
 *
 *  This is the function signature for keyboard key callback functions.
 *
 *  @param[in] window The window that received the event.
 *  @param[in] key The [keyboard key](@ref keys) that was pressed or released.
 *  @param[in] scancode The system-specific scancode of the key.
 *  @param[in] action @ref GLFW_PRESS, @ref GLFW_RELEASE or @ref GLFW_REPEAT.
 *  @param[in] mods Bit field describing which [modifier keys](@ref mods) were
 *  held down.
 *
 *  @sa glfwSetKeyCallback
 *
 *  @ingroup input
 */
static void keyCallback( GLFWwindow* w, int key, int scancode, int action, int modBits) {
  GLFWWindow* window = GLFWWindow::getCurrentWindowPtr();

  GEvent e;
  e.key.which = 0; //All keyboard events map to 0 currently
  if(action == GLFW_RELEASE){
    e.key.type  = GEventType::KEY_UP;
    e.key.state = GButtonState::RELEASED;
  } else if(action == GLFW_PRESS) {
    e.key.type  = GEventType::KEY_DOWN;
    e.key.state = GButtonState::PRESSED;    
  } else if (action == GLFW_REPEAT) {
    e.key.type  = GEventType::KEY_REPEAT;
    e.key.state = GButtonState::PRESSED;    
  } else {
    alwaysAssertM(false, format("Unknown key event: %d with keycode %d", action, key));
  }
  
  switch(key){
  case GLFW_KEY_ESCAPE:        e.key.keysym.sym = GKey::ESCAPE;     break;
  case GLFW_KEY_ENTER:         e.key.keysym.sym = GKey::RETURN;     break;
  case GLFW_KEY_TAB:           e.key.keysym.sym = GKey::TAB;        break;
  case GLFW_KEY_BACKSPACE:     e.key.keysym.sym = GKey::BACKSPACE;  break;
  case GLFW_KEY_INSERT:        e.key.keysym.sym = GKey::INSERT;     break;    
  case GLFW_KEY_DELETE:        e.key.keysym.sym = GKey::DELETE;     break;
  case GLFW_KEY_RIGHT:         e.key.keysym.sym = GKey::RIGHT;      break;
  case GLFW_KEY_LEFT:          e.key.keysym.sym = GKey::LEFT;       break;
  case GLFW_KEY_DOWN:          e.key.keysym.sym = GKey::DOWN;       break;
  case GLFW_KEY_UP:            e.key.keysym.sym = GKey::UP;         break;
  case GLFW_KEY_PAGE_UP:       e.key.keysym.sym = GKey::PAGEUP;     break;
  case GLFW_KEY_PAGE_DOWN:     e.key.keysym.sym = GKey::PAGEDOWN;   break;
  case GLFW_KEY_HOME:          e.key.keysym.sym = GKey::HOME;       break;
  case GLFW_KEY_END:           e.key.keysym.sym = GKey::END;        break;
  case GLFW_KEY_CAPS_LOCK:     e.key.keysym.sym = GKey::CAPSLOCK;   break;
  case GLFW_KEY_SCROLL_LOCK:   e.key.keysym.sym = GKey::SCROLLOCK;  break;
  case GLFW_KEY_NUM_LOCK:      e.key.keysym.sym = GKey::NUMLOCK;    break;
  case GLFW_KEY_PRINT_SCREEN:  e.key.keysym.sym = GKey::PRINT;      break;
  case GLFW_KEY_PAUSE:         e.key.keysym.sym = GKey::PAUSE;      break;
  case GLFW_KEY_LEFT_SHIFT:    
    e.key.keysym.sym = GKey::LSHIFT;
    window->modifyCurrentKeyMod(GKeyMod::LSHIFT, GButtonState(e.key.state));
    break;
  case GLFW_KEY_LEFT_CONTROL:  
    e.key.keysym.sym = GKey::LCTRL;      
    window->modifyCurrentKeyMod(GKeyMod::LCTRL, GButtonState(e.key.state));
    break;
  case GLFW_KEY_LEFT_ALT:      
    e.key.keysym.sym = GKey::LALT;       
    window->modifyCurrentKeyMod(GKeyMod::LALT, GButtonState(e.key.state));
    break;
  case GLFW_KEY_RIGHT_SHIFT:   
    e.key.keysym.sym = GKey::RSHIFT;     
    window->modifyCurrentKeyMod(GKeyMod::RSHIFT, GButtonState(e.key.state));
    break;
  case GLFW_KEY_RIGHT_CONTROL: 
    e.key.keysym.sym = GKey::RCTRL;      
    window->modifyCurrentKeyMod(GKeyMod::RCTRL, GButtonState(e.key.state));
    break;
  case GLFW_KEY_RIGHT_ALT:     
    e.key.keysym.sym = GKey::RALT;       
    window->modifyCurrentKeyMod(GKeyMod::RALT, GButtonState(e.key.state));
    break;
  case GLFW_KEY_LEFT_SUPER:    e.key.keysym.sym = GKey::LSUPER;     break;
  case GLFW_KEY_RIGHT_SUPER:   e.key.keysym.sym = GKey::RSUPER;     break;
  case GLFW_KEY_MENU:          e.key.keysym.sym = GKey::MENU;       break;
  default:
    if( key >= 'A' && key <= 'Z' ){
      // GLFW uses uppercase as the canonical code, 
      // G3D uses lower case, so we must compensate
      e.key.keysym.sym = (GKey::Value)(key + ('a' - 'A'));
    } else if( key == GLFW_KEY_SPACE      ||
	       key == GLFW_KEY_APOSTROPHE ||
	       (key >= GLFW_KEY_COMMA && key <= GLFW_KEY_SLASH) ||
	       (key >= '0' && key <= '9') ||
	       key == GLFW_KEY_SEMICOLON  ||
	       key == GLFW_KEY_EQUAL      ||
	       (key >= GLFW_KEY_LEFT_BRACKET && key <= GLFW_KEY_RIGHT_BRACKET) ||
	       key == GLFW_KEY_GRAVE_ACCENT ||
	       key == GLFW_KEY_WORLD_1      ||
	       key == GLFW_KEY_WORLD_2) {
      // Maps directly to G3D keys
      e.key.keysym.sym = (GKey::Value)key;
    } else if ( key >= GLFW_KEY_F1 && key <= GLFW_KEY_F15) {
      e.key.keysym.sym = (GKey::Value)(key - GLFW_KEY_F1 + GKey::F1);
    } else if ( key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_EQUAL) {
      e.key.keysym.sym = (GKey::Value)(key - GLFW_KEY_KP_0 + GKey::KP0);
    } else if ( key == GLFW_KEY_UNKNOWN ) {
      // The key was platform specific. It has a scancode we can use to try to
      // identify it. Currently the only known key press that reaches here is
      // the "fn" key on OS X.
      return;
    } else {
      alwaysAssertM(false, format("Unknown Key with code %d performed action %d.\n", key, action));
    }
  }

  //TODO: Properly set e.key.keysym.unicode
  e.key.keysym.unicode = key;

  e.key.keysym.scancode = scancode;

  // TODO: Use modBits directly?
  e.key.keysym.mod = window->getCurrentKeyMod();
  window->fireEvent(e);
  //debugPrintf("Key Callback (Key %d, Action %d)\n", key, action);
}


static void scrollCallback( GLFWwindow* w, double dx, double dy){
  GLFWWindow* window = GLFWWindow::getCurrentWindowPtr();

  GEvent e;
  e.scroll2d.which = 0;
  e.scroll2d.type  = GEventType::MOUSE_SCROLL_2D;
  e.scroll2d.dx    = (int16) dx;
  e.scroll2d.dy    = (int16) dy;
  window->fireEvent(e);
  //debugPrintf("Scroll Callback (%f,%f)\n", (float)dx, (float)dy);
}


static void monitorCallback(GLFWmonitor* monitor, int event) {
// TODO:
}

void GLFWWindow::handleCursorEnter(int action){
    //GLFW emits GL_TRUE on cursor enter, and GL_FALSE on exit
    m_cursorInside = (action == GL_TRUE);
}


void GLFWWindow::modifyCurrentKeyMod(GKeyMod::Value button, GButtonState state) {
  if(state == GButtonState::PRESSED){
    m_currentKeyMod = (GKeyMod::Value)((uint8)button | (uint8)m_currentKeyMod);
  } else if(state == GButtonState::RELEASED){
    m_currentKeyMod = (GKeyMod::Value)((~(uint8)button) & (uint8)m_currentKeyMod);
  }
}


void GLFWWindow::initializeCurrentKeyMod() {
  static const int modKeys[6] = {GLFW_KEY_RIGHT_SHIFT, GLFW_KEY_RIGHT_CONTROL, GLFW_KEY_RIGHT_ALT,
			   GLFW_KEY_LEFT_SHIFT, GLFW_KEY_LEFT_CONTROL,  GLFW_KEY_LEFT_ALT};
  static const GKeyMod::Value gKeyModEquivalent[6] = {GKeyMod::RSHIFT, GKeyMod::RCTRL, GKeyMod::RALT,
					       GKeyMod::LSHIFT, GKeyMod::LCTRL, GKeyMod::LALT};
  for (int i = 0; i < 6; ++i) {
    int result = glfwGetKey(m_glfwWindow, modKeys[i]);
    if (result == GLFW_PRESS) {
      modifyCurrentKeyMod(gKeyModEquivalent[i], GButtonState::RELEASED);
    }
  }
}


void GLFWWindow::setEventCallbacks() {
    glfwSetErrorCallback(errorCallback);

    glfwSetKeyCallback(m_glfwWindow, keyCallback);
    glfwSetCharCallback(m_glfwWindow, charCallback);

    glfwSetMonitorCallback(monitorCallback);

    glfwSetMouseButtonCallback(m_glfwWindow, mouseButtonCallback);
    glfwSetCursorPosCallback(m_glfwWindow, cursorPosCallback);
    glfwSetCursorEnterCallback(m_glfwWindow, cursorEnterCallback);
    glfwSetScrollCallback(m_glfwWindow, scrollCallback);

    glfwSetWindowSizeCallback(m_glfwWindow, windowSizeCallback);
    glfwSetWindowCloseCallback(m_glfwWindow, windowCloseCallback);
    glfwSetWindowRefreshCallback(m_glfwWindow, windowRefreshCallback);
    glfwSetWindowFocusCallback(m_glfwWindow, windowFocusCallback);
    glfwSetWindowIconifyCallback(m_glfwWindow, windowIconifyCallback);

    glfwSetDropCallback(m_glfwWindow, filesDroppedCallback);
    //debugPrintf("Set Event Callbacks\n");
}



#if defined(G3D_WINDOWS)
// See http://msdn.microsoft.com/en-us/library/ms724385(VS.85).aspx
Vector2 GLFWWindow::primaryDisplaySize() {
    return Vector2((float)GetSystemMetrics(SM_CXSCREEN), (float)GetSystemMetrics(SM_CYSCREEN));
}


Vector2 GLFWWindow::virtualDisplaySize() {
    return Vector2((float)GetSystemMetrics(SM_CXVIRTUALSCREEN), (float)GetSystemMetrics(SM_CYVIRTUALSCREEN));
}


Vector2int32 GLFWWindow::primaryDisplayWindowSize() {
    RECT workarea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workarea, 0); // total usable area, excluding taskbar

    const Vector2int32 decorationSize = getDecoration(workarea.right, workarea.bottom);
    const int32 x = workarea.right - decorationSize.x * 4; //acount for window border
    const int32 y = workarea.bottom - (decorationSize.y + decorationSize.x * 2);

    return Vector2int32(x, y);
}

#else

Vector2 GLFWWindow::primaryDisplaySize() {
    // This returns the actual pixel count of the display on Windows,
    // which is not always useful on Hi-DPI displays.  So we only use
    // it on non-windows builds.
    if (! glfwInit()) {
        alwaysAssertM(false, "Failed to initialize GLFW\n");
    }
    GLFWmonitor* primaryDisplay = glfwGetPrimaryMonitor();
    debugAssert(notNull(primaryDisplay));
    const GLFWvidmode* vidMode = glfwGetVideoMode(primaryDisplay);
    debugAssert(notNull(vidMode));
    return Vector2(vidMode->width, vidMode->height);
}

Vector2 GLFWWindow::virtualDisplaySize() {
    // Find the size of the virtual display size by finding the minimum and maximum coordinates
    // of the actual displays on the virtual display canvas.
    if (! glfwInit()) {
        alwaysAssertM(false, "Failed to initialize GLFW\n");
    }
    int monitorCount;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
    Vector2 maxDisplayCoordinate = Vector2(0.0f,0.0f);
    Vector2 minDisplayCoordinate = Vector2(0.0f,0.0f);
    for (int i = 0; i < monitorCount; ++i) {
        const GLFWvidmode* vidMode = glfwGetVideoMode(monitors[i]);
        Vector2int32 monitorPos;
        glfwGetMonitorPos(monitors[i], &monitorPos.x, &monitorPos.y);
        maxDisplayCoordinate.x = max(maxDisplayCoordinate.x, (float)monitorPos.x + vidMode->width);
        maxDisplayCoordinate.y = max(maxDisplayCoordinate.y, (float)monitorPos.y + vidMode->height);
        minDisplayCoordinate.x = min(minDisplayCoordinate.x, (float)monitorPos.x);
        minDisplayCoordinate.y = min(minDisplayCoordinate.y, (float)monitorPos.y);
    }
    
    return maxDisplayCoordinate - minDisplayCoordinate;
}

Vector2int32 GLFWWindow::primaryDisplayWindowSize() {
    Vector2 v = primaryDisplaySize();
    return Vector2int32((int32)v.x, (int32)v.y);
}

#endif

int GLFWWindow::numDisplays() {
    int count;
    glfwGetMonitors(&count);
    return count;
}


void GLFWWindow::updateSettings() {
}


GLFWWindow* GLFWWindow::create(const OSWindow::Settings& settings){
    GLFWWindow* window = new GLFWWindow(settings);
    return window;
}

static String toString(const GLFWvidmode& mode) {
    return format("VidMode: %d Hz, (%dx%d), Bit depths (%d, %d, %d)", mode.refreshRate, mode.width, mode.height, mode.redBits, mode.greenBits, mode.blueBits);
}

static GLFWvidmode findClosestVidMode(const GLFWvidmode* modes, int count, const OSWindow::Settings& settings) {
    Array<GLFWvidmode> vidModes;
    vidModes.resize(count);
    for (int i = 0; i < count; ++i) {
        vidModes[i] = modes[i];
    }

    // TODO: Find the closest width, remove all that do not fit
    int targetWidth = settings.width;
    int minWidthDistance = 100000;
    for (auto mode : vidModes) {
        minWidthDistance = min(minWidthDistance, iAbs(mode.width - targetWidth));
    }

    for (int i = vidModes.size() - 1; i >= 0; --i) {
        if (minWidthDistance < iAbs(vidModes[i].width - targetWidth)) {
            vidModes.remove(i);
        }
    }

    // TODO: Find the closest height, remove all that do not fit
    int targetHeight = settings.height;
    int minHeightDistance = 100000;
    for (auto mode : vidModes) {
        minHeightDistance = min(minHeightDistance, iAbs(mode.height - targetHeight));
    }

    for (int i = vidModes.size() - 1; i >= 0; --i) {
        if (minHeightDistance < iAbs(vidModes[i].height - targetHeight)) {
            vidModes.remove(i);
        }
    }

    // Find the closest refresh rate, remove all that do not fit
    int targetRefreshRate = settings.refreshRate;
    int minRefreshRateDist = 0;
    if (settings.refreshRate == 0) {
        for (auto mode : vidModes) {
            targetRefreshRate = max(targetRefreshRate, mode.refreshRate);
        }
    }
    else {
        minRefreshRateDist = 1000;
        for (auto mode : vidModes) {
            minRefreshRateDist = min(minRefreshRateDist, iAbs(mode.refreshRate - targetRefreshRate));
        }
    }
    for (int i = vidModes.size() - 1; i >= 0; --i) {
        if (minRefreshRateDist < iAbs(vidModes[i].refreshRate - targetRefreshRate)) {
            vidModes.remove(i);
        }
    }


    // TODO: Find the closest bit depth, remove all that do not fit


    // Return the highest bit depth, highest resolution vid mode that fits the criteria
    return vidModes.last();

}


GLFWWindow::GLFWWindow(const OSWindow::Settings& settings) : m_currentKeyMod(GKeyMod::NONE){
    if (! glfwInit()) {
        alwaysAssertM(false, "Failed to initialize GLFW\n");
    }

    setCurrentWindowPtr(this);
    m_mouseVisible = true;
    m_inputCapture = false;
    m_iconified    = false;
    m_settings = settings;
    GLFWwindow* share = NULL;


    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, settings.majorGLVersion);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, settings.minorGLVersion);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, settings.forwardCompatibilityMode ? GL_TRUE : GL_FALSE);
    if (settings.coreContext) {
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }
    int windowX, windowY;
    if (settings.center) {
        int w = settings.width;
        int h = settings.height;
        const Vector2int32& displaySize = primaryDisplayWindowSize();
        windowX = (displaySize.x - w) / 2;
	    windowY = (displaySize.y - h) / 2;
    } else {
        windowX = settings.x;
        windowY = settings.y;
    }
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, settings.debugContext ? GL_TRUE : GL_FALSE);

    glfwWindowHint(GLFW_RESIZABLE,  settings.resizable);
    //TODO:    glfwWindowHint(GLFW_REFRESH_RATE, settings.refreshRate);
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE); // Make invisible, move, then show

    glfwWindowHint(GLFW_DECORATED, settings.framed ? GL_TRUE : GL_FALSE);

    glfwWindowHint(GLFW_AUTO_ICONIFY, GL_TRUE);

    glfwWindowHint(GLFW_SAMPLES, settings.msaaSamples ? 16 : 0);

    glfwWindowHint(GLFW_STEREO, settings.stereo ? GL_TRUE : GL_FALSE);

    glfwWindowHint(GLFW_FLOATING, settings.alwaysOnTop ? GL_TRUE : GL_FALSE);

    GLFWmonitor* monitor = settings.fullScreen ? glfwGetPrimaryMonitor() : NULL;
    int width = settings.width;
    int height = settings.height;

    // Use the current monitor video mode
    if (notNull(monitor)) {
        int count;
        const GLFWvidmode* modes = glfwGetVideoModes(monitor, &count);
        const GLFWvidmode& mode = findClosestVidMode(modes, count, settings);

#       if 0 // TEMPORARY.
        debugPrintf("Monitor modes available:\n");
        for (int i = 0; i < count; ++i) {
            debugPrintf("%s\n", toString(modes[i]).c_str());
        }
        debugPrintf("Monitor mode selected: %s\n", toString(mode).c_str());
#       endif

        glfwWindowHint(GLFW_RED_BITS, mode.redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode.greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode.blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode.refreshRate);
        m_settings.refreshRate = mode.refreshRate;
        m_settings.width = mode.width;
        m_settings.height = mode.height;
        width = m_settings.width;
        height = m_settings.height;
    }

    m_glfwWindow = glfwCreateWindow(width, height, settings.caption.c_str(), monitor, share);

    if (isNull(m_glfwWindow)) {
        glfwTerminate();
        alwaysAssertM(false, "Failed to open GLFW window\n");
    }

    if (! settings.fullScreen) {
       setFullPosition(windowX, windowY);
    }

    if (settings.visible) {
        glfwShowWindow(m_glfwWindow);
    }
    
    if (settings.fullScreen) {
        glfwSetInputMode(m_glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    updateSettings();
    m_cursorInside = true;
    setEventCallbacks();
    initializeCurrentKeyMod();
    glfwMakeContextCurrent(m_glfwWindow);
    glfwSwapInterval(settings.asynchronous ? 0 : 1);
    GLCaps::init();
    debugAssertGLOk();
    updateJoystickMapping();
	if (! settings.defaultIconFilename.empty()) {
		setIcon(settings.defaultIconFilename);
	}
}


GLFWWindow::~GLFWWindow(){
    glfwTerminate();
}


void GLFWWindow::getSettings(OSWindow::Settings& settings) const {
    settings = m_settings;
}

    
int GLFWWindow::width() const {
    return m_settings.width;
}


int GLFWWindow::height() const {
    return m_settings.height;
}

    
Rect2D GLFWWindow::clientRect() const {
    int x,y;
    glfwGetWindowPos(m_glfwWindow, &x, &y);
    return Rect2D::xywh((float)x, (float)y, (float)width(), (float)height());
}


void GLFWWindow::setSize(int width, int height){
    glfwSetWindowSize(m_glfwWindow, width, height);
}


void GLFWWindow::setClientRect(const Rect2D &dims){
    setSize((int)dims.width(), (int)dims.height());
    setClientPosition((int)dims.x0(), (int)dims.y0());

}
    

void GLFWWindow::setClientPosition(int x, int y) {
    glfwSetWindowPos(m_glfwWindow, x, y);
}


Vector2int32 GLFWWindow::getDecoration(int width, int height, OSWindow::Settings s) {
#ifdef G3D_WINDOWS
    DWORD style = 0;

    if (s.framed) {
        // http://msdn.microsoft.com/library/default.asp?url=/library/en-us/winui/WinUI/WindowsUserInterface/Windowing/Windows/WindowReference/WindowStyles.asp
        style |= WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | (s.resizable? WS_SIZEBOX : 1) | (s.allowMaximize? WS_MAXIMIZEBOX : 1);
    } else {
        style |= WS_POPUP;
    }
    RECT rect = { 0, 0, width, height };
    AdjustWindowRect(&rect, style, false);
    // This does not seem to be right...should be rect.right - rect.left, rect.top - rect.bottom, but this returns the right answer
    return Vector2int32( -(int32)rect.left, -(int32)rect.top);
#else
    return Vector2int32(0,0);
#endif
}


void GLFWWindow::setFullPosition(int x, int y) {
    Vector2int32 decorationSize = getDecoration(width(), height(), m_settings);
    glfwSetWindowPos(m_glfwWindow, x + decorationSize.x*2, y + decorationSize.y);
}
    

bool GLFWWindow::hasFocus() const {
    int result = glfwGetWindowAttrib(m_glfwWindow, GLFW_FOCUSED);
    return (result == GL_TRUE);
}
    

String GLFWWindow::getAPIVersion() const {
    String versionString = glfwGetVersionString();
    return versionString;
}


String GLFWWindow::getAPIName() const{
    return "GLFW";
}


void GLFWWindow::setGammaRamp(const Array<uint16>& gammaRamp) {
    alwaysAssertM(gammaRamp.size() == 256, "Gamma ramp must have 256 entries");

    uint16* ptr = const_cast<uint16*>(gammaRamp.getCArray());
    GLFWgammaramp glfwGRamp;
    uint16* red    = (uint16*)malloc(sizeof(uint16) * 256);
    uint16* green = (uint16*)malloc(sizeof(uint16) * 256);
    uint16* blue   = (uint16*)malloc(sizeof(uint16) * 256);
    for (int i = 0; i < 256; ++i) {
        red[i] = blue[i] = green[i] = ptr[i]; 
    }
    glfwGRamp.red   = red;
    glfwGRamp.blue  = blue;
    glfwGRamp.green = green;
    glfwGRamp.size = 256;

    int monitorCount;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
    for (int i = 0; i < monitorCount; ++i) {
      glfwSetGammaRamp(monitors[i], &glfwGRamp);
    }
}
   

void GLFWWindow::setCaption(const String& title) {
    m_settings.caption = title;
    glfwSetWindowTitle(m_glfwWindow, title.c_str());
}


String GLFWWindow::caption(){
    return m_settings.caption;
}


void GLFWWindow::updateJoystickMapping() {
    m_joystickMapping.fastClear();
    for(int i = 0; i <= GLFW_JOYSTICK_LAST - GLFW_JOYSTICK_1; ++i) {
      if(glfwJoystickPresent(GLFW_JOYSTICK_1 + i)) {
        m_joystickMapping.append(i);
      }
    }
}
  

int GLFWWindow::numJoysticks() const {
    return m_joystickMapping.size();
}
  

String GLFWWindow::joystickName(unsigned int stickNum) const {
    alwaysAssertM(numJoysticks() > int(stickNum), format("Tried to access joystick %d, only %d available.", stickNum, numJoysticks()));
    return glfwGetJoystickName(m_joystickMapping[stickNum]);
}
    

void GLFWWindow::setIcon(const shared_ptr<Image>& src) {
#   ifdef G3D_WINDOWS
        alwaysAssertM((src->format() == ImageFormat::RGB8()) ||
            (src->format() == ImageFormat::RGBA8()), 
            "Icon image must have at least 3 channels.");

        // Convert to Windows BGRA format
        shared_ptr<Image> colorData = src->clone();
        colorData->convertToRGBA8();

        Array<uint8> binaryMaskData;
        binaryMaskData.resize(iCeil(src->width() * src->height() / 8.0f));

        // Create the binary mask by shifting in the appropriate bits
        System::memset(binaryMaskData.getCArray(), 0, binaryMaskData.size());
        for (int y = 0; y < src->height(); ++y) {
            for (int x = 0; x < src->width(); ++x) {
                Color4 pixel;
                colorData->get(Point2int32(x, y), pixel);
                uint8 bit = (pixel.a > 127 ? 1 : 0) << (x % 8);
                binaryMaskData[(y * (src->width() / 8)) + (x / 8)] |= bit;
            }
        }
        shared_ptr<PixelTransferBuffer> bgraColorBuffer = ImageConvert::convertBuffer(colorData->toPixelTransferBuffer(), ImageFormat::BGRA8());

        HBITMAP bwMask = ::CreateBitmap(src->width(), src->height(), 1, 1, binaryMaskData.getCArray());

        const void* ptr = bgraColorBuffer->mapRead();
        HBITMAP color  = ::CreateBitmap(src->width(), src->height(), 1, 32, ptr);
        bgraColorBuffer->unmap(ptr);

        ICONINFO iconInfo;
        iconInfo.xHotspot = 0;
        iconInfo.yHotspot = 0;
        iconInfo.hbmColor = color;
        iconInfo.hbmMask = bwMask;
        iconInfo.fIcon = true;

        HICON hicon = ::CreateIconIndirect(&iconInfo);
        m_usedIcons.insert((intptr_t)hicon);

        // Purposely leak any icon created indirectly like hicon becase we don't know whether to save it or not.
        HICON hsmall = (HICON)::SendMessage(glfwGetWin32Window(m_glfwWindow), WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)hicon);
        HICON hlarge = (HICON)::SendMessage(glfwGetWin32Window(m_glfwWindow), WM_SETICON, (WPARAM)ICON_BIG,   (LPARAM)hicon);

        if (m_usedIcons.contains((intptr_t)hsmall)) {
            ::DestroyIcon(hsmall);
            m_usedIcons.remove((intptr_t)hsmall);
        }

        if (m_usedIcons.contains((intptr_t)hlarge)) {
            ::DestroyIcon(hlarge);
            m_usedIcons.remove((intptr_t)hlarge);
        }

        ::DeleteObject(bwMask);
        ::DeleteObject(color);

#   else
#       ifdef G3D_OSX
	// Save to temporary file and load from disk
	char tempFilename [L_tmpnam];
	tmpnam(tempFilename);
	const String tempFilenameFinal = format("%s%s", tempFilename, ".png"); 
	src->save(tempFilenameFinal);
	setIcon(tempFilenameFinal);
	FileSystem::removeFile(tempFilenameFinal);
#       else
        fprintf(stderr, "GLFWWindow::setIcon not yet implemented on this platform.\n");
#       endif
#   endif
}


void GLFWWindow::setIcon(const String& imageFileName) {
	alwaysAssertM(FileSystem::exists(imageFileName), "Could not locate icon `" + imageFileName + "'");
#ifdef G3D_OSX
	setCocoaIcon(imageFileName.c_str());
	//   fprintf(stderr, "GLFWWindow::setIcon not yet implemented on this platform.\n");
#else
    setIcon(Image::fromFile(imageFileName));
#endif
}
  

void GLFWWindow::setRelativeMousePosition(double x, double y) {
    const int xpos = (int)x;
    const int ypos = (int)y;
    glfwSetCursorPos(m_glfwWindow, xpos, ypos);
}


void GLFWWindow::setRelativeMousePosition(const Vector2 &p){
    setRelativeMousePosition(p.x, p.y);
}


void GLFWWindow::getMousePosition(int &x, int &y) const {
    double x_d, y_d;
    glfwGetCursorPos(m_glfwWindow, &x_d, &y_d);
    x = (int)x_d;
    y = (int)y_d;
}


void GLFWWindow::getMouseButtonState(uint8 &mouseButtons) const {
    mouseButtons = 0;
    int shift = 0;
    for(int i = GLFW_MOUSE_BUTTON_1; i <= GLFW_MOUSE_BUTTON_8; ++i){
        if(glfwGetMouseButton(m_glfwWindow, i) == GLFW_PRESS) {
	        // Center and right(buttons 2 and 3) have swapped order in G3D
	        if (i == GLFW_MOUSE_BUTTON_2) ++shift;
	        if (i == GLFW_MOUSE_BUTTON_3) --shift;
	        mouseButtons |= (1 << shift);
	        if (i == GLFW_MOUSE_BUTTON_2) --shift;
	        if (i == GLFW_MOUSE_BUTTON_3) ++shift;
        }
        ++shift;
    }
}


void GLFWWindow::getRelativeMouseState(Vector2 &position, uint8 &mouseButtons) const {
    int x,y;
    getMousePosition(x, y);
    position.x = (float)x;
    position.y = (float)y;
    getMouseButtonState(mouseButtons);
}


void GLFWWindow::getRelativeMouseState(int &x, int &y, uint8 &mouseButtons) const {
    getMousePosition(x, y);
    getMouseButtonState(mouseButtons);
}


void GLFWWindow::getRelativeMouseState(double &x, double &y, uint8 &mouseButtons) const {
    int intX, intY;
    getMousePosition(intX, intY);
    x = intX;
    y = intY;
    getMouseButtonState(mouseButtons);  
}


void GLFWWindow::getJoystickState(unsigned int stickNum, Array<float> &axis, Array<bool> &buttons) const {
    alwaysAssertM(numJoysticks() > int(stickNum), format("Tried to access joystick %d, only %d available.", stickNum, numJoysticks()));
    const int mappedStickNum = m_joystickMapping[stickNum];
    int axisCount, buttonCount;
    const float* axesValues   = glfwGetJoystickAxes(mappedStickNum, &axisCount);
    const unsigned char* buttonValues = glfwGetJoystickButtons(mappedStickNum, &buttonCount);
    axis.resize(axisCount, false);
    buttons.resize(buttonCount, false);

    for (int i = 0; i < axisCount; ++i) {
        axis[i] = axesValues[i];
    }

    for (int i = 0; i < buttonCount; ++i) {
        buttons[i] = (buttonValues[i] == GLFW_PRESS);
    }
}
    

int GLFWWindow::visibleCursorMode() {
    return m_inputCapture ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL;
}


void GLFWWindow::setInputCapture(bool c) {
    if (c != m_inputCapture) {
        m_inputCapture = c;
        if (mouseVisible()) {
            glfwSetInputMode(m_glfwWindow, GLFW_CURSOR, visibleCursorMode());
        }
    }
}


bool GLFWWindow::inputCapture() const {
  return m_inputCapture;
}
    

void GLFWWindow::setMouseVisible(bool b){
    m_mouseHideCount = b ? 0 : 1;
    
    if (m_mouseVisible == b) {
        return;
    }

    int cursorMode = b ? visibleCursorMode() : GLFW_CURSOR_DISABLED;

    glfwSetInputMode(m_glfwWindow, GLFW_CURSOR, cursorMode);
  
    m_mouseVisible = b;
}


bool GLFWWindow::mouseVisible() const {
    return m_mouseVisible;
}


bool GLFWWindow::isIconified() const {
    return m_iconified;
}


void GLFWWindow::setIconified(bool b) {
    m_iconified = b;
}


void GLFWWindow::reallyMakeCurrent() const {
    glfwMakeContextCurrent(m_glfwWindow);
}

    
void GLFWWindow::swapGLBuffers() {
    glfwSwapBuffers(m_glfwWindow);
}
    
    
void GLFWWindow::getOSEvents(Queue<GEvent>& events) {
    (void)events;
    // Callbacks create our events
    glfwPollEvents();
}


void GLFWWindow::getDroppedFilenames(Array<String>& files) {
    files.fastClear();
    if(m_fileList.size() > 0) {
        files.append(m_fileList);
    }
}


String GLFWWindow::_clipboardText() const {
    return glfwGetClipboardString(m_glfwWindow); 
}


void GLFWWindow::_setClipboardText(const String& text) const {
    glfwSetClipboardString(m_glfwWindow, text.c_str());
}

} // namespace G3D 

