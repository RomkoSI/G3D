/**
 \file GApp.cpp

 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2003-11-03
 \edited  2016-03-23
 */

#include "G3D/platform.h"

#include "GLG3D/Camera.h"
#include "G3D/fileutils.h"
#include "G3D/Log.h"
#include "G3D/units.h"
#include "G3D/NetworkDevice.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/GApp.h"
#include "GLG3D/FirstPersonManipulator.h"
#include "GLG3D/UserInput.h"
#include "GLG3D/OSWindow.h"
#include "GLG3D/Draw.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/VideoRecordDialog.h"
#include "G3D/ParseError.h"
#include "G3D/FileSystem.h"
#include "GLG3D/GuiTextureBox.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/Light.h"
#include "GLG3D/AmbientOcclusion.h"
#include "GLG3D/Shader.h"
#include "GLG3D/MotionBlur.h"
#include "GLG3D/DepthOfField.h"
#include "GLG3D/SceneEditorWindow.h"
#include "GLG3D/UprightSplineManipulator.h"
#include "GLG3D/CameraControlWindow.h"
#include "GLG3D/GLPixelTransferBuffer.h"
#include "G3D/PixelTransferBuffer.h"
#include "G3D/CPUPixelTransferBuffer.h"
#include "GLG3D/DefaultRenderer.h"
#include "GLG3D/DebugTextWidget.h"

#include <time.h>

