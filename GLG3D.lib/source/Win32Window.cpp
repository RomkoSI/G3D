/**
\file Win32Window.cpp

\maintainer Morgan McGuire
\cite       Written by Corey Taylor & Morgan McGuire
\cite       Special thanks to Max McGuire of Ironlore
\cite http://sites.google.com/site/opengltutorialsbyaks/introduction-to-opengl-3-2---tutorial-01

\created       2004-05-21
\edited        2012-12-25

Copyright 2000-2012, Morgan McGuire.
All rights reserved.
*/

#include "G3D/platform.h"

// This file is ignored on other platforms
#ifdef G3D_WINDOWS

#include "GLG3D/GLCaps.h"
#include "G3D/CPUPixelTransferBuffer.h"
#include "G3D/Log.h"
#include "GLG3D/Win32Window.h"
#include "GLG3D/glcalls.h"
#include "GLG3D/UserInput.h"
#include "directinput8.h"
#include <winuser.h>
#include <windowsx.h>
#include <shellapi.h> // for drag and drop

#include <time.h>
#include <sstream>
#include <crtdbg.h>
#include "G3D/ImageConvert.h"
#include "GLG3D/GApp.h" // for screenPrintf

#ifndef WM_MOUSEHWHEEL // Only defined on Vista
#define WM_MOUSEHWHEEL 0x020E
#endif

using G3D::_internal::_DirectInput;

/*
DirectInput8 support is added by loading dinupt8.dll if found.

COM calls are not used to limit the style of code and includes needed.
DirectInput8 has a special creation function that lets us do this properly.

The joystick state structure used simulates the exports found in dinput8.lib

The joystick axis order returned to users is: X, Y, Z, Slider1, Slider2, rX, rY, rZ.

Important:  The cooperation level of Win32Window joysticks is Foreground:Non-Exclusive.
This means that other programs can get access to the joystick(preferably non-exclusive) and the joystick
is only aquired when Win32Window is in the foreground.
*/

namespace G3D {

// OpenGL version
static int major = 0;
static int minor = 0;

/** Our own copy of the function pointer; we need to load this at an awkward time, so it is stashed in a global. */
static PFNWGLCREATECONTEXTATTRIBSARBPROC g3d_wglCreateContextAttribsARB = NULL;

// Deals with unicode/MBCS/char issues
static LPCTSTR toTCHAR(const String& str) {
#   if defined(_MBCS) || defined(_UNICODE)
    static const int LEN = 1024;
    static TCHAR x[LEN];

    MultiByteToWideChar(
        CP_ACP, 
        0, 
        str.c_str(), 
        -1, 
        x, 
        LEN);

    //swprintf(x, LEN, _T("%s"), str.c_str());
    return const_cast<LPCTSTR>(x);
#   else
    return str.c_str();
#   endif
}

static const UINT BLIT_BUFFER = 0xC001;

#define WGL_SAMPLE_BUFFERS_ARB    0x2041
#define WGL_SAMPLES_ARB            0x2042

static bool hasWGLMultiSampleSupport = false;

static unsigned int _sdlKeys[GKey::LAST];
static bool sdlKeysInitialized = false;

// Prototype static helper functions at end of file
static bool ChangeResolution(int, int, int, int);
static void makeKeyEvent(int, int, GEvent&);
static void initWin32KeyMap();

std::auto_ptr<Win32Window>& Win32Window::shareWindow() {
    static std::auto_ptr<Win32Window> s(NULL);
    return s;
}


static uint8 buttonsToUint8(const bool* buttons) {
    uint8 mouseButtons = 0;
    // Clear mouseButtons and set each button bit.
    mouseButtons |= (buttons[0] ? 1 : 0) << 0;
    mouseButtons |= (buttons[1] ? 1 : 0) << 1;
    mouseButtons |= (buttons[2] ? 1 : 0) << 2;
    mouseButtons |= (buttons[3] ? 1 : 0) << 4;
    mouseButtons |= (buttons[4] ? 1 : 0) << 8;
    return mouseButtons;
}

/** For windows drag-drop */
class  DropTarget : public IDropTarget {
public:

