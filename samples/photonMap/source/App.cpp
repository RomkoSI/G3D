/** \file App.cpp */
#include "App.h"

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

// Global pointer to the app
App* app = NULL;

int main(int argc, const char* argv[]) {
    initGLG3D();

    GApp::Settings settings(argc, argv);
    
    // Change the window and other startup parameters by modifying the
    // settings class.  For example:
    settings.window.width       = 1280; 
    settings.window.height      = 720;
    settings.window.caption     = "Ray tracer with photon mapping";
    settings.colorGuardBandThickness  = Vector2int16(0, 0);
    settings.depthGuardBandThickness  = Vector2int16(0, 0);

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings), m_lastSceneUpdateTime(0) {
}


void App::onResolutionChange() {
    TextInput ti(TextInput::FROM_STRING,
                 m_resolutionList->selectedValue().text());

    m_rayTracerSettings.width = (int)ti.readNumber();
    ti.readSymbol("x");
    m_rayTracerSettings.height = (int)ti.readNumber();
}


void App::onRenderButton() {
    const shared_ptr<Image> image =
        m_rayTracer->render(m_rayTracerSettings, scene()->lightingEnvironment(), activeCamera(), m_rayTracerStats);

    // Upload to the GPU
    const shared_ptr<Texture> srcTexture = 
        Texture::fromPixelTransferBuffer("Rendered", image->toPixelTransferBuffer(), NULL, Texture::DIM_2D);
    
    shared_ptr<Texture> dstTexture;

    // Apply Film exposure
    m_film->exposeAndRender(renderDevice, activeCamera()->filmSettings(), srcTexture, dstTexture);

    // Display
    show(dstTexture);
}


void App::onInit() {
    GApp::onInit();
	renderDevice->setSwapBuffersAutomatically(true);

    ::app = this;

    // Called before the application loop begins.  Load data here and
    // not in the constructor so that common exceptions will be
    // automatically caught.


    showRenderingStats    = false;
    m_showWireframe       = false;
    m_showPhotons         = false;
    m_showPhotonMap       = false;

    m_rayTracer = RayTracer::create(scene());
    makeGUI();
    onResolutionChange();

    loadScene(developerWindow->sceneEditorWindow->selectedSceneName());
}