namespace G3D {

GApp* GApp::s_currentGApp = NULL;

/** Framerate when the app does not have focus.  Should be low, e.g., 4fps */
static const float BACKGROUND_FRAME_RATE = 4.0; // fps

// Forward reference
void initGLG3D(const G3DSpecification& spec = G3DSpecification());

GApp::Settings::Settings() :
    dataDir("<AUTO>"),
    debugFontName("console-small.fnt"),
    logFilename("log.txt"),
    useDeveloperTools(true),
    writeLicenseFile(true),
    colorGuardBandThickness(0, 0),
    depthGuardBandThickness(0, 0) {
    initGLG3D();
}


GApp::Settings::Settings(int argc, const char* argv[]) {
    *this = Settings();

    argArray.resize(argc);
    for (int i = 0; i < argc; ++i) {
        argArray[i] = argv[i];
    }
}

GApp::Settings::RendererSettings::RendererSettings() :
    factory(&(DefaultRenderer::create)),
    deferredShading(false),
    orderIndependentTransparency(false) {}


GApp* GApp::current() {
    return s_currentGApp;
}


void GApp::setCurrent(GApp* gApp) {
    s_currentGApp = gApp;
}


void screenPrintf(const char* fmt ...) {
    va_list argList;
    va_start(argList, fmt);
    if (GApp::current()) {
		GApp::current()->vscreenPrintf(fmt, argList);
    }
    va_end(argList);
}


void GApp::vscreenPrintf
(const char*                 fmt,
 va_list                     argPtr) {
    if (showDebugText) {
        String s = G3D::vformat(fmt, argPtr);
        const Array<String>& newlineSeparatedStrings = stringSplit(s, '\n');
        m_debugTextMutex.lock();
        debugText.append(newlineSeparatedStrings);
        m_debugTextMutex.unlock();
    }
}


DebugID debugDraw
(const shared_ptr<Shape>& shape,
 float             displayTime,
 const Color4&     solidColor,
 const Color4&     wireColor,
 const CFrame&     frame) {

	if (GApp::current()) {
        debugAssert(shape);
		GApp::DebugShape& s = GApp::current()->debugShapeArray.next();
        s.shape             = shape;
        s.solidColor        = solidColor;
        s.wireColor         = wireColor;
        s.frame             = frame;
        if (displayTime == 0) {
            s.endTime = 0;
        } else {
            s.endTime       = System::time() + displayTime;
        }
        s.id                = GApp::current()->m_lastDebugID++;
        return s.id;
    } else {
        return 0;
    }
}

DebugID debugDraw
(const Box& b,
 float             displayTime,
 const Color4&     solidColor,
 const Color4&     wireColor,
 const CoordinateFrame&     cframe){
     return debugDraw(shared_ptr<Shape>(new BoxShape(b)), displayTime, solidColor, wireColor, cframe);
}

DebugID debugDraw
(const Array<Vector3>& vertices,
 const Array<int>&     indices,
 float             displayTime,
 const Color4&     solidColor,
 const Color4&     wireColor,
 const CoordinateFrame&     cframe){
     return debugDraw(shared_ptr<Shape>(new MeshShape(vertices, indices)), displayTime, solidColor, wireColor, cframe);
}

DebugID debugDraw
(const CPUVertexArray& vertices,
 const Array<Tri>&     tris,
 float             displayTime,
 const Color4&     solidColor,
 const Color4&     wireColor,
 const CoordinateFrame&     cframe){
     return debugDraw(shared_ptr<Shape>(new MeshShape(vertices, tris)), displayTime, solidColor, wireColor, cframe);
}

DebugID debugDraw
(const Sphere& s,
 float             displayTime,
 const Color4&     solidColor,
 const Color4&     wireColor,
 const CoordinateFrame&     cframe){
     return debugDraw(shared_ptr<Shape>(new SphereShape(s)), displayTime, solidColor, wireColor, cframe);
}

DebugID debugDraw
(const Point3& p,
 float             displayTime,
 const Color4&     solidColor,
 const Color4&     wireColor,
 const CoordinateFrame&     cframe){
     const Sphere s(p, 0.0007);
     return debugDraw(shared_ptr<Shape>(new SphereShape(s)), displayTime, solidColor, wireColor, cframe);
}

DebugID debugDraw
(const CoordinateFrame& cf,
 float             displayTime,
 const Color4&     solidColor,
 const Color4&     wireColor,
 const CoordinateFrame&     cframe){
     return debugDraw(shared_ptr<Shape>(new AxesShape(cf)), displayTime, solidColor, wireColor, cframe);
}


DebugID debugDrawLabel
(const Point3& wsPos,
 const Vector3& csOffset,
 const GuiText text,
 float displayTime,
 float size,
 bool  sizeInPixels,
 const GFont::XAlign xalign,
 const GFont::YAlign yalign) {

	if (notNull(GApp::current())) {
		GApp::DebugLabel& L = GApp::current()->debugLabelArray.next();
        L.text = text;

		L.wsPos = wsPos + GApp::current()->activeCamera()->frame().vectorToWorldSpace(csOffset);
        if (sizeInPixels) {
            const float factor = -GApp::current()->activeCamera()->imagePlanePixelsPerMeter(GApp::current()->renderDevice->viewport());
			const float z = GApp::current()->activeCamera()->frame().pointToObjectSpace(L.wsPos).z;
            L.size  = (size / factor) * (float)abs(z);
        } else {
            L.size = size;
        }
        L.xalign = xalign;
        L.yalign = yalign;

        if (displayTime == 0) {
            L.endTime = 0;
        } else {
            L.endTime = System::time() + displayTime;
        }

		L.id = GApp::current()->m_lastDebugID;
		++GApp::current()->m_lastDebugID;
        return L.id;
    } else {
        return 0;
    }
}


DebugID debugDrawLabel
(const Point3& wsPos,
 const Vector3& csOffset,
 const String& text,
 const Color3& color,
 float displayTime,
 float size,
 bool  sizeInPixels,
 const GFont::XAlign xalign,
 const GFont::YAlign yalign) {
     return debugDrawLabel(wsPos, csOffset, GuiText(text, shared_ptr<GFont>(), -1.0f, color), displayTime, size, sizeInPixels, xalign, yalign);
}


/** Attempt to write license file */
static void writeLicense() {
    FILE* f = FileSystem::fopen("g3d-license.txt", "wt");
    if (f != NULL) {
        fprintf(f, "%s", license().c_str());
        FileSystem::fclose(f);
    }
}


void GApp::initializeOpenGL(RenderDevice* rd, OSWindow* window, bool createWindowIfNull, const Settings& settings) {
	if (notNull(rd)) {
		debugAssertM(notNull(window), "If you pass in your own RenderDevice, then you must also pass in your own OSWindow when creating a GApp.");

		m_hasUserCreatedRenderDevice = true;
		m_hasUserCreatedWindow = true;
		renderDevice = rd;
	} else if (createWindowIfNull) {
	    m_hasUserCreatedRenderDevice = false;
	    renderDevice = new RenderDevice();
        if (window != NULL) {
	        m_hasUserCreatedWindow = true;
		    renderDevice->init(window);
	    } else {
	        m_hasUserCreatedWindow = false;
		    renderDevice->init(settings.window);
	    }
    }


    if (isNull(renderDevice)) {
        return;
    }

    m_window = renderDevice->window();
    m_window->makeCurrent();
    m_osWindowDeviceFramebuffer = m_window->framebuffer();

    m_widgetManager = WidgetManager::create(m_window);
    userInput = new UserInput(m_window);

    {
        TextOutput t;

        t.writeSymbols("System","=", "{");
        t.pushIndent();
        t.writeNewline();
        System::describeSystem(t);
        if (renderDevice) {
            renderDevice->describeSystem(t);
        }

        NetworkDevice::instance()->describeSystem(t);
        t.writeNewline();
        t.writeSymbol("};");
        t.writeNewline();

        String s;
        t.commitString(s);
        logPrintf("%s\n", s.c_str());
    }
    m_debugCamera  = Camera::create("(Debug Camera)");
    m_activeCamera = m_debugCamera;

    debugAssertGLOk();
    loadFont(settings.debugFontName);
    debugAssertGLOk();

    shared_ptr<FirstPersonManipulator> manip = FirstPersonManipulator::create(userInput);
    manip->onUserInput(userInput);
    manip->setMoveRate(10);
    manip->setPosition(Vector3(0, 0, 4));
    manip->lookAt(Vector3::zero());
    manip->setMouseMode(FirstPersonManipulator::MOUSE_DIRECT_RIGHT_BUTTON);
    manip->setEnabled(true);
    m_debugCamera->setPosition(manip->translation());
    m_debugCamera->lookAt(Vector3::zero());
    setCameraManipulator(manip);
    m_debugController = manip;


    {
        GConsole::Settings settings;
        settings.backgroundColor = Color3::green() * 0.1f;
        console = GConsole::create(debugFont, settings, staticConsoleCallback, this);
        console->setActive(false);
        addWidget(console);
    }

    if (settings.film.enabled) {
        const ImageFormat* colorFormat = GLCaps::firstSupportedTexture(m_settings.film.preferredColorFormats);

        if (colorFormat == NULL) {
            // This GPU can't support the film class
            logPrintf("Warning: Disabled GApp::Settings::film.enabled because none of the provided color formats could be supported on this GPU.");
        } else {
            m_film = Film::create(colorFormat);

            m_osWindowHDRFramebuffer = Framebuffer::create("GApp::m_osWindowHDRFramebuffer");
            m_framebuffer = m_osWindowHDRFramebuffer;

            // The actual buffer allocation code:
            resize(renderDevice->width(), renderDevice->height());
        }
    }

    const shared_ptr<GFont>&    arialFont = GFont::fromFile(System::findDataFile("icon.fnt"));
    const shared_ptr<GuiTheme>& theme     = GuiTheme::fromFile(System::findDataFile("osx-10.7.gtm"), arialFont);

    debugWindow = GuiWindow::create("Control Window", theme,
        Rect2D::xywh(0.0f, 0.0f, (float)settings.window.width, 150.0f), GuiTheme::PANEL_WINDOW_STYLE, GuiWindow::NO_CLOSE);
    debugPane = debugWindow->pane();
    debugWindow->setVisible(false);
    addWidget(debugWindow);

    debugAssertGLOk();

    m_simTime     = 0;
    m_realTime    = 0;
    m_lastWaitTime  = System::time();

    m_depthOfField = DepthOfField::create();
    m_motionBlur   = MotionBlur::create();

    renderDevice->setColorClearValue(Color3(0.1f, 0.5f, 1.0f));

    m_gbufferSpecification.encoding[GBuffer::Field::CS_NORMAL]        = Texture::Encoding(ImageFormat::RGB10A2(), FrameName::CAMERA, 2.0f, -1.0f);
    m_gbufferSpecification.encoding[GBuffer::Field::DEPTH_AND_STENCIL]     = ImageFormat::DEPTH32();
    m_gbufferSpecification.depthEncoding = DepthEncoding::HYPERBOLIC;

    m_renderer = (settings.renderer.factory != NULL) ? settings.renderer.factory() : DefaultRenderer::create();
    const shared_ptr<DefaultRenderer>& defaultRenderer = dynamic_pointer_cast<DefaultRenderer>(m_renderer);

    if (settings.renderer.deferredShading && notNull(defaultRenderer)) {
        m_gbufferSpecification.encoding[GBuffer::Field::CS_FACE_NORMAL].format = NULL;
        m_gbufferSpecification.encoding[GBuffer::Field::EMISSIVE]           =
            GLCaps::supportsTexture(ImageFormat::RGB5()) ?
                Texture::Encoding(ImageFormat::RGB5(), FrameName::NONE, 3.0f, 0.0f) :
                Texture::Encoding(ImageFormat::R11G11B10F());

        m_gbufferSpecification.encoding[GBuffer::Field::LAMBERTIAN]         = ImageFormat::RGB8();
        m_gbufferSpecification.encoding[GBuffer::Field::GLOSSY]             = ImageFormat::RGBA8();

        defaultRenderer->setDeferredShading(true);
    }

    if (notNull(defaultRenderer)) {
        defaultRenderer->setOrderIndependentTransparency(settings.renderer.orderIndependentTransparency);
    }

    m_gbuffer = GBuffer::create(m_gbufferSpecification);
    m_gbuffer->resize(renderDevice->width() + m_settings.depthGuardBandThickness.x * 2, renderDevice->height() + m_settings.depthGuardBandThickness.y * 2);

    // Share the depth buffer with the forward-rendering pipeline
    m_osWindowHDRFramebuffer->set(Framebuffer::DEPTH, m_gbuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL));

    m_gbuffer->resize(renderDevice->width(), renderDevice->height());

    m_ambientOcclusion = AmbientOcclusion::create();

    // This program renders to texture for most 3D rendering, so it can
    // explicitly delay calling swapBuffers until the Film::exposeAndRender call,
    // since that is the first call that actually affects the back buffer.  This
    // reduces frame tearing without forcing vsync on.
    renderDevice->setSwapBuffersAutomatically(false);