    virtual HRESULT STDMETHODCALLTYPE DragEnter( 
            /* [unique][in] */ __RPC__in_opt IDataObject *pDataObj,
            /* [in] */ DWORD grfKeyState,
            /* [in] */ POINTL pt,
            /* [out][in] */ __RPC__inout DWORD *pdwEffect) override {
        *pdwEffect = DROPEFFECT_COPY;
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE DragOver( 
        DWORD grfKeyState,
        POINTL pt,
        DWORD *pdwEffect) override {
        *pdwEffect = DROPEFFECT_COPY;
        return S_OK;
    }
        
    virtual HRESULT STDMETHODCALLTYPE DragLeave( void) override  {
        return S_OK;
    }
        
    virtual HRESULT STDMETHODCALLTYPE Drop( 
        IDataObject *pDataObj,
        DWORD grfKeyState,
        POINTL pt,
        DWORD *pdwEffect) override {
        *pdwEffect = DROPEFFECT_COPY;
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(const IID & id, void ** v) {
        return S_OK;
    }

    virtual ULONG STDMETHODCALLTYPE AddRef(void) {
        return 0;
    }

    virtual ULONG STDMETHODCALLTYPE Release(void) {
        return 0;
    }
};

Win32Window::Win32Window(const OSWindow::Settings& s, bool creatingShareWindow)
    : createdWindow(true),
    m_diDevices(NULL),
    m_sysEventQueue(NULL),
    m_dropTarget(NULL)
{

    initWGL();

    m_hDC = NULL;
    m_mouseVisible = true;
    m_inputCapture = false;
    m_thread = ::GetCurrentThread();

    if (! sdlKeysInitialized) {
        initWin32KeyMap();
    }

    m_settings = s;

    String name = "";

    // Add the non-client area
    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = m_settings.width;
    rect.bottom = m_settings.height;

    DWORD style = 0;

    if (s.framed) {

        // http://msdn.microsoft.com/library/default.asp?url=/library/en-us/winui/WinUI/WindowsUserInterface/Windowing/Windows/WindowReference/WindowStyles.asp
        style |= WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;

        if (s.resizable) {
            style |= WS_SIZEBOX;
            if (s.allowMaximize) {
                style |= WS_MAXIMIZEBOX;
            }
        }

    } else {

        // Show nothing but the client area (cannot move window with mouse)
        style |= WS_POPUP;
    }

    int oldTop = rect.top;
    int oldLeft = rect.left;
    int oldWidth = rect.right - rect.left;
    int oldHeight = rect.bottom - rect.top;
    AdjustWindowRect(&rect, style, false);

    m_clientRectOffset.x = float(oldLeft - rect.left);
    m_clientRectOffset.y = float(oldTop - rect.top);
    m_decorationDimensions.x = float((rect.right - rect.left) - oldWidth);
    m_decorationDimensions.y = float((rect.bottom - rect.top) - oldHeight);

    int total_width  = rect.right - rect.left;
    int total_height = rect.bottom - rect.top;

    int startX = 0;
    int startY = 0;

    // Don't make the shared window full screen
    bool fullScreen = s.fullScreen && ! creatingShareWindow;

    if (! fullScreen) {
        if (s.center) {

            startX = (GetSystemMetrics(SM_CXSCREEN) - total_width) / 2;
            startY = (GetSystemMetrics(SM_CYSCREEN) - total_height) / 2;

        } else {

            startX = s.x;
            startY = s.y;
        }
    }

    m_clientX = m_settings.x = startX;
    m_clientY = m_settings.y = startY;

    HWND window = CreateWindow(
        Win32Window::g3dWndClass(), 
        toTCHAR(name),
        style,
        startX,
        startY,
        total_width,
        total_height,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    if (! creatingShareWindow) {
        DragAcceptFiles(window, true);

        // TODO: Add support for dropping non-file data (e.g., zipfile streams)
        //m_dropTarget = new DropTarget();
        //HRESULT r = ::RegisterDragDrop(window, m_dropTarget);
    }

    alwaysAssertM(window != NULL, "");

    // Set early so windows messages have value
    m_window = window;

    SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)this);

    init(window, creatingShareWindow);

    // Set default icon if available
    if (! m_settings.defaultIconFilename.empty()) {
        try {

            shared_ptr<Image> defaultIcon = Image::fromFile(m_settings.defaultIconFilename);
            setIcon(defaultIcon);
        } catch (const Image::Error& e) {
            // Throw away default icon
            debugPrintf("OSWindow's default icon failed to load: %s (%s)", e.filename.c_str(), e.reason.c_str());
            logPrintf("OSWindow's default icon failed to load: %s (%s)", e.filename.c_str(), e.reason.c_str());            
        }
    }

    if (fullScreen) {
        // Change the desktop resolution if we are running in fullscreen mode
        if (!ChangeResolution(m_settings.width, m_settings.height, (m_settings.rgbBits * 3) + m_settings.alphaBits, m_settings.refreshRate)) {
            alwaysAssertM(false, "Failed to change resolution");
        }
    }

    if (s.visible) {
        ShowWindow(window, SW_SHOW);
    }
}



Win32Window::Win32Window(const OSWindow::Settings& s, HWND hwnd) : createdWindow(false), m_diDevices(NULL) {
    initWGL();

    m_thread = ::GetCurrentThread();
    m_settings = s;
    init(hwnd);
}


Win32Window::Win32Window(const OSWindow::Settings& s, HDC hdc) : createdWindow(false), m_diDevices(NULL)  {
    initWGL();

    m_thread = ::GetCurrentThread();
    m_settings = s;

    HWND hwnd = ::WindowFromDC(hdc);

    debugAssert(hwnd != NULL);

    init(hwnd);
}


// See http://msdn.microsoft.com/en-us/library/ms724385(VS.85).aspx
Vector2 Win32Window::primaryDisplaySize() {
    return Vector2((float)GetSystemMetrics(SM_CXSCREEN), (float)GetSystemMetrics(SM_CYSCREEN));
}

Vector2 Win32Window::virtualDisplaySize() {
    return Vector2((float)GetSystemMetrics(SM_CXVIRTUALSCREEN), (float)GetSystemMetrics(SM_CYVIRTUALSCREEN));
}

Vector2int32 Win32Window::primaryDisplayWindowSize() {
    return Vector2int32((int32)GetSystemMetrics(SM_CXMAXIMIZED), (int32)GetSystemMetrics(SM_CYMAXIMIZED));
}

int Win32Window::numDisplays() {
    return GetSystemMetrics(SM_CMONITORS);
}


Win32Window* Win32Window::create(const OSWindow::Settings& settings) {

    // Create Win32Window which uses DI8 joysticks but WM_ keyboard messages
    return new Win32Window(settings);    

}


Win32Window* Win32Window::create(const OSWindow::Settings& settings, HWND hwnd) {

    // Create Win32Window which uses DI8 joysticks but WM_ keyboard messages
    return new Win32Window(settings, hwnd);    

}

Win32Window* Win32Window::create(const OSWindow::Settings& settings, HDC hdc) {
    // Create Win32Window which uses DI8 joysticks but WM_ keyboard messages
    return new Win32Window(settings, hdc);    
}


void Win32Window::init(HWND hwnd, bool creatingShareWindow) {

    if (! creatingShareWindow && m_settings.sharedContext) {
        createShareWindow(m_settings);
    }

    m_window = hwnd;

    // Setup the pixel format properties for the output device
    m_hDC = GetDC(m_window);

    if (! creatingShareWindow) {
        // for glMakeCurrent()
        OpenGLWindowHDC = m_hDC;
    }

    bool foundARBFormat = false;

    // Index of the format
    int pixelFormat = 0;

    // Structural description of non-extended features
    PIXELFORMATDESCRIPTOR pixelFormatDesc;

    if (wglChoosePixelFormatARB != NULL) {
        // Use wglChoosePixelFormatARB to override the pixel format choice for antialiasing.
        // Based on http://nehe.gamedev.net/data/lessons/lesson.asp?lesson=46
        // and http://oss.sgi.com/projects/ogl-sample/registry/ARB/wgl_pixel_format.txt

        Array<float> fAttributes;
        Array<int>   iAttributes;

        // End sentinel; we have no float attribute to set
        fAttributes.append(0.0f, 0.0f);

        iAttributes.append(WGL_DRAW_TO_WINDOW_ARB, GL_TRUE);
        iAttributes.append(WGL_SUPPORT_OPENGL_ARB, GL_TRUE);

        // NVIDIA docs say to use WGL_SWAP_EXCHANGE_ARB, but Evan Hart recommended WGL_SWAP_UNDEFINED_ARB instead for SLI
        // Setting WGL_SWAP_UNDEFINED_ARB appears to disable multisample, however.
        //iAttributes.append(WGL_SWAP_METHOD_ARB,    WGL_SWAP_UNDEFINED_ARB);
        
        if (m_settings.hardware) {
            iAttributes.append(WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB);
        }
        
        iAttributes.append(WGL_DOUBLE_BUFFER_ARB,  GL_TRUE);
        iAttributes.append(WGL_COLOR_BITS_ARB,     m_settings.rgbBits * 3);
        iAttributes.append(WGL_RED_BITS_ARB,       m_settings.rgbBits);
        iAttributes.append(WGL_GREEN_BITS_ARB,     m_settings.rgbBits);
        iAttributes.append(WGL_BLUE_BITS_ARB,      m_settings.rgbBits);
        iAttributes.append(WGL_ALPHA_BITS_ARB,     m_settings.alphaBits);
        iAttributes.append(WGL_DEPTH_BITS_ARB,     m_settings.depthBits);
        iAttributes.append(WGL_STENCIL_BITS_ARB,   m_settings.stencilBits);
        iAttributes.append(WGL_STEREO_ARB,         m_settings.stereo);
        iAttributes.append(WGL_AUX_BUFFERS_ARB,    0);        
        iAttributes.append(WGL_ACCUM_BITS_ARB,     0);        
        iAttributes.append(WGL_ACCUM_RED_BITS_ARB, 0);        
        iAttributes.append(WGL_ACCUM_GREEN_BITS_ARB, 0);        
        iAttributes.append(WGL_ACCUM_BLUE_BITS_ARB, 0);        


        if (hasWGLMultiSampleSupport && (m_settings.msaaSamples > 1)) {
            // On some ATI cards, even setting the samples to false will turn it on,
            // so we only take this branch when FSAA is explicitly requested.
            iAttributes.append(WGL_SAMPLE_BUFFERS_ARB, 1);
            iAttributes.append(WGL_SAMPLES_ARB,        m_settings.msaaSamples);
        } else {
            // Report actual settings
            m_settings.msaaSamples = 0;
        }
        iAttributes.append(0, 0); // end sentinel

        // http://www.nvidia.com/dev_content/nvopenglspecs/WGL_ARB_pixel_format.txt
        uint32 numFormats;
        int valid = wglChoosePixelFormatARB(
            m_hDC,
            iAttributes.getCArray(), 
            fAttributes.getCArray(),
            1,
            &pixelFormat,
            &numFormats);

        // "If the function succeeds, the return value is TRUE. If the function
        // fails the return value is FALSE. To get extended error information,
        // call GetLastError. If no matching formats are found then nNumFormats
        // is set to zero and the function returns TRUE."  

        if (valid && (numFormats > 0)) {
            // Found a valid format
            foundARBFormat = true;
            // Write out the description
            DescribePixelFormat(m_hDC, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pixelFormatDesc);
        }
    }

    if ( ! foundARBFormat ) {

        ZeroMemory(&pixelFormatDesc, sizeof(PIXELFORMATDESCRIPTOR));

        pixelFormatDesc.nSize        = sizeof(PIXELFORMATDESCRIPTOR);
        pixelFormatDesc.nVersion     = 1; 
        pixelFormatDesc.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_SWAP_EXCHANGE;
        pixelFormatDesc.iPixelType   = PFD_TYPE_RGBA; 
        pixelFormatDesc.cColorBits   = m_settings.rgbBits * 3;
        pixelFormatDesc.cDepthBits   = m_settings.depthBits;
        pixelFormatDesc.cStencilBits = m_settings.stencilBits;
        pixelFormatDesc.iLayerType   = PFD_MAIN_PLANE; 
        pixelFormatDesc.cRedBits     = m_settings.rgbBits;
        pixelFormatDesc.cGreenBits   = m_settings.rgbBits;
        pixelFormatDesc.cBlueBits    = m_settings.rgbBits;
        pixelFormatDesc.cAlphaBits   = m_settings.alphaBits;
        pixelFormatDesc.cAlphaBits   = m_settings.alphaBits;
        pixelFormatDesc.cAuxBuffers  = 0;
        pixelFormatDesc.cAccumBits   = 0;

        // Reset for completeness
        pixelFormat = 0;

        // Get the initial pixel format.  We'll override this below in a moment.
        pixelFormat = ChoosePixelFormat(m_hDC, &pixelFormatDesc);
    }

    // debugAssert(pixelFormatDesc.dwFlags & PFD_SWAP_EXCHANGE);

    alwaysAssertM(pixelFormat != 0, "[0] Unsupported video mode");

    if (SetPixelFormat(m_hDC, pixelFormat, &pixelFormatDesc) == FALSE) {
        alwaysAssertM(false, "[1] Unsupported video mode");
    }

    HGLRC shareContext = 0;
    if (! creatingShareWindow && (shareWindow().get() != NULL)) {
        // Share resources with the shareWindow window.  Note that this
        // only occurs if there is a shareWindow, which may not be the 
        // case, depending on m_settings.
        shareContext = shareWindow().get()->m_glContext;
    }

    // Create the OpenGL context
    const int attribList[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, major,
        WGL_CONTEXT_MINOR_VERSION_ARB, minor, 
#ifdef G3D_DEBUG
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif  
        // Request backwards compatibility to pre-3.0 OpenGL, as recommended by NVIDIA:
        // http://developer.nvidia.com/object/opengl_3_driver.html
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB, 
        0
    };

    if (g3d_wglCreateContextAttribsARB != NULL) {
        m_glContext = g3d_wglCreateContextAttribsARB(m_hDC, shareContext, attribList);
    } else {
        logPrintf("Warning: using wglCreateContext instead of wglCreateContextAttribsARB; OpenGL "
            "compatibility profile will not be available.\n");

        // Old method
        m_glContext = wglCreateContext(m_hDC);

        if (! creatingShareWindow && (shareWindow().get() != NULL)) {
            // Share resources with the shareWindow window.  Note that this
            // only occurs if there is a shareWindow, which may not be the 
            // case, depending on m_settings.
            wglShareLists(shareWindow()->m_glContext, m_glContext);
        }
    }
    alwaysAssertM(m_glContext != NULL, "Failed to create OpenGL context.");

    // Initialize mouse buttons to the unpressed state
    for (int i = 0; i < 8; ++i) {
        m_mouseButtons[i] = false;
    }

    // Clear all keyboard buttons to unpressed
    for (int i = 0; i < 256; ++i) {
        m_keyboardButtons[i] = false;
    }

    this->makeCurrent();

    if (! creatingShareWindow) {
        GLCaps::init();
        setCaption(m_settings.caption);
    }
}


int Win32Window::width() const {
    return m_settings.width;
}


int Win32Window::height() const {
    return m_settings.height;
}

void Win32Window::setClientRect(const Rect2D& dims) {
    setFullRect(Rect2D::xywh(dims.x0() - m_clientRectOffset.x, dims.y0() - m_clientRectOffset.y, 
        dims.width() + m_decorationDimensions.x, dims.height() + m_decorationDimensions.y));
}

void Win32Window::setFullRect(const Rect2D& dims) {

    int W = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int H = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);

