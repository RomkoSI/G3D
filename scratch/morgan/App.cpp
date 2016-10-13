/** \file App.cpp */
#include "App.h"
#include <regex>

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

void tungstenToG3D() {
    const String sourceFilename = "C:/Users/morgan/Desktop/living-room-tungsten/scene.json";
    Any any;
    any.load(sourceFilename);
}


int main(int argc, const char* argv[]) {
    {
        G3DSpecification g3dSpec;
        g3dSpec.audio = false;
        initGLG3D(g3dSpec);
    }

    GApp::Settings settings(argc, argv);

    // Change the window and other startup parameters by modifying the
    // settings class.  For example:
    settings.window.caption             = argv[0];
    // settings.window.debugContext     = true;

    // settings.window.width            =  854; settings.window.height       = 480;
    // settings.window.width            = 1024; settings.window.height       = 768;
     settings.window.width            = 1280; settings.window.height       = 720;
    //settings.window.width               = 1920; settings.window.height       = 1080;
    // settings.window.width            = OSWindow::primaryDisplayWindowSize().x; settings.window.height = OSWindow::primaryDisplayWindowSize().y;
    settings.window.fullScreen          = false;
    settings.window.resizable           = ! settings.window.fullScreen;
    settings.window.framed              = ! settings.window.fullScreen;

    // Set to true for a significant performance boost if your app can't render at 60fps,
    // or if you *want* to render faster than the display.
    settings.window.asynchronous        = false;

    settings.hdrFramebuffer.depthGuardBandThickness = Vector2int16(64, 64);
    settings.hdrFramebuffer.colorGuardBandThickness = Vector2int16(0, 0);
    settings.dataDir                    = FileSystem::currentDirectory();
//    settings.screenshotDirectory        = "../journal/";

    settings.renderer.deferredShading = true;
    settings.renderer.orderIndependentTransparency = true;

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {
}


// Called before the application loop begins.  Load data here and
// not in the constructor so that common exceptions will be
// automatically caught.
void App::onInit() {
    GApp::onInit();
    setFrameDuration(1.0f / 120.0f);

    if (false) {
        ArticulatedModel::Specification s;
        XML xml;
        xml.load("C:/Users/morgan/Desktop/living-room-mitsuba/scene.xml");
        ArticulatedModel::Specification::mitsubaToG3D(xml, s);
        s.toAny().save("result.ArticulatedModel.Any");
    }

    /*
    const String filename = System::findDataFile("image/testimage.png");
    shared_ptr<Texture> texture = Texture::fromFile(filename, ImageFormat::SRGB8(), Texture::DIM_2D, false);
    shared_ptr<Image3>  image3  = texture->toImage3();
    shared_ptr<Image>   image   = Image::fromFile(filename);

    for (int x = 0; x < image3->width(); ++x) {
        Color3 c3 = image3->get(x, 0);
        Color3 c  = image->get<Color3>(x, 0);
        debugPrintf("%f %f %f        %f %f %f\n", c3.r, c3.g, c3.b,   c.r, c.g, c.b);
    }
    */

    
    showRenderingStats      = true;
    makeGUI();
    // For higher-quality screenshots:
    // developerWindow->videoRecordDialog->setScreenShotFormat("PNG");
    // developerWindow->videoRecordDialog->setCaptureGui(false);
    developerWindow->cameraControlWindow->moveTo(Point2(developerWindow->cameraControlWindow->rect().x0(), 0));
    loadScene(
        //"Feature Test"
        //"G3D Sponza"
        "G3D Sponza (White)"
        //developerWindow->sceneEditorWindow->selectedSceneName()  // Load the first scene encountered 
        );

    if (false) {
        labelFont = GFont::fromFile(System::findDataFile("arial.fnt"));
        {
            float pdfValue;
            const Vector3 n = Vector3::unitY();
            w_o = Vector3(1, 1, 0).direction();
            w_mirror = w_o.reflectAbout(n);
            pdf = std::make_shared<DirectionHistogram>(100, w_mirror);
            const float m = 100;
            for (int i = 0; i < 1000000; ++i) {
                Vector3 w_i;
                Vector3::cosHemiPlusCosPowHemiHemiRandom(w_mirror, n, m, 0.1f, Random::threadCommon(), w_i, pdfValue);
                pdf->insert(w_i);
            }
        }
    }

    m_triTree.setContents(scene());

    Stopwatch timer;
    const int w = 640, h = 400;
    const Rect2D viewport = Rect2D::xywh(0.0f, 0.0f, float(w), float(h));
    const shared_ptr<Camera>& camera = activeCamera();
    Array<Ray> rayBuffer;
    Array<shared_ptr<Surfel>> surfelBuffer;
    rayBuffer.resize(w * h);
    surfelBuffer.resize(rayBuffer.size());

    timer.tick();
    Thread::runConcurrently(Point2int32(0, 0), Point2int32(w, h), [&](Point2int32 P) {
        rayBuffer[P.x + P.y * w] = camera->worldRay(P.x + 0.5f, P.y + 0.5f, viewport);
    });
    timer.tock();

    debugPrintf("Generate %d rays: %f ms\n", rayBuffer.size(), timer.elapsedTime() / units::milliseconds());
    Array<TriTree::Hit> hitBuffer;
    hitBuffer.resize(rayBuffer.size());

    timer.tick();
    m_triTree.intersectRays(rayBuffer, hitBuffer, TriTreeBase::COHERENT_RAY_HINT);
    timer.tock();
    debugPrintf("Cast primary rays: %f ms\n", timer.elapsedTime() / units::milliseconds());

    // Preallocate the UniversalSurfels
    Thread::runConcurrently(0, hitBuffer.size(), [&](int i) {
        m_triTree.sample(hitBuffer[i], surfelBuffer[i]);
    });

    timer.tick();
    const TriTree::Hit* pHit = hitBuffer.getCArray();
    shared_ptr<Surfel>* pSurfel = surfelBuffer.getCArray();
            m_triTree.sample(pHit[0], pSurfel[0]);

	tbb::parallel_for(tbb::blocked_range<size_t>(0, hitBuffer.size(), 128), [&](const tbb::blocked_range<size_t>& r) {
		const size_t start = r.begin();
		const size_t end   = r.end();
		for (size_t i = start; i < end; ++i) {
            m_triTree.sample(pHit[i], pSurfel[i]);
        }
    });
    timer.tock();
    debugPrintf("Construct surfels: %f ms\n", timer.elapsedTime() / units::milliseconds());
}


void App::makeGUI() {
    // Initialize the developer HUD (using the existing scene)
    createDeveloperHUD();
    debugWindow->setVisible(true);
    developerWindow->videoRecordDialog->setEnabled(true);


    // More examples of debugging GUI controls:
    // debugPane->addCheckBox("Use explicit checking", &explicitCheck);
    // debugPane->addTextBox("Name", &myName);
    // debugPane->addNumberBox("height", &height, "m", GuiTheme::LINEAR_SLIDER, 1.0f, 2.5f);
    // button = debugPane->addButton("Run Simulator");
    // debugPane->addButton("Generate Heightfield", this, &App::generateHeightfield);
    // debugPane->addButton("Generate Heightfield", [this](){ makeHeightfield(imageName, scale, "model/heightfield.off"); });

    debugWindow->pack();
    debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));
}