    m_debugTextWidget = DebugTextWidget::create(this);
    addWidget(m_debugTextWidget, false);
}


GApp::GApp(const Settings& settings, OSWindow* window, RenderDevice* rd, bool createWindowIfNull) :
    m_lastDebugID(0),
    m_activeVideoRecordDialog(NULL),
    m_submitToDisplayMode(SubmitToDisplayMode::MAXIMIZE_THROUGHPUT),
    m_settings(settings),
    m_renderPeriod(1),
    m_endProgram(false),
    m_exitCode(0),
    m_debugTextColor(Color3::black()),
    m_debugTextOutlineColor(Color3(0.7f)),
    m_lastFrameOverWait(0),
    debugPane(NULL),
    renderDevice(NULL),
    userInput(NULL),
    m_lastWaitTime(System::time()),
    m_wallClockTargetDuration(1.0f / 60.0f),
    m_lowerFrameRateInBackground(true),
    m_simTimeStep(MATCH_REAL_TIME_TARGET),
    m_simTimeScale(1.0f),
    m_previousSimTimeStep(1.0f / 60.0f),
    m_previousRealTimeStep(1.0f / 60.0f),
    m_realTime(0),
    m_simTime(0) {

    setCurrent(this);

#   ifdef G3D_DEBUG
        // Let the debugger catch them
        catchCommonExceptions = false;
#   else
        catchCommonExceptions = true;
#   endif

    logLazyPrintf("\nEntering GApp::GApp()\n");
    char b[2048];
    (void)getcwd(b, 2048);
    logLazyPrintf("cwd = %s\n", b);

    if (settings.dataDir == "<AUTO>") {
        dataDir = FilePath::parent(System::currentProgramFilename());
    } else {
        dataDir = settings.dataDir;
    }
    logPrintf("System::setAppDataDir(\"%s\")\n", dataDir.c_str());
    System::setAppDataDir(dataDir);

    if (settings.writeLicenseFile && ! FileSystem::exists("g3d-license.txt")) {
        writeLicense();
    }

    if (m_settings.screenshotDirectory != "") {
        if (! isSlash(m_settings.screenshotDirectory[m_settings.screenshotDirectory.length() - 1])) {
            m_settings.screenshotDirectory += "/";
        }
        debugAssertM(FileSystem::exists(m_settings.screenshotDirectory),
            "GApp::Settings.screenshotDirectory set to non-existent directory " + m_settings.screenshotDirectory);
    }

    
    showDebugText               = true;
    escapeKeyAction             = ACTION_QUIT;
    showRenderingStats          = true;
    manageUserInput             = true;

    initializeOpenGL(renderDevice, window, createWindowIfNull, settings);

    logPrintf("Done GApp::GApp()\n\n");
}


const SceneVisualizationSettings& GApp::sceneVisualizationSettings() const {
    if (notNull(developerWindow) && notNull(developerWindow->sceneEditorWindow)) {
        return developerWindow->sceneEditorWindow->sceneVisualizationSettings();
    } else {
        static SceneVisualizationSettings v;
        return v;
    }
}


void GApp::createDeveloperHUD() {

    const shared_ptr<UprightSplineManipulator>& splineManipulator = UprightSplineManipulator::create(m_debugCamera);
    addWidget(splineManipulator);

    shared_ptr<GFont>      arialFont   = GFont::fromFile(System::findDataFile("arial.fnt"));
    shared_ptr<GuiTheme>   theme       = GuiTheme::fromFile(System::findDataFile("osx-10.7.gtm"), arialFont);

    developerWindow = DeveloperWindow::create
        (this,
            m_debugController,
            splineManipulator,
            Pointer<shared_ptr<Manipulator> >(this, &GApp::cameraManipulator, &GApp::setCameraManipulator),
            m_debugCamera,
            scene(),
            m_film,
            theme,
            console,
            Pointer<bool>(debugWindow, &GuiWindow::visible, &GuiWindow::setVisible),
            &showRenderingStats,
            &showDebugText,
            m_settings.screenshotDirectory);

    addWidget(developerWindow);
}


shared_ptr<GuiWindow> GApp::show(const shared_ptr<PixelTransferBuffer>& t, const String& windowCaption) {
    bool generateMipMaps = false;
    return show(Texture::fromPixelTransferBuffer("", t, NULL, Texture::DIM_2D, generateMipMaps), windowCaption);
}


shared_ptr<GuiWindow> GApp::show(const shared_ptr<Image>& t, const String& windowCaption) {
    return show(dynamic_pointer_cast<PixelTransferBuffer>(t->toPixelTransferBuffer()), windowCaption);
}