void App::makeGUI() {
    createDeveloperHUD();

    // Turn on the developer HUD
    debugWindow->setVisible(true);
    developerWindow->cameraControlWindow->setVisible(true);
    developerWindow->videoRecordDialog->setEnabled(true);
    developerWindow->cameraControlWindow->moveTo(Point2(950, 120));

    // For higher-quality screenshots:
    developerWindow->videoRecordDialog->setScreenShotFormat("PNG");
    // developerWindow->videoRecordDialog->setCaptureGui(false);
    
    GuiPane* rtPane = debugPane->addPane("Ray Trace", GuiTheme::ORNATE_PANE_STYLE);

    m_resolutionList = rtPane->addDropDownList
        ("Resolution", Array<String>("16 x 9", "160 x 90", "320 x 180", "640 x 360", "1280 x 720"),
         NULL, GuiControl::Callback(this, &App::onResolutionChange));
    m_resolutionList->setSelectedValue("640 x 360");

    rtPane->addNumberBox("Primary Rays", &m_rayTracerSettings.sqrtNumPrimaryRays, "\xB2", GuiTheme::LINEAR_SLIDER, 1, 8);
    rtPane->addNumberBox("Bounces", &m_rayTracerSettings.numBackwardBounces, "", GuiTheme::LINEAR_SLIDER, 1, 10);
    rtPane->addCheckBox("Check Final Visibility", &m_rayTracerSettings.checkFinalVisibility);
    {
        GuiControl* c = rtPane->addButton("Render", this, &App::onRenderButton);
        c->setSize(210, 28);
    }
    rtPane->pack();

    GuiPane* pmPane = debugPane->addPane("Photon Map", GuiTheme::ORNATE_PANE_STYLE);
    pmPane->moveRightOf(rtPane, 10);
#   define ARRANGE(type, exp) {GuiNumberBox<type>* nb = (exp); nb->setWidth(250); nb->setCaptionWidth(80); } 
    ARRANGE(int, pmPane->addNumberBox("# Emitted", &m_rayTracerSettings.photon.numEmitted, "", GuiTheme::LOG_SLIDER, 0, 10000000));
    ARRANGE(int, pmPane->addNumberBox("Bounces", &m_rayTracerSettings.photon.numBounces, "", GuiTheme::LINEAR_SLIDER, 1, 10));
    ARRANGE(float, pmPane->addNumberBox("Min Radius", &m_rayTracerSettings.photon.minGatherRadius, "m", GuiTheme::LOG_SLIDER, 0.001f, 2.0f, 0.001f));
    ARRANGE(float, pmPane->addNumberBox("Max Radius", &m_rayTracerSettings.photon.maxGatherRadius, "m", GuiTheme::LOG_SLIDER, 0.001f, 2.0f, 0.001f));
    ARRANGE(float, pmPane->addNumberBox("Broadening Rate", &m_rayTracerSettings.photon.radiusBroadeningRate, "", GuiTheme::LINEAR_SLIDER, 0.01f, 1.0f, 0.01f));
#   undef ARRANGE
    pmPane->pack();


    GuiPane* statsPane = debugPane->addPane("Scene Statistics", GuiTheme::ORNATE_PANE_STYLE);
    statsPane->moveRightOf(pmPane, 10);

#   define ARRANGE(exp) {GuiNumberBox<int>* inb = (exp); inb->setCaptionWidth(100); inb->setEnabled(false); }
    ARRANGE( statsPane->addNumberBox("Triangles",    &m_rayTracerStats.triangles));
    ARRANGE( statsPane->addNumberBox("Lights",       &m_rayTracerStats.lights));
    ARRANGE( statsPane->addNumberBox("Pixels",       &m_rayTracerStats.pixels));
    ARRANGE( statsPane->addNumberBox("Stored Photons",&m_rayTracerStats.storedPhotons));
#   undef ARRANGE

    statsPane->pack();

    GuiPane* timePane = debugPane->addPane("Time", GuiTheme::ORNATE_PANE_STYLE);
    timePane->moveRightOf(statsPane); timePane->moveBy(10, 0);
    timePane->addNumberBox("Tree Build",   &m_rayTracerStats.buildTriTreeTimeMilliseconds, "ms", GuiTheme::NO_SLIDER, 0.0f, finf(), 0.1f)->setEnabled(false);
    timePane->addNumberBox("Photon Trace", &m_rayTracerStats.photonTraceTimeMilliseconds,  "ms", GuiTheme::NO_SLIDER, 0.0f, finf(), 0.1f)->setEnabled(false);
    timePane->addNumberBox("Map Build",    &m_rayTracerStats.buildPhotonMapTimeMilliseconds, "ms", GuiTheme::NO_SLIDER, 0.0f, finf(), 0.1f)->setEnabled(false);
    timePane->addNumberBox("Ray Trace",    &m_rayTracerStats.rayTraceTimeMilliseconds,     "ms", GuiTheme::NO_SLIDER, 0.0f, finf(), 0.1f)->setEnabled(false);
    timePane->pack();

    GuiPane* dbPane = debugPane->addPane("Debug", GuiTheme::ORNATE_PANE_STYLE);
    dbPane->moveRightOf(timePane, 10);
    dbPane->beginRow(); {
        dbPane->addCheckBox(format("Multithreaded (%dx)", System::numCores()), &m_rayTracerSettings.multithreaded);
        dbPane->addCheckBox("Use Tree",      &m_rayTracerSettings.useTree);
    } dbPane->endRow();
    dbPane->beginRow(); {
        dbPane->addCheckBox("Show Wireframe", &m_showWireframe);
        dbPane->addCheckBox("Show Photons", &m_showPhotons);
    } dbPane->endRow();
    dbPane->addCheckBox("Show Photon Map", &m_showPhotonMap);

    dbPane->pack();

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

    if (! scene()) {
        return;
    }

    m_gbuffer->setSpecification(m_gbufferSpecification);
    m_gbuffer->resize(m_framebuffer->width(), m_framebuffer->height());
    m_gbuffer->prepare(rd, activeCamera(), 0, -(float)previousSimTimeStep(), m_settings.depthGuardBandThickness, m_settings.colorGuardBandThickness);

    m_renderer->render(rd, m_framebuffer, m_depthPeelFramebuffer, scene()->lightingEnvironment(), m_gbuffer, allSurfaces);

    // Debug visualizations and post-process effects
    rd->pushState(m_framebuffer); {
        // Call to make the App show the output of debugDraw(...)
        drawDebugShapes();
        const shared_ptr<Entity>& selectedEntity = (notNull(developerWindow) && notNull(developerWindow->sceneEditorWindow)) ? developerWindow->sceneEditorWindow->selectedEntity() : shared_ptr<Entity>();
        scene()->visualize(rd, selectedEntity, allSurfaces, sceneVisualizationSettings(), activeCamera());

        if (m_showPhotons) {
            rd->setPointSize(4);
            m_rayTracer->debugDrawPhotons(rd);
        }

        if (m_showPhotonMap) {
            m_rayTracer->debugDrawPhotonMap(rd);
        }

        // Post-process special effects
        m_depthOfField->apply(rd, m_framebuffer->texture(0), m_framebuffer->texture(Framebuffer::DEPTH), activeCamera(), m_settings.depthGuardBandThickness - m_settings.colorGuardBandThickness);
        
        m_motionBlur->apply(rd, m_framebuffer->texture(0), m_gbuffer->texture(GBuffer::Field::SS_EXPRESSIVE_MOTION), 
                            m_framebuffer->texture(Framebuffer::DEPTH), activeCamera(), 
                            m_settings.depthGuardBandThickness - m_settings.colorGuardBandThickness);
    } rd->popState();

    // We're about to render to the actual back buffer, so swap the buffers now.
    // This call also allows the screenshot and video recording to capture the
    // previous frame just before it is displayed.
    swapBuffers();

	// Clear the entire screen (needed even though we'll render over it, since
    // AFR uses clear() to detect that the buffer is not re-used.)
    rd->clear();

    // Perform gamma correction, bloom, and SSAA, and write to the native window frame buffer
    m_film->exposeAndRender(rd, activeCamera()->filmSettings(), m_framebuffer->texture(0));


}