void App::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& allSurfaces) {
    if (!scene()) {
        if ((submitToDisplayMode() == SubmitToDisplayMode::MAXIMIZE_THROUGHPUT) && (!rd->swapBuffersAutomatically())) {
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
    m_gbuffer->prepare(rd, activeCamera(), 0, -(float)previousSimTimeStep(), m_settings.hdrFramebuffer.depthGuardBandThickness, m_settings.hdrFramebuffer.colorGuardBandThickness);

    m_renderer->render(rd, m_framebuffer, scene()->lightingEnvironment().ambientOcclusionSettings.enabled ? m_depthPeelFramebuffer : shared_ptr<Framebuffer>(),
        scene()->lightingEnvironment(), m_gbuffer, allSurfaces);

    // Debug visualizations and post-process effects
    rd->pushState(m_framebuffer); {
        // Call to make the App show the output of debugDraw(...)
        rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
        drawDebugShapes();
        const shared_ptr<Entity>& selectedEntity = (notNull(developerWindow) && notNull(developerWindow->sceneEditorWindow)) ? developerWindow->sceneEditorWindow->selectedEntity() : shared_ptr<Entity>();
        scene()->visualize(rd, selectedEntity, allSurfaces, sceneVisualizationSettings(), activeCamera());

        if (pdf) {
            Draw::arrow(w_o * 2.0f, w_o * 0.25f, rd, Color3::orange(), 1.0f);
            Draw::arrow(Point3::zero(), w_mirror * 2.0f, rd, Color3::blue(), 1.0f);
            Draw::plane(Plane(Vector3::unitY(), Point3::zero()), rd, Color3::white() * 0.5f, Color3::white() * 0.2f);
            labelFont->draw3DBillboard(rd, "pdf", Point3::unitY() * 2.0f, 0.3f);
            pdf->render(rd, Color3(0.5f, 1.0f, 1.0f) * 0.5f, Color3::white() * 0.9f);
        }

        // Post-process special effects
        m_depthOfField->apply(rd, m_framebuffer->texture(0), m_framebuffer->texture(Framebuffer::DEPTH), activeCamera(), m_settings.hdrFramebuffer.depthGuardBandThickness - m_settings.hdrFramebuffer.colorGuardBandThickness);

        m_motionBlur->apply(rd, m_framebuffer->texture(0), m_gbuffer->texture(GBuffer::Field::SS_EXPRESSIVE_MOTION),
            m_framebuffer->texture(Framebuffer::DEPTH), activeCamera(),
            m_settings.hdrFramebuffer.depthGuardBandThickness - m_settings.hdrFramebuffer.colorGuardBandThickness);
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
    m_film->exposeAndRender(rd, activeCamera()->filmSettings(), m_framebuffer->texture(0), settings().hdrFramebuffer.colorGuardBandThickness.x + settings().hdrFramebuffer.depthGuardBandThickness.x, settings().hdrFramebuffer.depthGuardBandThickness.x);
}


void App::onAI() {
    GApp::onAI();
    // Add non-simulation game logic and AI code here
}


void App::onNetwork() {
    GApp::onNetwork();
    // Poll net messages here
}


void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    GApp::onSimulation(rdt, sdt, idt);

    // Example GUI dynamic layout code.  Resize the debugWindow to fill
    // the screen horizontally.
    debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));
}


bool App::onEvent(const GEvent& event) {
    // Handle super-class events
    if (GApp::onEvent(event)) { return true; }

    // If you need to track individual UI events, manage them here.
    // Return true if you want to prevent other parts of the system
    // from observing this specific event.
    //
    // For example,
    // if ((event.type == GEventType::GUI_ACTION) && (event.gui.control == m_button)) { ... return true; }
    // if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::TAB)) { ... return true; }
    // if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == 'p')) { ... return true; }

    return false;
}


void App::onUserInput(UserInput* ui) {
    GApp::onUserInput(ui);
    (void)ui;
    // Add key handling here based on the keys currently held or
    // ones that changed in the last frame.
}


void App::onPose(Array<shared_ptr<Surface> >& surface, Array<shared_ptr<Surface2D> >& surface2D) {
    GApp::onPose(surface, surface2D);

    // Append any models to the arrays that you want to later be rendered by onGraphics()
}


void App::onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D> >& posed2D) {
    // Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction.
    Surface2D::sortAndRender(rd, posed2D);
}


void App::onCleanup() {
    // Called after the application loop ends.  Place a majority of cleanup code
    // here instead of in the constructor so that exceptions can be caught.
}


void App::endProgram() {
    m_endProgram = true;
}