shared_ptr<GuiWindow> GApp::show(const shared_ptr<Texture>& t, const String& windowCaption) {
    static const Vector2 offset(25, 15);
    static Vector2 lastPos(0, 0);
    static float y0 = 0;

    lastPos += offset;

    String name;
    String dayTime;

    {
        // Use the current time as the name
        time_t t1;
        ::time(&t1);
        tm* t = localtime(&t1);
        static const char* day[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        int hour = t->tm_hour;
        const char* ap = "am";
        if (hour == 0) {
            hour = 12;
        } else if (hour >= 12) {
            ap = "pm";
            if (hour > 12) {
                hour -= 12;
            }
        }
        dayTime = format("%s %d:%02d:%02d %s", day[t->tm_wday], hour, t->tm_min, t->tm_sec, ap);
    }


    if (! windowCaption.empty()) {
        name = windowCaption + " - ";
    }
    name += dayTime;

    const shared_ptr<GuiWindow>& display = GuiWindow::create(name, shared_ptr<GuiTheme>(), Rect2D::xywh(lastPos,Vector2(0, 0)), GuiTheme::NORMAL_WINDOW_STYLE, GuiWindow::REMOVE_ON_CLOSE);

    GuiTextureBox* box = display->pane()->addTextureBox(this, t);
    box->setSizeFromInterior(t->vector2Bounds().min(Vector2(window()->width() * 0.9f, window()->height() * 0.9f)));
    box->zoomTo1();
    display->pack();

    // Cascade, but don't go off the screen
    if ((display->rect().x1() > window()->width()) || (display->rect().y1() > window()->height())) {
        lastPos = offset;
        lastPos.y += y0;
        y0 += offset.y;

        display->moveTo(lastPos);

        if (display->rect().y1() > window()->height()) {
            y0 = 0;
            lastPos = offset;
            display->moveTo(lastPos);
        }
    }

    addWidget(display);
    return display;
}


void GApp::drawMessage(const String& message) {
    drawTitle(message, "", Any(), Color3::black(), Color4(Color3::white(), 0.8f));
}


void GApp::drawTitle(const String& title, const String& subtitle, const Any& any, const Color3& fontColor, const Color4& backColor) {
    renderDevice->push2D();
    {
        // Background
        renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
        Draw::rect2D(renderDevice->viewport(), renderDevice, backColor);
        // Text
        const shared_ptr<GFont> font = debugWindow->theme()->defaultStyle().font;
        const float titleWidth = font->bounds(title, 1).x;
        const float titleSize  = min(30.0f, renderDevice->viewport().width() / titleWidth * 0.80f);
        font->draw2D(renderDevice, title, renderDevice->viewport().center(), titleSize,
                     fontColor, backColor, GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);

        float subtitleSize = 0.0f;
        if (! subtitle.empty()) {
            const float subtitleWidth = font->bounds(subtitle, 1).x;
            subtitleSize = min(22.5f, renderDevice->viewport().width() / subtitleWidth * 0.60f);
            font->draw2D(renderDevice, subtitle, renderDevice->viewport().center() + Vector2(0.0f, font->bounds(title, titleSize).y), subtitleSize,
                         fontColor, backColor, GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
        }

        if (! any.isNil()) {
            any.verifyType(Any::TABLE);
            const float anyTextSize = 20.0f;
            const float baseHeight = renderDevice->viewport().center().y + font->bounds(title, titleSize).y + font->bounds(subtitle, subtitleSize).y;
            const int maxEntriesPerColumn = (int) ((renderDevice->viewport().height() - baseHeight) / font->bounds("l", anyTextSize).y);
            const int cols                = (int) (1 + any.size() / maxEntriesPerColumn);

            Array<String> keys = any.table().getKeys();
            //determine the longest key in order to allign columns well
            Array<float> keyWidth;
            for (int c = 0; c < any.size()/cols; ++c) {
                keyWidth.append(0.0f);
                for (int i = c * maxEntriesPerColumn; i < min((c+1) * maxEntriesPerColumn, any.size()); ++i) {
                    float kwidth = font->bounds(keys[i], anyTextSize).x;
                    keyWidth[c]  = kwidth > keyWidth[c] ? kwidth : keyWidth[c];
                }
            }

            const float horizontalBuffer  = font->bounds("==", anyTextSize).x;
            const float heightIncrement   = font->bounds("==", anyTextSize).y;

            //distance from an edge of a screen to the center of a column, and between centers of columns
            const float centerDist = renderDevice->viewport().width() / (2 * cols);

            for (int c = 0; c < any.size()/cols; ++c) {
                float height = baseHeight;
                for (int i = c * maxEntriesPerColumn; i < min((c+1) * maxEntriesPerColumn, any.size()); ++i) {
                    float columnIndex = 2.0f*c + 1.0f;
                    font->draw2D(renderDevice, keys[i], Vector2(centerDist * (columnIndex) - (horizontalBuffer + keyWidth[c]), height), anyTextSize,
                         fontColor, backColor, GFont::XALIGN_LEFT, GFont::YALIGN_CENTER);
                    font->draw2D(renderDevice, " = ", Vector2(centerDist * (columnIndex), height), anyTextSize,
                         fontColor, backColor, GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
                    font->draw2D(renderDevice, any[keys[i]].unparse(), Vector2(centerDist * (columnIndex) + horizontalBuffer, height), anyTextSize,
                         fontColor, backColor, GFont::XALIGN_LEFT, GFont::YALIGN_CENTER);
                    height += heightIncrement;
                }
            }
        }
    }
    renderDevice->pop2D();
    renderDevice->swapBuffers();
}


void GApp::setExitCode(int code) {
    m_endProgram = true;
    m_exitCode = code;
}


void GApp::loadFont(const String& fontName) {
    logPrintf("Entering GApp::loadFont(\"%s\")\n", fontName.c_str());
    const String& filename = System::findDataFile(fontName);
    logPrintf("Found \"%s\" at \"%s\"\n", fontName.c_str(), filename.c_str());
    if (FileSystem::exists(filename)) {
        debugFont = GFont::fromFile(filename);
    } else {
        logPrintf(
            "Warning: G3D::GApp could not load font \"%s\".\n"
            "This may be because the G3D::GApp::Settings::dataDir was not\n"
            "properly set in main().\n",
            filename.c_str());

        debugFont.reset();
    }
    logPrintf("Done GApp::loadFont(...)\n");
}


GApp::~GApp() {
	if (current() == this) {
        setCurrent(NULL);
    }

    // Drop pointers to all OpenGL resources before shutting down the RenderDevice
    m_cameraManipulator.reset();

    m_film.reset();
    m_posed3D.clear();
    m_posed2D.clear();
    m_framebuffer.reset();
    m_osWindowHDRFramebuffer.reset();
    m_widgetManager.reset();
    developerWindow.reset();
    debugShapeArray.clear();
    debugLabelArray.clear();

    debugPane = NULL;
    debugWindow.reset();
    m_debugController.reset();
    m_debugCamera.reset();
    m_activeCamera.reset();

    NetworkDevice::cleanup();

    debugFont.reset();
    delete userInput;
    userInput = NULL;

    VertexBuffer::cleanupAllVertexBuffers();
    if (!m_hasUserCreatedRenderDevice && m_hasUserCreatedWindow) {
        // Destroy the render device explicitly (rather than waiting for the Window to do so)
        renderDevice->cleanup();
        delete renderDevice;
    }
    renderDevice = NULL;

    if (! m_hasUserCreatedWindow) {
        delete m_window;
        m_window = NULL;
    }
}


int GApp::run() {
    int ret = 0;
    if (catchCommonExceptions) {
        try {
            onRun();
            ret = m_exitCode;
        } catch (const char* e) {
            alwaysAssertM(false, e);
            ret = -1;
        } catch (const Image::Error& e) {
            alwaysAssertM(false, e.reason + "\n" + e.filename);
            ret = -1;
        } catch (const String& s) {
            alwaysAssertM(false, s);
            ret = -1;
        } catch (const TextInput::WrongTokenType& t) {
            alwaysAssertM(false, t.message);
            ret = -1;
        } catch (const TextInput::WrongSymbol& t) {
            alwaysAssertM(false, t.message);
            ret = -1;
        } catch (const LightweightConduit::PacketSizeException& e) {
            alwaysAssertM(false, e.message);
            ret = -1;
        } catch (const ParseError& e) {
            alwaysAssertM(false, e.formatFileInfo() + e.message);
            ret = -1;
        } catch (const FileNotFound& e) {
            alwaysAssertM(false, e.message);
            ret = -1;
        }
    } else {
        onRun();
        ret = m_exitCode;
    }

    return ret;
}


void GApp::onRun() {
    if (window()->requiresMainLoop()) {

        // The window push/pop will take care of
        // calling beginRun/oneFrame/endRun for us.
        window()->pushLoopBody(this);

    } else {
        beginRun();

        debugAssertGLOk();
        // Main loop
        do {
            oneFrame();
        } while (! m_endProgram);

        endRun();
    }
}


void GApp::loadScene(const String& sceneName) {
    // Use immediate mode rendering to force a simple message onto the screen
    drawMessage("Loading " + sceneName + "...");

    const String oldSceneName = scene()->name();

    // Load the scene
    try {
        const Any& any = scene()->load(sceneName);

        // If the debug camera was active and the scene is the same as before, retain the old camera.  Otherwise,
        // switch to the default camera specified by the scene.

        if ((oldSceneName != scene()->name()) || (activeCamera()->name() != "(Debug Camera)")) {

            // Because the CameraControlWindow is hard-coded to the
            // m_debugCamera, we have to copy the camera's values here
            // instead of assigning a pointer to it.
            m_debugCamera->copyParametersFrom(scene()->defaultCamera());
            m_debugController->setFrame(m_debugCamera->frame());

            setActiveCamera(scene()->defaultCamera());
        }

        onAfterLoadScene(any, sceneName);

    } catch (const ParseError& e) {
        const String& msg = e.filename + format(":%d(%d): ", e.line, e.character) + e.message;
        debugPrintf("%s", msg.c_str());
        logPrintf("%s", msg.c_str());
        drawMessage(msg);
        System::sleep(5);
        scene()->clear();
        scene()->lightingEnvironment().ambientOcclusion = m_ambientOcclusion;
    }

    // Trigger one frame of rendering, to force shaders to load and compile
    m_posed3D.fastClear();
    m_posed2D.fastClear();
    if (scene()) {
        onPose(m_posed3D, m_posed2D);
    }
    onGraphics(renderDevice, m_posed3D, m_posed2D);

    // Reset our idea of "now" so that simulation doesn't see a huge lag
    // due to the scene load time.
    m_lastTime = m_now = System::time() - 0.0001f;
}


void GApp::saveScene() {
    // Called when the "save" button is pressed
    if (scene()) {
        Any a = scene()->toAny();
        const String& filename = a.source().filename;
        if (filename != "") {
            a.save(filename);
            debugPrintf("Saved %s\n", filename.c_str());
        } else {
            debugPrintf("Could not save: empty filename");
        }
    }
}


bool GApp::onEvent(const GEvent& event) {
    if (event.type == GEventType::VIDEO_RESIZE) {
        resize(event.resize.w, event.resize.h);
        // Don't consume the resize event--we want subclasses to be able to handle it as well
        return false;
    }

    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::F5)) {

		String oldCaption = window()->caption();
        window()->setCaption(oldCaption + " (Reloading shaders...)");
        Shader::reloadAll();
        window()->setCaption(oldCaption);
        return true;

    } else if (event.type == GEventType::KEY_DOWN && event.key.keysym.sym == GKey::F8) {

        Array<shared_ptr<Texture> > output;
        renderCubeMap(renderDevice, output, m_debugCamera, shared_ptr<Texture>(), 2048);
        drawMessage("Saving Cube Map...");
        const CubeMapConvention::CubeMapInfo& cubeMapInfo = Texture::cubeMapInfo(CubeMapConvention::DIRECTX);
        for (int f = 0; f < 6; ++f) {
            const CubeMapConvention::CubeMapInfo::Face& faceInfo = cubeMapInfo.face[f];
            shared_ptr<Image> temp = Image::fromPixelTransferBuffer(output[f]->toPixelTransferBuffer(ImageFormat::RGB8()));
            temp->rotateCW(toRadians(90.0) * (-faceInfo.rotations));
            if (faceInfo.flipY) { temp->flipVertical();   }
            if (faceInfo.flipX) { temp->flipHorizontal(); }
            temp->save(format("cube-%s.png", faceInfo.suffix.c_str()));
        }
        return true;

    } else if (event.type == GEventType::FILE_DROP) {

        Array<String> fileArray;
        window()->getDroppedFilenames(fileArray);
        const String lowerFilename = toLower(fileArray[0]);
        if (endsWith(lowerFilename, ".scn.any") || endsWith(fileArray[0], ".Scene.Any")) {

            // Load a scene
            loadScene(fileArray[0]);
            return true;

        } else if (endsWith(lowerFilename, ".am.any") ||
            endsWith(fileArray[0], ".ArticulatedModel.Any") ||
            endsWith(fileArray[0], ".MD3Model.Any") ||
            endsWith(fileArray[0], ".MD2Model.Any") ||
            endsWith(fileArray[0], ".Heightfield.Any") ||
            endsWith(lowerFilename, ".3ds") || endsWith(lowerFilename, ".ifs")  ||
            endsWith(lowerFilename, ".obj") || endsWith(lowerFilename, ".ply2") ||
            endsWith(lowerFilename, ".off") || endsWith(lowerFilename, ".ply")  ||
            endsWith(lowerFilename, ".bsp") || endsWith(lowerFilename, ".stl")  ||
            endsWith(lowerFilename, ".lwo") || endsWith(lowerFilename, ".stla") ||
            endsWith(lowerFilename, ".dae") || endsWith(lowerFilename, ".fbx")) {

            // Trace a ray from the drop point
            Model::HitInfo hitInfo;
			scene()->intersect(scene()->eyeRay(activeCamera(), Vector2(event.drop.x + 0.5f, event.drop.y + 0.5f), renderDevice->viewport(), settings().depthGuardBandThickness), ignoreFloat, false, Array<shared_ptr<Entity> >(), hitInfo);

            if (hitInfo.point.isNaN()) {
                // The drop wasn't on a surface, so choose a point in front of the camera at a reasonable distance
                const CFrame& cframe = activeCamera()->frame();
                hitInfo.set(shared_ptr<Model>(), shared_ptr<Entity>(), shared_ptr<Material>(), Vector3::unitY(), cframe.lookVector() * 4 + cframe.translation);
            }

            // Insert a Model
            Any modelAny;

            // If a 3d model file was dropped, generate an ArticulatedModel::Specification
            // using ArticulatedModelDialog
            if (endsWith(lowerFilename, ".3ds") || endsWith(lowerFilename, ".ifs")  ||
                endsWith(lowerFilename, ".obj") || endsWith(lowerFilename, ".ply2") ||
                endsWith(lowerFilename, ".off") || endsWith(lowerFilename, ".ply")  ||
                endsWith(lowerFilename, ".bsp") || endsWith(lowerFilename, ".stl")  ||
                endsWith(lowerFilename, ".lwo") || endsWith(lowerFilename, ".stla") ||
                endsWith(lowerFilename, ".dae") || endsWith(lowerFilename, ".fbx")) {

                shared_ptr<ArticulatedModelSpecificationEditorDialog> amd = ArticulatedModelSpecificationEditorDialog::create(window(), debugWindow->theme());
                ArticulatedModel::Specification spec = ArticulatedModel::Specification();
                spec.filename = fileArray[0];
                spec.scale = 1.0f;
                amd->getSpecification(spec);
                modelAny = spec.toAny();
            } else {
                // Otherwise just load the dropped file
                modelAny.load(fileArray[0]);
                // Make the filename of the model findable, as the name can no longer be relative to the .Any file
                // this means giving the full path if necessary, or the path after G3D10DATA
                String fileName = modelAny.get("filename", modelAny.get("directory", "")).string();

                fileName = FilePath::concat(FilePath::parent(fileArray[0]), fileName);
                static Array<String> dataPaths;
                if (dataPaths.length() == 0) {
                    System::initializeDirectoryArray(dataPaths);
                }
                for (int i = 0; i < dataPaths.size(); ++i) {
                    if (beginsWith(fileName, dataPaths[i])) {
                        fileName = fileName.substr(dataPaths[i].length());
                    }
                }
                fileName = FilePath::canonicalize(fileName);
                if (modelAny.containsKey("filename")) {
                    modelAny["filename"] = fileName;
                } else if (modelAny.containsKey("directory")) {
                    modelAny["directory"] = fileName;
                }
            }
            int nameModifier = 0;
            Array<String> entityNames;
            scene()->getEntityNames(entityNames);
            // creates a unique name in order to avoid conflicts from multiple models being dragged in
            String name = makeValidIndentifierWithUnderscores(FilePath::base(fileArray[0]));

            if (entityNames.contains(name)) {
                while (entityNames.contains(format("%s%d", name.c_str(), ++nameModifier)));
                name = name.append(format("%d", nameModifier));
            }

            const String& newModelName  = format("%s%d", name.c_str(), nameModifier);
            const String& newEntityName = name;

            scene()->createModel(modelAny, newModelName);

            Any entityAny(Any::TABLE, "VisibleEntity");
            // Insert an Entity for that model
            entityAny["frame"] = CFrame(hitInfo.point);
            entityAny["model"] = newModelName;

            scene()->createEntity("VisibleEntity", newEntityName, entityAny);
            return true;
        }
    } else if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == 'g') && (activeCamera() == m_debugCamera)) {
        Model::HitInfo info;
        Vector2 mouse;
        uint8 ignore;
        window()->getRelativeMouseState(mouse, ignore);
        const shared_ptr<Entity>& selection =
            scene()->intersect(scene()->eyeRay(activeCamera(), mouse + Vector2(0.5f, 0.5f), renderDevice->viewport(),
                        settings().depthGuardBandThickness), ignoreFloat, sceneVisualizationSettings().showMarkers, Array<shared_ptr<Entity> >(), info);

        if (notNull(selection)) {
            m_debugCamera->setFrame(CFrame(m_debugCamera->frame().rotation, info.point + (m_debugCamera->frame().rotation * Vector3(0.0f, 0.0f, 1.5f))));
            m_debugController->setFrame(m_debugCamera->frame());
        }
    }

    return false;
}


