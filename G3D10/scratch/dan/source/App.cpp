/** \file App.cpp */
#include "App.h"

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
    {
        G3DSpecification g3dSpec;
        g3dSpec.audio = false;
        initGLG3D(g3dSpec);
    }

    GApp::Settings settings(argc, argv);

    // Change the window and other startup parameters by modifying the
    // settings class.  For example:
    settings.window.caption = argv[0];
    // settings.window.debugContext     = true;

    // settings.window.width              =  854; settings.window.height       = 480;
    // settings.window.width            = 1024; settings.window.height       = 768;
    settings.window.width = 1280; settings.window.height = 720;
    //    settings.window.width               = 1920; settings.window.height       = 1080;
    // settings.window.width            = OSWindow::primaryDisplayWindowSize().x; settings.window.height = OSWindow::primaryDisplayWindowSize().y;
    settings.window.fullScreen = false;
    settings.window.resizable = !settings.window.fullScreen;
    settings.window.framed = !settings.window.fullScreen;

    // Set to true for a significant performance boost if your app can't render at 60fps,
    // or if you *want* to render faster than the display.
    settings.window.asynchronous = false;

    settings.depthGuardBandThickness = Vector2int16(64, 64);
    settings.colorGuardBandThickness = Vector2int16(16, 16);
    settings.dataDir = "../scratch/dan/data-files";
    chdir(settings.dataDir.c_str());
    settings.screenshotDirectory = "";

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
    
    // Call setScene(shared_ptr<Scene>()) or setScene(MyScene::create()) to replace
    // the default scene here.

    showRenderingStats = true;

    testStruct.color = m_framebuffer->texture(Framebuffer::COLOR0);
    testStruct.scale = 2.0f;
    testStruct.bounds = Vector2(512, 512);

    makeGUI();
    // For higher-quality screenshots:
    // developerWindow->videoRecordDialog->setScreenShotFormat("PNG");
    // developerWindow->videoRecordDialog->setCaptureGui(false);
    developerWindow->cameraControlWindow->moveTo(Point2(developerWindow->cameraControlWindow->rect().x0(), 0));
    loadScene(
        //"G3D Sponza"
        "G3D Cornell Box" // Load something simple
                          //developerWindow->sceneEditorWindow->selectedSceneName()  // Load the first scene encountered 
        );    
    //shared_ptr<Image> im = Image::create(512, 512, ImageFormat::RGB32I());
    //for (int x = 0; x < 512; ++x) {
    //    for (int y = 0; y < 512; ++y) {
    //       im->set<Color4>(x, y, Color4(2,2,2,2));
    //}
    //}
    shared_ptr<Image> uffiziImage = Image::fromFile(System::findDataFile("uffizi-large.exr"));
    static shared_ptr<Texture> uffiziTexture = Texture::fromImage("uffiziTexture", uffiziImage);
    shared_ptr<Image> im = Image::create(1024, 1024, ImageFormat::RGB32F());
    shared_ptr<Image> output[6];
    static shared_ptr<Texture> textures[6];

    shared_ptr<Camera> conversion = Camera::create("conversionCamera");
    conversion->setFieldOfViewAngleDegrees(90.0f);
    conversion->setFrame(CoordinateFrame::fromXYZYPRDegrees(0, 0, 0));
    CFrame frame = conversion->frame();
    Rect2D rect = Rect2D::xywh(0, 0, 1024, 1024);

    const CubeMapConvention::CubeMapInfo& cubeMapInfo = Texture::cubeMapInfo(CubeMapConvention::DIRECTX);

    for (int face = 0; face < 6; ++face) {
        output[face] = Image::create(1024, 1024, ImageFormat::RGB32F());
        Texture::getCubeMapRotation(CubeFace(face), frame.rotation);
        conversion->setFrame(frame);

        for (int y = 0; y < output[face]->height(); ++y) {
            for (int x = 0; x < output[face]->width(); ++x) {
                const Vector3& direction = conversion->worldRay(x + 0.5f, y + 0.5f, output[face]->bounds()).direction();
                //direction = frame.rotation * direction;
                const Vector2 texCoords((1.0f + atan2(direction.x, -direction.z) / pif()) / 2.0f * uffiziImage->width(), aCos(direction.y) / pif() * uffiziImage->height());
                output[face]->set(x, y, uffiziImage->bicubic(texCoords.x, texCoords.y).rgb());
            }
        }

        const CubeMapConvention::CubeMapInfo::Face& faceInfo = cubeMapInfo.face[face];
        //why is this not saving what is in the images?
        textures[face] = Texture::fromImage(format("face%d", face), output[face]);
        output[face]->rotateCW(-toRadians(90.0f) * faceInfo.rotations);
        if (faceInfo.flipY) { output[face]->flipVertical(); }
        if (faceInfo.flipX) { output[face]->flipHorizontal(); }
        output[face]->save(format("uffizi-%s.exr", faceInfo.suffix.c_str()));
    }
}


