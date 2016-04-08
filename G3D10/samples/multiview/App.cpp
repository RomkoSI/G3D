#include "App.h"

G3D_START_AT_MAIN();

int main(int argc, const char** argv) {
    GApp::Settings settings(argc, argv);

    settings.window.caption = "G3D MultiView Demo";
    settings.window.width       = 1280; 
    settings.window.height      = 720;

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {
    catchCommonExceptions = false;
}


void App::onInit() {
    createDeveloperHUD();
	renderDevice->setSwapBuffersAutomatically(true);
    renderDevice->setColorClearValue(Color3::white());
    debugWindow->setVisible(false);
    developerWindow->cameraControlWindow->setVisible(true);
    developerWindow->cameraControlWindow->moveTo(Vector2(developerWindow->cameraControlWindow->rect().x0(), 0));
    developerWindow->setVisible(false);
    showRenderingStats = false;
    
    m_debugCamera->setFrame(CFrame::fromXYZYPRDegrees(-0.61369f, 0.734589f, 0.934322f, 314.163f, -12.1352f));
    m_debugCamera->filmSettings().setVignetteBottomStrength(0);
    m_debugCamera->filmSettings().setVignetteTopStrength(0);

    m_scene = BuildingScene::create();

    m_debugCamera->filmSettings().setAntialiasingEnabled(false);
    
    shared_ptr<GuiTheme> theme = debugWindow->theme();

    // Example of how to create windows
    shared_ptr<GuiWindow> toolBar = GuiWindow::create("Tools", theme, Rect2D::xywh(0,0,0,0), GuiTheme::TOOL_WINDOW_STYLE);

    shared_ptr<IconSet> icons = IconSet::fromFile(System::findDataFile("tango.icn"));
    GuiPane* toolPane = toolBar->pane();

    toolPane->addButton(icons->get("22x22/uwe/CreateCylinder.png"), GuiTheme::TOOL_BUTTON_STYLE);
    toolPane->addButton(icons->get("22x22/uwe/CreateBox.png"), GuiTheme::TOOL_BUTTON_STYLE);
    toolPane->addButton(icons->get("22x22/uwe/Emitter.png"), GuiTheme::TOOL_BUTTON_STYLE);
    toolPane->addButton(icons->get("22x22/uwe/PointLight.png"), GuiTheme::TOOL_BUTTON_STYLE)->moveBy(Vector2(10,0));
    toolPane->addButton(icons->get("22x22/categories/applications-multimedia.png"), GuiTheme::TOOL_BUTTON_STYLE);
    toolPane->addButton(icons->get("22x22/categories/applications-graphics.png"), GuiTheme::TOOL_BUTTON_STYLE);
    toolPane->addButton(icons->get("22x22/categories/applications-system.png"), GuiTheme::TOOL_BUTTON_STYLE);
    toolBar->pack();
    addWidget(toolBar);
}


void App::onPose(Array<shared_ptr<Surface> >& surfaceArray, Array<shared_ptr<Surface2D> >& surface2D) {
    GApp::onPose(surfaceArray, surface2D);

    // Append any models to the arrays that you want to later be rendered by onGraphics()
    if (m_scene) {
        m_scene->onPose(surfaceArray);
    }
}


void App::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface3D) { 
    // Bind the main framebuffer
    rd->pushState(m_framebuffer); {
        rd->clear();

        LightingEnvironment env = m_scene->lighting();
        env.ambientOcclusion = m_ambientOcclusion;
        env.ambientOcclusionSettings.useNormalBuffer = false;

        // Render full shading viewport
        const Rect2D& shadeViewport = Rect2D::xywh(0, 0, rd->width() / 2.0f, rd->height() / 2.0f);
        rd->setViewport(shadeViewport);

        Draw::skyBox(rd, env.environmentMapArray[0]);

        Surface::renderDepthOnly(rd, surface3D, CullFace::BACK);

        m_ambientOcclusion->update(rd, env.ambientOcclusionSettings, activeCamera(), m_framebuffer->texture(Framebuffer::DEPTH));

        Array<shared_ptr<Surface> > sortedVisible;
        Surface::cull(activeCamera()->frame(), activeCamera()->projection(), rd->viewport(), surface3D, sortedVisible);
        Surface::sortBackToFront(sortedVisible, activeCamera()->frame().lookVector());
        rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
        for (int i = 0; i < sortedVisible.size(); ++i) {
            sortedVisible[i]->render(rd, env, RenderPassType::OPAQUE_SAMPLES, "");
        }

        // Wireframe views
        shared_ptr<Camera> wireCamera[3] = {Camera::create(), Camera::create(), Camera::create()};
        wireCamera[0]->setFrame(CFrame::fromXYZYPRDegrees(0,40,0,0,-90));
        wireCamera[1]->setFrame(CFrame::fromXYZYPRDegrees(0,0,40,0,0));
        wireCamera[2]->setFrame(CFrame::fromXYZYPRDegrees(40,0,0,90,0));

        Rect2D wireViewport[3];
        wireViewport[0] = shadeViewport + Vector2(rd->width() / 2.0f, 0.0f);
        wireViewport[1] = shadeViewport + Vector2(rd->width() / 2.0f, rd->height() / 2.0f);
        wireViewport[2] = shadeViewport + Vector2(0.0f, rd->height() / 2.0f);

        for (int i = 0; i < 3; ++i) {
            rd->setViewport(wireViewport[i]);
            rd->setProjectionAndCameraMatrix(wireCamera[i]->projection(), wireCamera[i]->frame());

            Surface::renderWireframe(rd, surface3D);
            Draw::axes(rd);

            // Call to make the GApp show the output of debugDraw calls
            drawDebugShapes();
        }

    } rd->popState();

    // Perform gamma correction, bloom, and SSAA, and write to the native window frame buffer
    m_film->exposeAndRender(rd, activeCamera()->filmSettings(), m_framebuffer->texture(0));
}


void App::onGraphics2D(RenderDevice* rd, Array<Surface2DRef>& posed2D) { 
    SlowMesh slowMesh(PrimitiveType::LINES);
	slowMesh.setColor(Color3::black()); {        
        slowMesh.makeVertex(Point2(rd->width() / 2.0f, 0.0f));
        slowMesh.makeVertex(Point2(rd->width() / 2.0f, (float)rd->height())); 
        slowMesh.makeVertex(Point2(0.0f, rd->height() / 2.0f));
        slowMesh.makeVertex(Point2((float)rd->width(), rd->height() / 2.0f));
    } slowMesh.render(rd);

    // Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction
    Surface2D::sortAndRender(rd, posed2D);
}