void GApp::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& allSurfaces) {
    if (! scene()) {
        if ((submitToDisplayMode() == SubmitToDisplayMode::MAXIMIZE_THROUGHPUT) && (! rd->swapBuffersAutomatically())) {
            swapBuffers();
        }
        rd->clear();
        rd->pushState(); {
            rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
            drawDebugShapes();
        } rd->popState();
        return;
    }

    GBuffer::Specification gbufferSpec = m_gbufferSpecification;
    extendGBufferSpecification(gbufferSpec);
    m_gbuffer->setSpecification(gbufferSpec);

    m_gbuffer->resize(m_framebuffer->width(), m_framebuffer->height());
    m_gbuffer->prepare(rd, activeCamera(), 0, -(float)previousSimTimeStep(), m_settings.depthGuardBandThickness, m_settings.colorGuardBandThickness);

    m_renderer->render(rd, m_framebuffer, scene()->lightingEnvironment().ambientOcclusionSettings.enabled ? m_depthPeelFramebuffer : nullptr, 
        scene()->lightingEnvironment(), m_gbuffer, allSurfaces);

    // Debug visualizations and post-process effects
    rd->pushState(m_framebuffer); {
        // Call to make the App show the output of debugDraw(...)
        rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
        drawDebugShapes();
        const shared_ptr<Entity>& selectedEntity = (notNull(developerWindow) && notNull(developerWindow->sceneEditorWindow)) ? developerWindow->sceneEditorWindow->selectedEntity() : nullptr;
        scene()->visualize(rd, selectedEntity, allSurfaces, sceneVisualizationSettings(), activeCamera());

        // Post-process special effects
        m_depthOfField->apply(rd, m_framebuffer->texture(0), m_framebuffer->texture(Framebuffer::DEPTH), activeCamera(), m_settings.depthGuardBandThickness - m_settings.colorGuardBandThickness);

        m_motionBlur->apply(rd, m_framebuffer->texture(0), m_gbuffer->texture(GBuffer::Field::SS_EXPRESSIVE_MOTION),
                            m_framebuffer->texture(Framebuffer::DEPTH), activeCamera(),
                            m_settings.depthGuardBandThickness - m_settings.colorGuardBandThickness);
    } rd->popState();

    // We're about to render to the actual back buffer, so swap the buffers now.
    // This call also allows the screenshot and video recording to capture the
    // previous frame just before it is displayed.
    if (submitToDisplayMode() == SubmitToDisplayMode::MAXIMIZE_THROUGHPUT) {
        swapBuffers();
    }

	// Clear the entire screen (needed even though we'll render over it, since
    // AFR uses clear() to detect that the buffer is not re-used.)
    rd->clear();

    // Perform gamma correction, bloom, and SSAA, and write to the native window frame buffer
    m_film->exposeAndRender(rd, activeCamera()->filmSettings(), m_framebuffer->texture(0));
}