    int x = iClamp((int)dims.x0(), 0, W);
    int y = iClamp((int)dims.y0(), 0, H);
    int w = iClamp((int)dims.width(), 1, W);
    int h = iClamp((int)dims.height(), 1, H);

    // Set dimensions and repaint.
    ::MoveWindow(m_window, x, y, w, h, true);
}


void Win32Window::setFullPosition(int x, int y) {
    setFullRect(Rect2D::xywh(Point2((float)x, (float)y), fullRect().wh())); 
}


Rect2D Win32Window::clientRect() const {
    return Rect2D::xywh((float)m_clientX, (float)m_clientY, (float)width(), (float)height());
}


Rect2D Win32Window::fullRect() const {
    return Rect2D::xywh((float)m_clientX - m_clientRectOffset.x, (float)m_clientY - m_clientRectOffset.y, 
        (float)width() + m_decorationDimensions.x, (float)height() + m_decorationDimensions.y);
}


bool Win32Window::hasFocus() const {
    // Double check state with foreground and visibility just to be sure.
    return ( (m_window == ::GetForegroundWindow()) && (::IsWindowVisible(m_window)) );
}


String Win32Window::getAPIVersion() const {
    return "1.0";
}


String Win32Window::getAPIName() const {
    return "Windows";
}


bool Win32Window::requiresMainLoop() const {
    return false;
}


void Win32Window::setIcon(const shared_ptr<Image>& src) {
    alwaysAssertM((src->format() == ImageFormat::RGB8()) ||
        (src->format() == ImageFormat::RGBA8()), 
        "Icon image must have at least 3 channels.");

//    alwaysAssertM((src.width() == 32) && (src.height() == 32),
//        "Icons must be 32x32 on windows.");


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
    m_usedIcons.insert((int)(intptr_t)hicon);

    // Purposely leak any icon created indirectly like hicon becase we don't know whether to save it or not.
    HICON hsmall = (HICON)::SendMessage(this->m_window, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)hicon);
    HICON hlarge = (HICON)::SendMessage(this->m_window, WM_SETICON, (WPARAM)ICON_BIG,   (LPARAM)hicon);

    if (m_usedIcons.contains((int)(intptr_t)hsmall)) {
        ::DestroyIcon(hsmall);
        m_usedIcons.remove((int)(intptr_t)hsmall);
    }

    if (m_usedIcons.contains((int)(intptr_t)hlarge)) {
        ::DestroyIcon(hlarge);
        m_usedIcons.remove((int)(intptr_t)hlarge);
    }

    ::DeleteObject(bwMask);
    ::DeleteObject(color);
}


void Win32Window::swapGLBuffers() {
    debugAssertGLOk();

    ::SwapBuffers(hdc());


#   ifdef G3D_DEBUG
        // Executing glGetError() after SwapBuffers() causes the CPU to block as if
        // a glFinish had been called, so we only do this in debug mode.
        GLenum e = glGetError();
        if (e == GL_INVALID_ENUM) {
            logPrintf("WARNING: SwapBuffers failed inside G3D::Win32Window; probably because "
                "the context changed when switching monitors.\n\n");
        }

        debugAssertGLOk();
#   endif
}


void Win32Window::close() {
    PostMessage(m_window, WM_CLOSE, 0, 0);
}


Win32Window::~Win32Window() {
    if (OSWindow::current() == this) {
        if (wglMakeCurrent(NULL, NULL) == FALSE)    {
            debugAssertM(false, "Failed to set context");
        }

        if (createdWindow) {
            // Release the mouse
            setMouseVisible(true);
            setInputCapture(false);
        }
    }

    if (createdWindow) {
        SetWindowLongPtr(m_window, GWLP_USERDATA, (LONG_PTR)NULL);
        close();
    }

    // Do not need to release private HDC's
    delete m_dropTarget;
    delete m_diDevices;
}


void Win32Window::getSettings(OSWindow::Settings& s) const {
    s = m_settings;
}


void Win32Window::setCaption(const String& caption) {
    if (m_title != caption) {
        m_title = caption;
        SetWindowText(m_window, toTCHAR(m_title));
    }
}


String Win32Window::caption() {
    return m_title;
}
     