void App::makeGUI() {
    // Initialize the developer HUD (using the existing scene)
    createDeveloperHUD();
    debugWindow->setVisible(true);
    developerWindow->videoRecordDialog->setEnabled(true);

    GuiPane* infoPane = debugPane->addPane("Info", GuiTheme::ORNATE_PANE_STYLE);

    // Example of how to add debugging controls
    infoPane->addLabel("You can add GUI controls");
    infoPane->addLabel("in App::onInit().");
    infoPane->addButton("Exit", this, &App::endProgram);
    infoPane->pack();

    // More examples of debugging GUI controls:
    // debugPane->addCheckBox("Use explicit checking", &explicitCheck);
    // debugPane->addTextBox("Name", &myName);
    // debugPane->addNumberBox("height", &height, "m", GuiTheme::LINEAR_SLIDER, 1.0f, 2.5f);
    // button = debugPane->addButton("Run Simulator");

    debugWindow->pack();
    debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));
}


void App::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& allSurfaces) {
    // This implementation is equivalent to the default GApp's. It is repeated here to make it
    // easy to modify rendering. If you don't require custom rendering, just delete this
    // method from your application and rely on the base class.

    if (!scene()) {
        return;
    }


    m_gbuffer->setSpecification(m_gbufferSpecification);
    m_gbuffer->resize(m_framebuffer->width(), m_framebuffer->height());
    m_gbuffer->prepare(rd, activeCamera(), 0, -(float)previousSimTimeStep(), m_settings.depthGuardBandThickness, m_settings.colorGuardBandThickness);
   
    m_renderer->render(rd, m_framebuffer, m_depthPeelFramebuffer, scene()->lightingEnvironment(), m_gbuffer, allSurfaces);

    // Debug visualizations and post-process effects
    rd->pushState(m_framebuffer); {
        // Call to make the App show the output of debugDraw(...)
        rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
        drawDebugShapes();
        //Draw::points(points, rd, Color4(1.0f, 1.0f, 1.0f, 1.0f));
        //dir.render(rd,Color3::white(),Color3::black());
        const shared_ptr<Entity>& selectedEntity = (notNull(developerWindow) && notNull(developerWindow->sceneEditorWindow)) ? developerWindow->sceneEditorWindow->selectedEntity() : shared_ptr<Entity>();
        scene()->visualize(rd, selectedEntity, allSurfaces, sceneVisualizationSettings());

        // Post-process special effects
        m_depthOfField->apply(rd, m_framebuffer->texture(0), m_framebuffer->texture(Framebuffer::DEPTH), activeCamera(), m_settings.depthGuardBandThickness - m_settings.colorGuardBandThickness);

        m_motionBlur->apply(rd, m_framebuffer->texture(0), m_gbuffer->texture(GBuffer::Field::SS_EXPRESSIVE_MOTION),
            m_framebuffer->texture(Framebuffer::DEPTH), activeCamera(),
            m_settings.depthGuardBandThickness - m_settings.colorGuardBandThickness);
    } rd->popState();

    if ((submitToDisplayMode() == SubmitToDisplayMode::MAXIMIZE_THROUGHPUT) && (!renderDevice->swapBuffersAutomatically())) {
        // We're about to render to the actual back buffer, so swap the buffers now.
        // This call also allows the screenshot and video recording to capture the
        // previous frame just before it is displayed.
        swapBuffers();
    }

    // Clear the entire screen (needed even though we'll render over it, since
    // AFR uses clear() to detect that the buffer is not re-used.)
    rd->clear();
    testStruct.color = Texture::fromFile(System::findDataFile("image\\checker-32x32-1024x1024.png"));
    static shared_ptr<Texture> tex = Texture::createEmpty("testStruct", testStruct.color->width(), testStruct.color->height());
    static shared_ptr<Framebuffer> buffer = Framebuffer::create(tex);
    rd->push2D(buffer); {
        Args args;
        args.setUniform("inputStruct.color", testStruct.color, Sampler::buffer());
        args.setUniform("inputStruct.scale", testStruct.scale);
        args.setUniform("inputStruct.bounds", testStruct.bounds);
        args.setRect(testStruct.color->rect2DBounds());
        LAUNCH_SHADER("struct.pix", args);
    } rd->pop2D();

    // Perform gamma correction, bloom, and SSAA, and write to the native window frame buffer
    m_film->exposeAndRender(rd, activeCamera()->filmSettings(), m_framebuffer->texture(0));

    static shared_ptr<Texture> colorBlit = Texture::createEmpty("blitColor",m_framebuffer->width(), m_framebuffer->height());
    static shared_ptr<Texture> depthBlit = Texture::createEmpty("blitDepth", m_framebuffer->width(), m_framebuffer->height(), ImageFormat::DEPTH32F());
    static shared_ptr<Framebuffer> bufferBlit = Framebuffer::create(colorBlit, depthBlit);
    m_framebuffer->blitTo(rd, bufferBlit, false, false, true, false, true);

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