void GApp::onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D> >& posed2D) {
    Surface2D::sortAndRender(rd, posed2D);
}


void GApp::onGraphics(RenderDevice* rd, Array<shared_ptr<Surface> >& posed3D, Array<shared_ptr<Surface2D> >& posed2D) {
    rd->pushState(); {
        debugAssert(notNull(activeCamera()));
        rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
        onGraphics3D(rd, posed3D);
    } rd->popState();

    rd->push2D(); {
        onGraphics2D(rd, posed2D);
    } rd->pop2D();
}


void GApp::addWidget(const shared_ptr<Widget>& module, bool setFocus) {
    m_widgetManager->add(module);

    if (setFocus) {
        m_widgetManager->setFocusedWidget(module);
    }
}


void GApp::removeWidget(const shared_ptr<Widget>& module) {
    m_widgetManager->remove(module);
}


void GApp::resize(int w, int h) {

    // ensure a minimum size before guard band
    w = max(8, w);
    h = max(8, h);
    if (notNull(developerWindow)) {
        const Vector2 developerBounds = developerWindow->bounds().wh();
        developerWindow->setRect(Rect2D::xywh(Point2(float(w), float(h)) - developerBounds, developerBounds));
    }

    // Add the color guard band
    w += m_settings.depthGuardBandThickness.x * 2;
    h += m_settings.depthGuardBandThickness.y * 2;

    // Does the m_osWindowHDRFramebuffer need to be reallocated?  Do this even if we
    // aren't using it at the moment, but not if we are minimized.
    shared_ptr<Texture> color0(m_osWindowHDRFramebuffer->texture(0));

    if (notNull(m_film) && ! window()->isIconified() &&
        (isNull(color0) ||
        (m_osWindowHDRFramebuffer->width() != w) ||
        (m_osWindowHDRFramebuffer->height() != h))) {

        m_osWindowHDRFramebuffer->clear();

        const ImageFormat* colorFormat = GLCaps::firstSupportedTexture(m_settings.film.preferredColorFormats);
        const ImageFormat* depthFormat = GLCaps::firstSupportedTexture(m_settings.film.preferredDepthFormats);
        const bool generateMipMaps = false;
        m_osWindowHDRFramebuffer->set(Framebuffer::COLOR0, Texture::createEmpty("G3D::GApp::m_osWindowHDRFramebuffer/color", w, h, colorFormat, Texture::DIM_2D, generateMipMaps, 1));

        if (notNull(depthFormat)) {
            // Prefer creating a texture if we can

            const Framebuffer::AttachmentPoint p = (depthFormat->stencilBits > 0) ? Framebuffer::DEPTH_AND_STENCIL : Framebuffer::DEPTH;
            alwaysAssertM(GLCaps::supportsTexture(depthFormat), "");

            // Most applications will reset this to be bound to the GBuffer's depth buffer
            m_osWindowHDRFramebuffer->set(p, Texture::createEmpty("G3D::GApp::m_osWindowHDRFramebuffer/depth", w, h, depthFormat, Texture::DIM_2D, generateMipMaps, 1));

            m_depthPeelFramebuffer  = Framebuffer::create(Texture::createEmpty("G3D::GApp::m_depthPeelFramebuffer", m_osWindowHDRFramebuffer->width(), m_osWindowHDRFramebuffer->height(),
                                         depthFormat, Texture::DIM_2D));
        }
    }
}