void Win32Window::getOSEvents(Queue<GEvent>& events) {

    // init global event queue pointer for window proc
    m_sysEventQueue = &events;

    MSG message;

    while (PeekMessage(&message, m_window, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    RECT rect;
    GetWindowRect(m_window, &rect);
    m_settings.x = rect.left;
    m_settings.y = rect.top;

    GetClientRect(m_window, &rect);
    m_settings.width = rect.right - rect.left;
    m_settings.height = rect.bottom - rect.top;

    m_clientX = m_settings.x;
    m_clientY = m_settings.y;

    if (m_settings.framed) {
        // Add the border offset
        m_clientX    += GetSystemMetrics(m_settings.resizable ? SM_CXSIZEFRAME : SM_CXFIXEDFRAME);
        m_clientY += GetSystemMetrics(m_settings.resizable ? SM_CYSIZEFRAME : SM_CYFIXEDFRAME) + GetSystemMetrics(SM_CYCAPTION);
    }

    // reset global event queue pointer to be safe
    m_sysEventQueue = NULL;
}


void Win32Window::getDroppedFilenames(Array<String>& files) {
    files.fastClear();
    if (m_droppedFiles.size() > 0) {
        files.append(m_droppedFiles);
    }
}


void Win32Window::setMouseVisible(bool b) {
    m_mouseHideCount = b ? 0 : 1;

    if (m_mouseVisible == b) {
        return;
    }

    if (b) {
        while (ShowCursor(true) < 0);
    } else {
        while (ShowCursor(false) >= 0); 
    }

    m_mouseVisible = b;
}


bool Win32Window::mouseVisible() const {
    return m_mouseVisible;
}


bool Win32Window::inputCapture() const {
    return m_inputCapture;
}


void Win32Window::setGammaRamp(const Array<uint16>& gammaRamp) {
    alwaysAssertM(gammaRamp.size() >= 256, "Gamma ramp must have at least 256 entries");

    Log* debugLog = Log::common();

    uint16* ptr = const_cast<uint16*>(gammaRamp.getCArray());
    uint16 wptr[3 * 256];
    for (int i = 0; i < 256; ++i) {
        wptr[i] = wptr[i + 256] = wptr[i + 512] = ptr[i]; 
    }
    BOOL success = SetDeviceGammaRamp(hdc(), wptr);

    if (! success) {
        if (debugLog) {debugLog->println("Error setting gamma ramp! (Possibly LCD monitor)");}
    }
}


void Win32Window::setRelativeMousePosition(double x, double y) {
    SetCursorPos(iRound(x + m_clientX), iRound(y + m_clientY));
}


void Win32Window::setRelativeMousePosition(const Vector2& p) {
    setRelativeMousePosition(p.x, p.y);
}


void Win32Window::getRelativeMouseState(Vector2& p, uint8& mouseButtons) const {
    int x, y;
    getRelativeMouseState(x, y, mouseButtons);
    p.x = (float)x;
    p.y = (float)y;
}

void Win32Window::getRelativeMouseState(int& x, int& y, uint8& mouseButtons) const {
    POINT point;
    GetCursorPos(&point);
    x = point.x - m_clientX;
    y = point.y - m_clientY;

    mouseButtons = buttonsToUint8(m_mouseButtons);
}


void Win32Window::getRelativeMouseState(double& x, double& y, uint8& mouseButtons) const {
    int ix, iy;
    getRelativeMouseState(ix, iy, mouseButtons);
    x = ix;
    y = iy;
}

void Win32Window::enableDirectInput() const {
    if (m_diDevices == NULL) {
        m_diDevices = new _DirectInput(m_window);
    }
}

int Win32Window::numJoysticks() const {
    enableDirectInput();
    return m_diDevices->getNumJoysticks();
}


String Win32Window::joystickName(unsigned int sticknum) const {
    enableDirectInput();
    return m_diDevices->getJoystickName(sticknum);
}


void Win32Window::getJoystickState(unsigned int stickNum, Array<float>& axis, Array<bool>& button) const {

    enableDirectInput();
    if (! m_diDevices->joystickExists(stickNum)) {
        return;
    }

    int povDegrees = 0xFFFF;
    m_diDevices->getJoystickState(stickNum, axis, button, povDegrees);

    // The povDegrees describes extra buttons for the POV hat
    switch (povDegrees) {
    case 0:        button.append( true, false, false, false);        break;
    case 45:       button.append( true,  true, false, false);        break;
    case 90:       button.append(false,  true, false, false);        break;
    case 135:      button.append(false,  true,  true, false);        break;
    case 180:      button.append(false, false,  true, false);        break;
    case 225:      button.append(false, false,  true,  true);        break;
    case 270:      button.append(false, false, false,  true);        break;
    case 315:      button.append(true,  false, false, false);        break;
    default:       button.append(false, false, false, false);        break;
    }
}


void Win32Window::setInputCapture(bool c) {
    m_inputCaptureCount = c ? 1 : 0;

    if (c != m_inputCapture) {
        m_inputCapture = c;

        if (m_inputCapture) {
            RECT wrect;
            GetWindowRect(m_window, &wrect);
            m_clientX = wrect.left;
            m_clientY = wrect.top;

            RECT rect = 
            {m_clientX + (LONG)m_clientRectOffset.x, 
            m_clientY + (LONG)m_clientRectOffset.y, 
            m_clientX + m_settings.width + (LONG)m_clientRectOffset.x, 
            m_clientY + m_settings.height + (LONG)m_clientRectOffset.y};
            ClipCursor(&rect);
        } else {
            ClipCursor(NULL);
        }
    }
}


static void getGLVersion(int& major, int& minor) {
    // for all versions
    char* ver = (char*)glGetString(GL_VERSION); // e.g., ver = "3.2.0"

    major = ver[0] - '0';
    if (major >= 3) {
        // for GL 3.x
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);
    } else {
        minor = ver[2] - '0';
    }

    // GLSL
    // ver = (char*)glGetString(GL_SHADING_LANGUAGE_VERSION); // ver = "1.50 NVIDIA via Cg compiler"
}


void Win32Window::initWGL() {
    // This function need only be called once
    static bool wglInitialized = false;
    if (wglInitialized) {
        return;
    }
    wglInitialized = true;

    // See http://nehe.gamedev.net/wiki/GL30_Lesson01Win.ashx for discussion

    String name = "G3D Temp Window";
    WNDCLASS window_class;

    window_class.style         = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc   = Win32Window::windowProc;
    window_class.cbClsExtra    = 0; 
    window_class.cbWndExtra    = 0;
    window_class.hInstance     = GetModuleHandle(NULL);
    window_class.hIcon         = LoadIcon(NULL, IDI_APPLICATION); 
    window_class.hCursor       = LoadCursor(NULL, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    window_class.lpszMenuName  = toTCHAR(name);
    window_class.lpszClassName = _T("window"); 

    int ret = RegisterClass(&window_class);
    alwaysAssertM(ret, "Registration Failed");

    // Create some dummy pixel format.
    PIXELFORMATDESCRIPTOR pfd =    
    {
        sizeof (PIXELFORMATDESCRIPTOR),                                    // Size Of This Pixel Format Descriptor
        1,                                                                // Version Number
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_SWAP_EXCHANGE,
        PFD_TYPE_RGBA,                                                    // Request An RGBA Format
        24,                                                                // Select Our Color Depth
        0, 0, 0, 0, 0, 0,                                                // Color Bits Ignored
        0,                                                                // Alpha Buffer
        0,                                                                // Shift Bit Ignored
        0,                                                                // No Accumulation Buffer
        0, 0, 0, 0,                                                        // Accumulation Bits Ignored
        16,                                                                // 16Bit Z-Buffer (Depth Buffer)  
        0,                                                                // No Stencil Buffer
        0,                                                                // No Auxiliary Buffer
        PFD_MAIN_PLANE,                                                    // Main Drawing Layer
        0,                                                                // Reserved
        0, 0, 0                                                            // Layer Masks Ignored
    };

    HWND hWnd = CreateWindow(_T("window"), _T(""), 0, 0, 0, 100, 100, NULL, NULL, GetModuleHandle(NULL), NULL);
    debugAssert(hWnd);

    HDC  hDC  = GetDC(hWnd);
    debugAssert(hDC);

    int pixelFormat = ChoosePixelFormat(hDC, &pfd);
    debugAssert(pixelFormat);

    if (SetPixelFormat(hDC, pixelFormat, &pfd) == FALSE) {
        debugAssertM(false, "Failed to set pixel format");
    }

    HGLRC hRC = wglCreateContext(hDC);
    debugAssert(hRC);

    // wglMakeCurrent is the bottleneck of this routine; it takes about 0.1 s
    if (wglMakeCurrent(hDC, hRC) == FALSE)    {
        debugAssertM(false, "Failed to set context");
    }

    // We've now brought OpenGL online.  Grab the pointers we need and 
    // destroy everything.

    /* Failed attempt to load without creating a context:
    HMODULE opengl32dll = LoadLibrary(_T("opengl32"));
    debugAssert(opengl32dll != NULL);

    wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)GetProcAddress(opengl32dll, "wglChoosePixelFormatARB");
    debugAssert(w != NULL);

    wglGetExtensionsStringARB =
        (PFNWGLGETEXTENSIONSSTRINGARBPROC)GetProcAddress(opengl32dll, "wglGetExtensionsStringARB");
*/

    wglChoosePixelFormatARB =
        (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
    debugAssert(wglChoosePixelFormatARB != NULL);

    PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB =
        (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");


    // Load the create pointer while we have a valid context
    g3d_wglCreateContextAttribsARB = 
        (PFNWGLCREATECONTEXTATTRIBSARBPROC) wglGetProcAddress("wglCreateContextAttribsARB");

    getGLVersion(major, minor);

    if (wglGetExtensionsStringARB != NULL) {

        std::string wglExtensions = wglGetExtensionsStringARB(hDC);

        std::istringstream extensionsStream;
        extensionsStream.str(wglExtensions.c_str());

        std::string extension;
        while (extensionsStream >> extension) {
            if (extension == "WGL_ARB_multisample") {
                hasWGLMultiSampleSupport = true;
                break;
            }
        }

    } else {
        hasWGLMultiSampleSupport = false;
    }

    // Now destroy the dummy windows
    wglDeleteContext(hRC);            
    hRC = 0;    
    ReleaseDC(hWnd, hDC);    
    hWnd = 0;                
    DestroyWindow(hWnd);            
    hWnd = 0;
}


void Win32Window::createShareWindow(OSWindow::Settings settings) {
    static bool init = false;
    if (init) {
        return;
    }

    init = true;    

    // We want a small (low memory), invisible window
    settings.visible = false;
    settings.width = 16;
    settings.height = 16;
    settings.framed = false;
    settings.fullScreen = false;


    // This call will force us to re-enter createShareWindow, however
    // the second time through init will be true, so we'll skip the 
    // recursion.
    shareWindow().reset(new Win32Window(settings, true));
}


void Win32Window::reallyMakeCurrent() const {
    debugAssertM(m_thread == ::GetCurrentThread(), 
        "Cannot call OSWindow::makeCurrent on different threads.");

    if (wglMakeCurrent(m_hDC, m_glContext) == FALSE)    {
        debugAssertM(false, "Failed to set context");
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
Static helper functions for Win32Window
*/

/** Changes the screen resolution */
static bool ChangeResolution(int width, int height, int bpp, int refreshRate) {

    if (refreshRate == 0) {
        refreshRate = 85;
    }

    DEVMODE deviceMode;

    ZeroMemory(&deviceMode, sizeof(DEVMODE));

    int bppTries[3];
    bppTries[0] = bpp;
    bppTries[1] = 32;
    bppTries[2] = 16;

    deviceMode.dmSize       = sizeof(DEVMODE);
    deviceMode.dmPelsWidth  = width;
    deviceMode.dmPelsHeight = height;
    deviceMode.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

    if (refreshRate > 0) {
        deviceMode.dmDisplayFrequency = refreshRate;
        deviceMode.dmFields |= DM_DISPLAYFREQUENCY;
    }

    LONG result = -1;

    for (int i = 0; (i < 3) && (result != DISP_CHANGE_SUCCESSFUL); ++i) {
        deviceMode.dmBitsPerPel = bppTries[i];
        result = ChangeDisplaySettings(&deviceMode, CDS_FULLSCREEN);
    }

    // If it didn't work, try just changing the resolution and not the
    // refresh rate.
    if ((result != DISP_CHANGE_SUCCESSFUL) && (refreshRate > 0)) {
        deviceMode.dmFields &= ~DM_DISPLAYFREQUENCY;
        for (int i = 0; (i < 3) && (result != DISP_CHANGE_SUCCESSFUL); ++i) {
            deviceMode.dmBitsPerPel = bppTries[i];
            result = ChangeDisplaySettings(&deviceMode, CDS_FULLSCREEN);
        }
    }

    return result == DISP_CHANGE_SUCCESSFUL;
}


static void makeKeyEvent(int vkCode, int lParam, GEvent& e) {

    // If true, we're looking at the right hand version of
    // Fix VK_SHIFT, VK_CONTROL, VK_MENU
    bool extended = (lParam >> 24) & 0x01;

    // Check for normal letter event
    if ((vkCode >= 'A') && (vkCode <= 'Z')) {

        // Make key codes lower case canonically
        e.key.keysym.sym = (GKey::Value)(vkCode - 'A' + 'a');

    } else if (vkCode == VK_SHIFT) {

        e.key.keysym.sym = extended ? GKey::RSHIFT : GKey::LSHIFT;

    } else if (vkCode == VK_CONTROL) {

        e.key.keysym.sym = extended ? GKey::RCTRL : GKey::LCTRL;

    } else if (vkCode == VK_MENU) {

        e.key.keysym.sym = extended ? GKey::RALT : GKey::LALT;

    } else {

        e.key.keysym.sym = (GKey::Value)_sdlKeys[iClamp(vkCode, 0, GKey::LAST - 1)];

    }

    e.key.keysym.scancode = MapVirtualKey(vkCode, 0); 
    //(lParam >> 16) & 0x7F;

    static BYTE lpKeyState[256];
    GetKeyboardState(lpKeyState);

    int mod = 0;
    if (lpKeyState[VK_LSHIFT] & 0x80) {
        mod = mod | GKeyMod::LSHIFT;
    }

    if (lpKeyState[VK_RSHIFT] & 0x80) {
        mod = mod | GKeyMod::RSHIFT;
    }

    if (lpKeyState[VK_LCONTROL] & 0x80) {
        mod = mod | GKeyMod::LCTRL;
    }

    if (lpKeyState[VK_RCONTROL] & 0x80) {
        mod = mod | GKeyMod::RCTRL;
    }

    if (lpKeyState[VK_LMENU] & 0x80) {
        mod = mod | GKeyMod::LALT;
    }

    if (lpKeyState[VK_RMENU] & 0x80) {
        mod = mod | GKeyMod::RALT;
    }
    e.key.keysym.mod = (GKeyMod::Value)mod;

    ToUnicode(vkCode, e.key.keysym.scancode, lpKeyState, (LPWSTR)&e.key.keysym.unicode, 1, 0);
}


void Win32Window::mouseButton(UINT mouseMessage, DWORD lParam, DWORD wParam) {
    GEvent e;

    // all events map to mouse 0 currently
    e.button.which = 0;

    // xy position of click
    e.button.x = GET_X_LPARAM(lParam);
    e.button.y = GET_Y_LPARAM(lParam);

    switch (mouseMessage) {
        case WM_LBUTTONDBLCLK:
            // double clicks are a regular click event with 2 clicks
            // only double click events have 2 clicks, all others are 1 click even in the same location
            e.type = GEventType::MOUSE_BUTTON_CLICK;
            e.button.numClicks = 2;
            e.button.button = 0;
            e.button.controlKeyIsDown = false;
            break;

        case WM_MBUTTONDBLCLK:
            e.type = GEventType::MOUSE_BUTTON_CLICK;
            e.button.numClicks = 2;
            e.button.button = 1;
            e.button.controlKeyIsDown = false;
            break;

        case WM_RBUTTONDBLCLK:
            e.type = GEventType::MOUSE_BUTTON_CLICK;
            e.button.numClicks = 2;
            e.button.button = 2;
            e.button.controlKeyIsDown = false;
            break;

        case WM_XBUTTONDBLCLK:
            e.type = GEventType::MOUSE_BUTTON_CLICK;
            e.button.numClicks = 2;
            e.button.button = 3 + (((GET_XBUTTON_WPARAM(wParam) & XBUTTON2) != 0) ? 1 : 0);
            e.button.controlKeyIsDown = false;
            break;

        case WM_LBUTTONDOWN:
            // map a button down message to a down event, pressed state and button index of
            // 0 - left, 1 - middle, 2 - right, 3+ - x
            e.type = GEventType::MOUSE_BUTTON_DOWN;
            e.button.state = GButtonState::PRESSED;
            e.button.button = 0;
            e.button.controlKeyIsDown = false;
            break;

        case WM_MBUTTONDOWN:
            e.type = GEventType::MOUSE_BUTTON_DOWN;
            e.button.state = GButtonState::PRESSED;
            e.button.button = 1;
            e.button.controlKeyIsDown = false;
            break;

        case WM_RBUTTONDOWN:
            e.type = GEventType::MOUSE_BUTTON_DOWN;
            e.button.state = GButtonState::PRESSED;
            e.button.button = 2;
            e.button.controlKeyIsDown = false;
            break;

        case WM_XBUTTONDOWN:
            e.type = GEventType::MOUSE_BUTTON_DOWN;
            e.button.state = GButtonState::PRESSED;
            e.button.button = 3 + (((GET_XBUTTON_WPARAM(wParam) & XBUTTON2) != 0) ? 1 : 0);
            e.button.controlKeyIsDown = false;
            break;

        case WM_LBUTTONUP:
            // map a button up message to a up event, released state and button index of
            // 0 - left, 1 - middle, 2 - right, 3+ - x
            e.type = GEventType::MOUSE_BUTTON_UP;
            e.button.state = GButtonState::RELEASED;
            e.button.button = 0;
            e.button.controlKeyIsDown = false;
            break;

        case WM_MBUTTONUP:
            e.type = GEventType::MOUSE_BUTTON_UP;
            e.button.state = GButtonState::RELEASED;
            e.button.button = 1;
            e.button.controlKeyIsDown = false;
            break;

        case WM_RBUTTONUP:
            e.type = GEventType::MOUSE_BUTTON_UP;
            e.button.state = GButtonState::RELEASED;
            e.button.button = 2;
            e.button.controlKeyIsDown = false;
            break;

        case WM_XBUTTONUP:
            e.type = GEventType::MOUSE_BUTTON_UP;
            e.button.state = GButtonState::RELEASED;
            e.button.button = 3 + (((GET_XBUTTON_WPARAM(wParam) & XBUTTON2) != 0) ? 1 : 0);
            e.button.controlKeyIsDown = false;
            break;

        default:
            // handling non-mouse event?
            debugAssert(false);
    }

    // add initial event
    m_sysEventQueue->pushBack(e);

    // check for any clicks as defined by a button down then up sequence
    if (e.type == GEventType::MOUSE_BUTTON_UP && m_mouseButtons[e.button.button]) {

        // add click event from same location
        e.type = GEventType::MOUSE_BUTTON_CLICK;
        e.button.numClicks = 1;

        m_sysEventQueue->pushBack(e);
    }

    m_mouseButtons[e.button.button] = (e.type == GEventType::MOUSE_BUTTON_DOWN);
}


/**
Initializes SDL to Win32 key map
*/
static void initWin32KeyMap() {
    memset(_sdlKeys, 0, sizeof(_sdlKeys));

    _sdlKeys[VK_BACK] = GKey::BACKSPACE;
    _sdlKeys[VK_TAB] = GKey::TAB;
    _sdlKeys[VK_CLEAR] = GKey::CLEAR;
    _sdlKeys[VK_RETURN] = GKey::RETURN;
    _sdlKeys[VK_PAUSE] = GKey::PAUSE;
    _sdlKeys[VK_ESCAPE] = GKey::ESCAPE;
    _sdlKeys[VK_SPACE] = GKey::SPACE;
    _sdlKeys[VK_APOSTROPHE] = GKey::QUOTE;
    _sdlKeys[VK_COMMA] = GKey::COMMA;
    _sdlKeys[VK_MINUS] = GKey::MINUS;
    _sdlKeys[VK_PERIOD] = GKey::PERIOD;
    _sdlKeys[VK_SLASH] = GKey::SLASH;
    _sdlKeys['0'] = '0';
    _sdlKeys['1'] = '1';
    _sdlKeys['2'] = '2';
    _sdlKeys['3'] = '3';
    _sdlKeys['4'] = '4';
    _sdlKeys['5'] = '5';
    _sdlKeys['6'] = '6';
    _sdlKeys['7'] = '7';
    _sdlKeys['8'] = '8';
    _sdlKeys['9'] = '9';
    _sdlKeys[VK_SEMICOLON] = GKey::SEMICOLON;
    _sdlKeys[VK_EQUALS] = GKey::EQUALS;
    _sdlKeys[VK_LBRACKET] = GKey::LEFTBRACKET;
    _sdlKeys[VK_BACKSLASH] = GKey::BACKSLASH;
    _sdlKeys[VK_RBRACKET] = GKey::RIGHTBRACKET;
    _sdlKeys[VK_GRAVE] = GKey::BACKQUOTE;
    _sdlKeys[VK_BACKTICK] = GKey::BACKQUOTE;
    _sdlKeys[VK_DELETE] = GKey::DELETE;

    _sdlKeys[VK_NUMPAD0] = GKey::KP0;
    _sdlKeys[VK_NUMPAD1] = GKey::KP1;
    _sdlKeys[VK_NUMPAD2] = GKey::KP2;
    _sdlKeys[VK_NUMPAD3] = GKey::KP3;
    _sdlKeys[VK_NUMPAD4] = GKey::KP4;
    _sdlKeys[VK_NUMPAD5] = GKey::KP5;
    _sdlKeys[VK_NUMPAD6] = GKey::KP6;
    _sdlKeys[VK_NUMPAD7] = GKey::KP7;
    _sdlKeys[VK_NUMPAD8] = GKey::KP8;
    _sdlKeys[VK_NUMPAD9] = GKey::KP9;
    _sdlKeys[VK_DECIMAL] = GKey::KP_PERIOD;
    _sdlKeys[VK_DIVIDE] = GKey::KP_DIVIDE;
    _sdlKeys[VK_MULTIPLY] = GKey::KP_MULTIPLY;
    _sdlKeys[VK_SUBTRACT] = GKey::KP_MINUS;
    _sdlKeys[VK_ADD] = GKey::KP_PLUS;

    _sdlKeys[VK_UP] = GKey::UP;
    _sdlKeys[VK_DOWN] = GKey::DOWN;
    _sdlKeys[VK_RIGHT] = GKey::RIGHT;
    _sdlKeys[VK_LEFT] = GKey::LEFT;
    _sdlKeys[VK_INSERT] = GKey::INSERT;
    _sdlKeys[VK_HOME] = GKey::HOME;
    _sdlKeys[VK_END] = GKey::END;
    _sdlKeys[VK_PRIOR] = GKey::PAGEUP;
    _sdlKeys[VK_NEXT] = GKey::PAGEDOWN;

    _sdlKeys[VK_F1] = GKey::F1;
    _sdlKeys[VK_F2] = GKey::F2;
    _sdlKeys[VK_F3] = GKey::F3;
    _sdlKeys[VK_F4] = GKey::F4;
    _sdlKeys[VK_F5] = GKey::F5;
    _sdlKeys[VK_F6] = GKey::F6;
    _sdlKeys[VK_F7] = GKey::F7;
    _sdlKeys[VK_F8] = GKey::F8;
    _sdlKeys[VK_F9] = GKey::F9;
    _sdlKeys[VK_F10] = GKey::F10;
    _sdlKeys[VK_F11] = GKey::F11;
    _sdlKeys[VK_F12] = GKey::F12;
    _sdlKeys[VK_F13] = GKey::F13;
    _sdlKeys[VK_F14] = GKey::F14;
    _sdlKeys[VK_F15] = GKey::F15;

    _sdlKeys[VK_NUMLOCK] = GKey::NUMLOCK;
    _sdlKeys[VK_CAPITAL] = GKey::CAPSLOCK;
    _sdlKeys[VK_SCROLL] = GKey::SCROLLOCK;
    _sdlKeys[VK_RSHIFT] = GKey::RSHIFT;
    _sdlKeys[VK_LSHIFT] = GKey::LSHIFT;
    _sdlKeys[VK_RCONTROL] = GKey::RCTRL;
    _sdlKeys[VK_LCONTROL] = GKey::LCTRL;
    _sdlKeys[VK_RMENU] = GKey::RALT;
    _sdlKeys[VK_LMENU] = GKey::LALT;
    _sdlKeys[VK_RWIN] = GKey::RSUPER;
    _sdlKeys[VK_LWIN] = GKey::LSUPER;

    _sdlKeys[VK_HELP] = GKey::HELP;
    _sdlKeys[VK_PRINT] = GKey::PRINT;
    _sdlKeys[VK_SNAPSHOT] = GKey::PRINT;
    _sdlKeys[VK_CANCEL] = GKey::BREAK;
    _sdlKeys[VK_APPS] = GKey::MENU;

    sdlKeysInitialized = true;
}


#if 0
static void printPixelFormatDescription(int format, HDC hdc, TextOutput& out) {

    PIXELFORMATDESCRIPTOR pixelFormat;
    DescribePixelFormat(hdc, format, sizeof(PIXELFORMATDESCRIPTOR), &pixelFormat);

    /*
    typedef struct tagPIXELFORMATDESCRIPTOR { // pfd   
    WORD  nSize; 
    WORD  nVersion; 
    DWORD dwFlags; 
    BYTE  iPixelType; 
    BYTE  cColorBits; 
    BYTE  cRedBits; 
    BYTE  cRedShift; 
    BYTE  cGreenBits; 
    BYTE  cGreenShift; 
    BYTE  cBlueBits; 
    BYTE  cBlueShift; 
    BYTE  cAlphaBits; 
    BYTE  cAlphaShift; 
    BYTE  cAccumBits; 
    BYTE  cAccumRedBits; 
    BYTE  cAccumGreenBits; 
    BYTE  cAccumBlueBits; 
    BYTE  cAccumAlphaBits; 
    BYTE  cDepthBits; 
    BYTE  cStencilBits; 
    BYTE  cAuxBuffers; 
    BYTE  iLayerType; 
    BYTE  bReserved; 
    DWORD dwLayerMask; 
    DWORD dwVisibleMask; 
    DWORD dwDamageMask; 
    } PIXELFORMATDESCRIPTOR; 
    */

    out.printf("#%d Format Description\n", format);
    out.printf("nSize:\t\t\t\t%d\n", pixelFormat.nSize);
    out.printf("nVersion:\t\t\t%d\n", pixelFormat.nVersion);
    String s =
        (String((pixelFormat.dwFlags&PFD_DRAW_TO_WINDOW) ? "PFD_DRAW_TO_WINDOW|" : "") + 
         String((pixelFormat.dwFlags&PFD_DRAW_TO_BITMAP) ? "PFD_DRAW_TO_BITMAP|" : "") + 
         String((pixelFormat.dwFlags&PFD_SUPPORT_GDI) ? "PFD_SUPPORT_GDI|" : "") + 
         String((pixelFormat.dwFlags&PFD_SUPPORT_OPENGL) ? "PFD_SUPPORT_OPENGL|" : "") + 
         String((pixelFormat.dwFlags&PFD_GENERIC_ACCELERATED) ? "PFD_GENERIC_ACCELERATED|" : "") + 
         String((pixelFormat.dwFlags&PFD_GENERIC_FORMAT) ? "PFD_GENERIC_FORMAT|" : "") + 
         String((pixelFormat.dwFlags&PFD_NEED_PALETTE) ? "PFD_NEED_PALETTE|" : "") + 
         String((pixelFormat.dwFlags&PFD_NEED_SYSTEM_PALETTE) ? "PFD_NEED_SYSTEM_PALETTE|" : "") + 
         String((pixelFormat.dwFlags&PFD_DOUBLEBUFFER) ? "PFD_DOUBLEBUFFER|" : "") + 
         String((pixelFormat.dwFlags&PFD_STEREO) ? "PFD_STEREO|" : "") +
         String((pixelFormat.dwFlags&PFD_SWAP_LAYER_BUFFERS) ? "PFD_SWAP_LAYER_BUFFERS" : ""));

    out.printf("dwFlags:\t\t\t%s\n", s.c_str());
    out.printf("iPixelType:\t\t\t%d\n", pixelFormat.iPixelType);
    out.printf("cColorBits:\t\t\t%d\n", pixelFormat.cColorBits);
    out.printf("cRedBits:\t\t\t%d\n", pixelFormat.cRedBits);
    out.printf("cRedShift:\t\t\t%d\n", pixelFormat.cRedShift);
    out.printf("cGreenBits:\t\t\t%d\n", pixelFormat.cGreenBits);
    out.printf("cGreenShift:\t\t\t%d\n", pixelFormat.cGreenShift);
    out.printf("cBlueBits:\t\t\t%d\n", pixelFormat.cBlueBits);
    out.printf("cBlueShift:\t\t\t%d\n", pixelFormat.cBlueShift);
    out.printf("cAlphaBits:\t\t\t%d\n", pixelFormat.cAlphaBits);
    out.printf("cAlphaShift:\t\t\t%d\n", pixelFormat.cAlphaShift);
    out.printf("cAccumBits:\t\t\t%d\n", pixelFormat.cAccumBits);
    out.printf("cAccumRedBits:\t\t\t%d\n", pixelFormat.cAccumRedBits);
    out.printf("cAccumGreenBits:\t\t%d\n", pixelFormat.cAccumGreenBits);
    out.printf("cAccumBlueBits:\t\t\t%d\n", pixelFormat.cAccumBlueBits);
    out.printf("cAccumAlphaBits:\t\t%d\n", pixelFormat.cAccumAlphaBits);
    out.printf("cDepthBits:\t\t\t%d\n", pixelFormat.cDepthBits);
    out.printf("cStencilBits:\t\t\t%d\n", pixelFormat.cStencilBits);
    out.printf("cAuxBuffers:\t\t\t%d\n", pixelFormat.cAuxBuffers);
    out.printf("iLayerType:\t\t\t%d\n", pixelFormat.iLayerType);
    out.printf("bReserved:\t\t\t%d\n", pixelFormat.bReserved);
    out.printf("dwLayerMask:\t\t\t%d\n", pixelFormat.dwLayerMask);
    out.printf("dwDamageMask:\t\t\t%d\n", pixelFormat.dwDamageMask);

    out.printf("\n");
}
#endif

LRESULT CALLBACK Win32Window::windowProc(HWND     window,
                                         UINT     message,
                                         WPARAM   wParam,
                                         LPARAM   lParam) {

    Win32Window* this_window = (Win32Window*)GetWindowLongPtr(window, GWLP_USERDATA);

    if (this_window != NULL && this_window->m_sysEventQueue != NULL) {
        GEvent e;

        switch (message) {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:

            //http://msdn.microsoft.com/en-us/library/ms646280(v=vs.85).aspx

            // only tracking keys less than 256
            if (wParam < 256) {
                
                // Number of key repeats represented by this message.  This appears to
                // be 1 even for the first true key down.
                const int repeatCount = lParam & 0xFF;

                bool wasDown = ((lParam >> 30) & 1) == 1;

                // if repeating (bit 30 set), only add key down event if
                // we haven't seen it already

                if (! this_window->m_keyboardButtons[wParam] || ! wasDown) {
                    e.key.type = GEventType::KEY_DOWN;
                } else {
                    e.key.type = GEventType::KEY_REPEAT;
                }
                e.key.state = GButtonState::PRESSED;

                makeKeyEvent((int)wParam, (int)lParam, e);

                this_window->m_keyboardButtons[wParam] = true;

                for (int i = 0; i < repeatCount; ++i) {
                    this_window->m_sysEventQueue->pushBack(e);
                }

                // Generate char input events.
                if (e.key.keysym.unicode > 31 && e.key.keysym.unicode != 127) { // Not a control character
                    GEvent charInputEvent;
                    charInputEvent.type = GEventType::CHAR_INPUT;
                    charInputEvent.character.unicode = e.key.keysym.unicode;

                    for (int i = 0; i < repeatCount; ++i) {
                        this_window->m_sysEventQueue->pushBack(charInputEvent);
                    } 
                }


            } else {
                // we are processing a key press over 256 which we aren't tracking
                // check to see if the VK_ mappings are valid and now high enough
                // that we need to track more (try using a map)
                debugAssert(wParam < 256);
            }
            return 0;


        case WM_KEYUP:
        case WM_SYSKEYUP:
            // only tracking keys less than 256
            if (wParam < 256) {
                e.key.type = GEventType::KEY_UP;
                e.key.state = GButtonState::RELEASED;

                makeKeyEvent((int)wParam, (int)lParam, e);
                this_window->m_keyboardButtons[wParam] = false;

                this_window->m_sysEventQueue->pushBack(e);
            } else {
                // we are processing a key press over 256 which we aren't tracking
                // check to see if the VK_ mappings are valid and now high enough
                // that we need to track more (try using a map)
                debugAssert(wParam < 256);
            }
            return 0;

        case WM_MOUSEMOVE:
            e.motion.type = GEventType::MOUSE_MOTION;
            e.motion.which = 0; // TODO: mouse index
            e.motion.state = buttonsToUint8(this_window->m_mouseButtons);
            e.motion.x = GET_X_LPARAM(lParam);
            e.motion.y = GET_Y_LPARAM(lParam);
            if (true) {
                static int oldX = e.motion.x, oldY = e.motion.y;
                e.motion.xrel = e.motion.x - oldX;
                e.motion.yrel = e.motion.y - oldY;
                this_window->m_sysEventQueue->pushBack(e);
            }
            return 0;

        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_XBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_XBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        case WM_XBUTTONUP:
            this_window->mouseButton(message, (DWORD)lParam, (DWORD)wParam);
            return 0;

        case WM_MOUSEWHEEL:
            e.scroll2d.type = GEventType::MOUSE_SCROLL_2D;
            e.scroll2d.which = 0;
            e.scroll2d.dx = 0;
            e.scroll2d.dy = GET_WHEEL_DELTA_WPARAM(wParam);
            this_window->m_sysEventQueue->pushBack(e);
            return 0;

        case WM_MOUSEHWHEEL:
            e.scroll2d.type = GEventType::MOUSE_SCROLL_2D;
            e.scroll2d.which = 0;
            e.scroll2d.dx = GET_WHEEL_DELTA_WPARAM(wParam); // in increments of WHEEL_DELTA = 120
            e.scroll2d.dy = 0;
            this_window->m_sysEventQueue->pushBack(e);
            return 0;

        case WM_DROPFILES:
            {
                e.drop.type = GEventType::FILE_DROP;
                HDROP hDrop = (HDROP)wParam;
                POINT point;
                DragQueryPoint(hDrop, &point);
                e.drop.x = point.x;
                e.drop.y = point.y;

                enum {NUM_FILES=0xFFFFFFFF};

                int n = DragQueryFile(hDrop, (UINT)NUM_FILES, NULL, 0UL);
                this_window->m_droppedFiles.clear();
                for (int i = 0; i < n; ++i) {
                    int numChars = DragQueryFile(hDrop, i, NULL, 0);
                    char* temp = (char*)System::malloc(numChars + 2);
                    DragQueryFileA(hDrop, i, temp, numChars + 1);
                    String s = temp;
                    this_window->m_droppedFiles.append(s);
                    System::free(temp);
                }
                DragFinish(hDrop);

                this_window->m_sysEventQueue->pushBack(e);
            }
            return 0;

        case WM_CLOSE:
            // handle close event
            e.type = GEventType::QUIT;
            this_window->m_sysEventQueue->pushBack(e);
            DestroyWindow(window);
            return 0;

        case WM_SIZE:
            // handle resize event (minimized is ignored)
            if ((wParam == SIZE_MAXIMIZED) ||
                (wParam == SIZE_RESTORED))
            {
                // Add a size event that will be returned next OSWindow poll
                e.type = GEventType::VIDEO_RESIZE;
                e.resize.w = LOWORD(lParam);
                e.resize.h = HIWORD(lParam);
                this_window->m_sysEventQueue->pushBack(e);
                this_window->handleResize(e.resize.w, e.resize.h);
            }
            return 0;

        case WM_SETFOCUS:
            e.type = GEventType::FOCUS;
            e.focus.hasFocus = true;
            this_window->m_sysEventQueue->pushBack(e);
            return 0;

        case WM_KILLFOCUS:
            e.type = GEventType::FOCUS;
            e.focus.hasFocus = false;
            this_window->m_sysEventQueue->pushBack(e);

            // Release keys held down so that they do not get stuck
            for (int i = 0; i < sizeof(this_window->m_keyboardButtons); ++i) {
                if (this_window->m_keyboardButtons[i]) {
                    ::PostMessage(window, WM_KEYUP, i, 0);
                }
            }
            return 0;

        }
    }

    return DefWindowProc(window, message, wParam, lParam);
}

/** Return the G3D window class, which owns a private DC. 
    See http://www.starstonesoftware.com/OpenGL/whyyou.htm
    for a discussion of why this is necessary. */
LPCTSTR Win32Window::g3dWndClass() {

    static LPCTSTR g3dWindowClassName = NULL;

    if (g3dWindowClassName == NULL) {

        WNDCLASS wndcls;

        wndcls.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_OWNDC;
        wndcls.lpfnWndProc = Win32Window::windowProc;
        wndcls.cbClsExtra = wndcls.cbWndExtra = 0;
        wndcls.hInstance = ::GetModuleHandle(NULL);
        wndcls.hIcon = NULL;
        wndcls.hCursor = ::LoadCursor(NULL, IDC_ARROW);
        wndcls.hbrBackground = NULL;
        wndcls.lpszMenuName = NULL;
        wndcls.lpszClassName = _T("G3DWindow");

        if (!RegisterClass(&wndcls)) {
            Log::common()->printf("\n**** WARNING: could not create G3DWindow class ****\n");
            // error!  Return the default window class.
            return _T("window");
        }

        g3dWindowClassName = wndcls.lpszClassName;        
    }

    return g3dWindowClassName;
}


String Win32Window::_clipboardText() const {
    String s;

    if (OpenClipboard(NULL)) {
        HANDLE h = GetClipboardData(CF_TEXT);

        if (h) {
            char* temp = (char*)GlobalLock(h);
            if (temp) {
                s = temp;
            }
            temp = NULL;
            GlobalUnlock(h);
        }
        CloseClipboard();
    }
    return s;
}


void Win32Window::_setClipboardText(const String& s) const {
    // See http://hsivonen.iki.fi/kesakoodi/clipboard/
    if (OpenClipboard(NULL)) {
        HGLOBAL hMem = GlobalAlloc(GHND | GMEM_DDESHARE, s.size() + 1);
        if (hMem) {
            char *pMem = (char*)GlobalLock(hMem);
            strcpy(pMem, s.c_str());
            GlobalUnlock(hMem);

            EmptyClipboard();
            SetClipboardData(CF_TEXT, hMem);
        }

        CloseClipboard();
        GlobalFree(hMem);
    }
}


void Win32Window::getFullScreenResolutions(Array<Vector2int32>& array) {
    array.fastClear();

    // http://msdn.microsoft.com/en-us/library/windows/desktop/dd162611(v=vs.85).aspx
    DEVMODE m;
    m.dmSize = sizeof(DEVMODE);
    m.dmDriverExtra = 0;

    for (int i = 0; EnumDisplaySettings(NULL, i, &m) != 0; ++i) {
        // Intentionally ignore anything lower than 16bpp; that can't be useful
        // for OpenGL rendering.
        if (m.dmBitsPerPel >= 16) {
            array.append(Vector2int32(m.dmPelsWidth, m.dmPelsHeight));
        }
    }
}

} // G3D namespace

#endif // G3D_WINDOWS