void GApp::oneFrame() {
    for (int repeat = 0; repeat < max(1, m_renderPeriod); ++repeat) {
		Profiler::nextFrame();
        m_lastTime = m_now;
        m_now = System::time();
        RealTime timeStep = m_now - m_lastTime;

        // User input
        m_userInputWatch.tick();
        if (manageUserInput) {
            processGEventQueue();
        }
        onAfterEvents();
        onUserInput(userInput);
        m_userInputWatch.tock();

        // Network
        m_networkWatch.tick();
        onNetwork();
        m_networkWatch.tock();

        // Logic
        m_logicWatch.tick();
        {
            onAI();
        }
        m_logicWatch.tock();

        // Simulation
        m_simulationWatch.tick();
        {
            RealTime rdt = timeStep;

            SimTime sdt = m_simTimeStep;
            if (sdt == MATCH_REAL_TIME_TARGET) {
                sdt = m_wallClockTargetDuration;
            } else if (sdt == REAL_TIME) {
                sdt = float(timeStep);
            }
            sdt *= m_simTimeScale;

            SimTime idt = m_wallClockTargetDuration;

            onBeforeSimulation(rdt, sdt, idt);
            onSimulation(rdt, sdt, idt);
            onAfterSimulation(rdt, sdt, idt);

            if (m_cameraManipulator) {
                m_debugCamera->setFrame(m_cameraManipulator->frame());
            }

            m_previousSimTimeStep = float(sdt);
            m_previousRealTimeStep = float(rdt);
            setRealTime(realTime() + rdt);
            setSimTime(simTime() + sdt);
        }
        m_simulationWatch.tock();
    }


    // Pose
    m_poseWatch.tick(); {
        m_posed3D.fastClear();
        m_posed2D.fastClear();
        onPose(m_posed3D, m_posed2D);
    } m_poseWatch.tock();

    // Wait
    // Note: we might end up spending all of our time inside of
    // RenderDevice::beginFrame.  Waiting here isn't double waiting,
    // though, because while we're sleeping the CPU the GPU is working
    // to catch up.

    m_waitWatch.tick(); {
        RealTime nowAfterLoop = System::time();

        // Compute accumulated time
        RealTime cumulativeTime = nowAfterLoop - m_lastWaitTime;

        // Perform wait for actual time needed
        RealTime duration = m_wallClockTargetDuration;
        if (! window()->hasFocus() && m_lowerFrameRateInBackground) {
            // Lower frame rate
            duration = 1.0 / BACKGROUND_FRAME_RATE;
        }
        RealTime desiredWaitTime = max(0.0, duration - cumulativeTime);
        onWait(max(0.0, desiredWaitTime - m_lastFrameOverWait) * 0.97);

        // Update wait timers
        m_lastWaitTime = System::time();
        RealTime actualWaitTime = m_lastWaitTime - nowAfterLoop;

        // Learn how much onWait appears to overshoot by and compensate
        double thisOverWait = actualWaitTime - desiredWaitTime;
        if (abs(thisOverWait - m_lastFrameOverWait) / max(abs(m_lastFrameOverWait), abs(thisOverWait)) > 0.4) {
            // Abruptly change our estimate
            m_lastFrameOverWait = thisOverWait;
        } else {
            // Smoothly change our estimate
            m_lastFrameOverWait = lerp(m_lastFrameOverWait, thisOverWait, 0.1);
        }
    }  m_waitWatch.tock();


    // Graphics
    debugAssertGLOk();
    if ((submitToDisplayMode() == SubmitToDisplayMode::BALANCE) && (! renderDevice->swapBuffersAutomatically())) {
        swapBuffers();
    }
    renderDevice->beginFrame();
    m_graphicsWatch.tick(); {
        debugAssertGLOk();
        renderDevice->pushState(); {
            debugAssertGLOk();
            onGraphics(renderDevice, m_posed3D, m_posed2D);
        } renderDevice->popState();
    }  m_graphicsWatch.tock();
    renderDevice->endFrame();
    if ((submitToDisplayMode() == SubmitToDisplayMode::MINIMIZE_LATENCY) && (!renderDevice->swapBuffersAutomatically())) {
        swapBuffers();
    }

    // Remove all expired debug shapes
    for (int i = 0; i < debugShapeArray.size(); ++i) {
        if (debugShapeArray[i].endTime <= m_now) {
            debugShapeArray.fastRemove(i);
            --i;
        }
    }

    for (int i = 0; i < debugLabelArray.size(); ++i) {
        if (debugLabelArray[i].endTime <= m_now) {
            debugLabelArray.fastRemove(i);
            --i;
        }
    }

    debugText.fastClear();

    if (m_endProgram && window()->requiresMainLoop()) {
        window()->popLoopBody();
    }
}


void GApp::swapBuffers() {
    if (m_activeVideoRecordDialog) {
        m_activeVideoRecordDialog->maybeRecord(renderDevice);
    }
	renderDevice->swapBuffers();
}


void GApp::drawDebugShapes() {
    renderDevice->setObjectToWorldMatrix(CFrame());

    if (debugShapeArray.size() > 0) {

        renderDevice->setPolygonOffset(-1.0f);
        for (int i = 0; i < debugShapeArray.size(); ++i) {
            const DebugShape& s = debugShapeArray[i];
            s.shape->render(renderDevice, s.frame, s.solidColor, s.wireColor);
        }
        renderDevice->setPolygonOffset(0.0f);
    }

    if (debugLabelArray.size() > 0) {
        for (int i = 0; i < debugLabelArray.size(); ++i) {
            const DebugLabel& label = debugLabelArray[i];
            if (label.text.text() != "") {
                static const shared_ptr<GFont> defaultFont = GFont::fromFile(System::findDataFile("arial.fnt"));
                const shared_ptr<GFont> f = label.text.element(0).font(defaultFont);
                f->draw3DBillboard(renderDevice, label.text, label.wsPos, label.size, label.text.element(0).color(Color3::black()), Color4::clear(), label.xalign, label.yalign);
            }
        }
    }
}


void GApp::removeAllDebugShapes() {
    debugShapeArray.fastClear();
    debugLabelArray.fastClear();
}


void GApp::removeDebugShape(DebugID id) {
    for (int i = 0; i < debugShapeArray.size(); ++i) {
        if (debugShapeArray[i].id == id) {
            debugShapeArray.fastRemove(i);
            return;
        }
    }
}


void GApp::onWait(RealTime t) {
    System::sleep(max(0.0, t));
}


void GApp::setRealTime(RealTime r) {
    m_realTime = r;
}


void GApp::setSimTime(SimTime s) {
    m_simTime = s;
}


void GApp::onInit() {
    debugAssert(notNull(m_ambientOcclusion));
    setScene(Scene::create(m_ambientOcclusion));
}


void GApp::onCleanup() {}


void GApp::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    if (notNull(m_cameraManipulator)) { m_cameraManipulator->setEnabled(activeCamera() == m_debugCamera); }

    m_widgetManager->onSimulation(rdt, sdt, idt);

    if (scene()) { scene()->onSimulation(sdt); }

    // Update the camera's previous frame.  The debug camera
    // is usually controlled by the camera manipulator and is
    // a copy of one from a scene, but is not itself in the scene, so it needs an explicit
    // simulation call here.
    m_debugCamera->onSimulation(0, idt);
}


void GApp::onBeforeSimulation(RealTime& rdt, SimTime& sdt, SimTime& idt) {
    (void)idt;
    (void)rdt;
    (void)sdt;
}


void GApp::onAfterSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    (void)idt;
    (void)rdt;
    (void)sdt;
}


void GApp::onPose(Array<shared_ptr<Surface> >& surface, Array<shared_ptr<Surface2D> >& surface2D) {
    m_widgetManager->onPose(surface, surface2D);

    if (scene()) {
        scene()->onPose(surface);
    }
}


void GApp::onNetwork() {
    m_widgetManager->onNetwork();
}


void GApp::onAI() {
    m_widgetManager->onAI();
}


void GApp::beginRun() {

    m_endProgram = false;
    m_exitCode = 0;

    onInit();

    // Move the controller to the camera's location
    if (m_cameraManipulator) {
        m_cameraManipulator->setFrame(m_debugCamera->frame());
    }

    m_now = System::time() - 0.001;
}


void GApp::endRun() {
    onCleanup();

    Log::common()->section("Files Used");

    Array<String> fileArray;
    FileSystem::usedFiles().getMembers(fileArray);

    // Canonicalize file names
    for (int i = 0; i < fileArray.size(); ++i) {
        fileArray[i] = FilePath::canonicalize(FileSystem::resolve(fileArray[i]));
    }

    // Alphabetize
    fileArray.sort();

    // Print
    for (int i = 0; i < fileArray.size(); ++i) {
        Log::common()->println(fileArray[i]);
    }
    Log::common()->println("");

    if (window()->requiresMainLoop() && m_endProgram) {
        ::exit(m_exitCode);
    }
}


void GApp::staticConsoleCallback(const String& command, void* me) {
    ((GApp*)me)->onConsoleCommand(command);
}


void GApp::onConsoleCommand(const String& cmd) {
    if (trimWhitespace(cmd) == "exit") {
        setExitCode(0);
        return;
    }
}


void GApp::onUserInput(UserInput* userInput) {
    m_widgetManager->onUserInput(userInput);
}


void GApp::processGEventQueue() {
    userInput->beginEvents();

    // Event handling
    GEvent event;
    while (window()->pollEvent(event)) {
        bool eventConsumed = false;

        // For event debugging
        //if (event.type != GEventType::MOUSE_MOTION) {
        //    printf("%s\n", event.toString().c_str());
        //}

        eventConsumed = WidgetManager::onEvent(event, m_widgetManager);

        if (! eventConsumed) {

            eventConsumed = onEvent(event);

            if (! eventConsumed) {
                switch(event.type) {
                case GEventType::QUIT:
                    setExitCode(0);
                    break;

                case GEventType::KEY_DOWN:

                    if (isNull(console) || ! console->active()) {
                        switch (event.key.keysym.sym) {
                        case GKey::ESCAPE:
                            switch (escapeKeyAction) {
                            case ACTION_QUIT:
                                setExitCode(0);
                                break;

                            case ACTION_SHOW_CONSOLE:
                                console->setActive(true);
                                eventConsumed = true;
                                break;

                            case ACTION_NONE:
                                break;
                            }
                            break;

                        // Add other key handlers here
                        default:;
                        }
                    }
                    break;

                // Add other event handlers here

                default:;
                }
            } // consumed
        } // consumed


        // userInput sees events if they are not consumed, or if they are release events
        if (! eventConsumed || (event.type == GEventType::MOUSE_BUTTON_UP) || (event.type == GEventType::KEY_UP)) {
            // userInput always gets to process events, so that it
            // maintains the current state correctly.
            userInput->processEvent(event);
        }
    }

    userInput->endEvents();
}


void GApp::onAfterEvents() {
    m_widgetManager->onAfterEvents();
}


void GApp::setActiveCamera(const shared_ptr<Camera>& camera) {
    m_activeCamera = camera;
}

void GApp::extendGBufferSpecification(GBuffer::Specification& spec) {
    if (notNull(scene())) {
        scene()->lightingEnvironment().ambientOcclusionSettings.extendGBufferSpecification(spec);
        activeCamera()->motionBlurSettings().extendGBufferSpecification(spec);
        activeCamera()->depthOfFieldSettings().extendGBufferSpecification(spec);
        activeCamera()->filmSettings().extendGBufferSpecification(spec);
    }
}

void GApp::renderCubeMap(RenderDevice* rd, Array<shared_ptr<Texture> >& output, const shared_ptr<Camera>& camera, const shared_ptr<Texture>& depthMap, int resolution) {

    Array<shared_ptr<Surface> > surface;
    {
        Array<shared_ptr<Surface2D> > ignore;
        onPose(surface, ignore);
    }
    const int oldFramebufferWidth       = m_osWindowHDRFramebuffer->width();
    const int oldFramebufferHeight      = m_osWindowHDRFramebuffer->height();
    const Vector2int16  oldColorGuard   = m_settings.colorGuardBandThickness;
    const Vector2int16  oldDepthGuard   = m_settings.depthGuardBandThickness;
    const shared_ptr<Camera>& oldCamera = activeCamera();

    m_settings.colorGuardBandThickness = Vector2int16(128, 128);
    m_settings.depthGuardBandThickness = Vector2int16(256, 256);
    const int fullWidth = resolution + (2 * m_settings.depthGuardBandThickness.x);
    m_osWindowHDRFramebuffer->resize(fullWidth, fullWidth);

    shared_ptr<Camera> newCamera = Camera::create("Cubemap Camera");
    newCamera->copyParametersFrom(camera);
    newCamera->depthOfFieldSettings().setEnabled(false);
    newCamera->motionBlurSettings().setEnabled(false);
    newCamera->setFieldOfView(2.0f * ::atan(1.0f + 2.0f*(float(m_settings.depthGuardBandThickness.x) / float(resolution)) ), FOVDirection::HORIZONTAL);

    const ImageFormat* imageFormat = ImageFormat::RGB16F();
    if ((output.size() == 0) || isNull(output[0])) {
        // allocate cube maps
        output.resize(6);
        for (int face = 0; face < 6; ++face) {
            output[face] = Texture::createEmpty(CubeFace(face).toString(), resolution, resolution, imageFormat, Texture::DIM_2D, false);
        }
    }

    // Configure the base camera
    CFrame cframe = newCamera->frame();

    setActiveCamera(newCamera);
    for (int face = 0; face < 6; ++face) {
        Texture::getCubeMapRotation(CubeFace(face), cframe.rotation);
        newCamera->setFrame(cframe);

        rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
        onGraphics3D(rd, surface);
        // render every face twice to let the screen space reflection/refraction texture to stabilize
        onGraphics3D(rd, surface);

        m_film->exposeAndRender(rd, activeCamera()->filmSettings(), m_osWindowHDRFramebuffer->texture(0), output[face]);
    }
    setActiveCamera(oldCamera);
    m_osWindowHDRFramebuffer->resize(oldFramebufferWidth, oldFramebufferHeight);
    m_settings.colorGuardBandThickness = oldColorGuard;
    m_settings.depthGuardBandThickness = oldDepthGuard;
}
}
