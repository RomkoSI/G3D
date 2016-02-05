#include "App.h"

static const bool DEVELOPER_MODE = false;

G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
    GApp::Settings settings(argc, argv);
    
    settings.window.width       = 1280; 
    settings.window.height      = 720;
    settings.window.resizable   = true;
    settings.window.caption     = "Deep G-Buffer Radiosity";
    settings.colorGuardBandThickness = Vector2int16(128, 128);
    settings.depthGuardBandThickness = Vector2int16(128, 128);
    
#   ifdef G3D_WINDOWS
        // On Unix operating systems, icompile automatically copies data files.  
        // On Windows, we just run from the data directory.
        if (FileSystem::exists("data-files")) {
            chdir("data-files");
        } else if (FileSystem::exists("../samples/deepGbufferRadiosity/data-files")) {
            chdir("../samples/deepGbufferRadiosity/data-files");
        }    
#   endif
    
    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {}


void App::initGBuffers() {
    m_gbufferSpecification.encoding[GBuffer::Field::CS_NORMAL] = Texture::Encoding(ImageFormat::RGB10A2(), FrameName::CAMERA, 2.0f, -1.0f);

    // For motion blur. To improve performance, remove this and just use SS_POSITION_CHANGE
    m_gbufferSpecification.encoding[GBuffer::Field::SS_EXPRESSIVE_MOTION]    = 
        Texture::Encoding(GLCaps::supportsTexture(ImageFormat::RG8()) ? ImageFormat::RG8() : ImageFormat::RGBA8(),
        FrameName::SCREEN, 128.0f, -64.0f);

    m_gbufferSpecification.encoding[GBuffer::Field::SS_POSITION_CHANGE].format = ImageFormat::RG16F();

    // To improve performance on scenes without emissive objects, remove this
    m_gbufferSpecification.encoding[GBuffer::Field::EMISSIVE]           =  
        GLCaps::supportsTexture(ImageFormat::RGB5()) ? 
            Texture::Encoding(ImageFormat::RGB5(), FrameName::NONE, 2.0f, 0.0f) : 
            Texture::Encoding(ImageFormat::R11G11B10F());

    m_gbufferSpecification.encoding[GBuffer::Field::LAMBERTIAN]         = ImageFormat::RGB8();
    m_gbufferSpecification.encoding[GBuffer::Field::GLOSSY]             = ImageFormat::RGBA8();
    m_gbufferSpecification.encoding[GBuffer::Field::DEPTH_AND_STENCIL]  = ImageFormat::DEPTH32F();
    m_gbufferSpecification.depthEncoding = DepthEncoding::HYPERBOLIC;

    // Update the actual m_gbuffer 
    m_gbuffer->setSpecification(m_gbufferSpecification);
    m_gbuffer->resize(renderDevice->width() + m_settings.depthGuardBandThickness.x * 2, renderDevice->height() + m_settings.depthGuardBandThickness.y * 2);

    m_peeledGBufferSpecification = m_gbufferSpecification;
    // The second layer only requires normals, Lambertian, and depth
    m_peeledGBufferSpecification.encoding[GBuffer::Field::SS_EXPRESSIVE_MOTION]  = NULL;
    m_peeledGBufferSpecification.encoding[GBuffer::Field::SS_POSITION_CHANGE]    = NULL;
    m_peeledGBufferSpecification.encoding[GBuffer::Field::EMISSIVE]              = NULL;
    m_peeledGBufferSpecification.encoding[GBuffer::Field::GLOSSY]                = NULL;
    m_peeledGBuffer = GBuffer::create(m_peeledGBufferSpecification, "PeeledGBuffer");
    
    m_peeledGBuffer->resize(m_gbuffer->width(), m_gbuffer->height());
}


void App::onInit() {
    initGBuffers();
    const ImageFormat* lambertianDirectFormat = ImageFormat::RGB16F();
    m_lambertianDirectBuffer = Framebuffer::create(Texture::createEmpty("App::m_lambertianDirectBuffer/Color0", m_gbuffer->width(), m_gbuffer->height(), 
                                         lambertianDirectFormat, Texture::DIM_2D));
    m_peeledLambertianDirectBuffer = Framebuffer::create(Texture::createEmpty("App::m_peeledLambertianDirectBuffer/Color0", m_gbuffer->width(), m_gbuffer->height(), 
                                         lambertianDirectFormat, Texture::DIM_2D));

    m_deepGBufferRadiosity = DeepGBufferRadiosity::create();

    renderDevice->setSwapBuffersAutomatically(false);

    setScene(Scene::create(m_ambientOcclusion));
    makeAdvancedGUI();
    makeGUI();
    Profiler::setEnabled(true);
    loadScene("Sponza (Statue)");
}


void App::renderLambertianOnly
   (RenderDevice*                   rd, 
    const shared_ptr<Framebuffer>&  fb, 
    const LightingEnvironment&      environment, 
    const shared_ptr<GBuffer>&      gbuffer,  
    const DeepGBufferRadiositySettings&      radiositySettings, 
    const shared_ptr<Texture>&      ssPositionChange,
    const shared_ptr<Texture>&      indirectBuffer, 
    const shared_ptr<Texture>&      oldDepth, 
    const shared_ptr<Texture>&      peeledIndirectBuffer, 
    const shared_ptr<Texture>&      peeledDepthBuffer) {
    
    fb->texture(0)->resize(rd->width(), rd->height());
    
    rd->push2D(fb); {
        rd->setColorClearValue(Color3::black());
        rd->clear();
        Args args;
        args.setRect(rd->viewport());
        environment.setShaderArgs(args);
        gbuffer->setShaderArgsRead(args, "gbuffer_");

        const Vector3& clipConstant = gbuffer->camera()->projection().reconstructFromDepthClipInfo();
        const Vector4& projConstant = gbuffer->camera()->projection().reconstructFromDepthProjInfo(gbuffer->width(), gbuffer->height());
        args.setUniform("clipInfo", clipConstant);
        args.setUniform("projInfo", projConstant);
        const bool useIndirect = notNull(oldDepth) && (radiositySettings.propagationDamping < 1.0f);
        args.setMacro("USE_INDIRECT", useIndirect);
        if (useIndirect) {
            indirectBuffer->setShaderArgs(args, "previousIndirectRadiosity_", Sampler::video());
            oldDepth->setShaderArgs(args, "previousDepth_", Sampler::video());
            args.setUniform("propagationDamping", radiositySettings.propagationDamping);
            args.setMacro("USE_PEELED_LAYER", false);
            ssPositionChange->setShaderArgs(args, "ssPositionChange_", Sampler::video());
        }
        
        args.setUniform("saturatedLightBoost",      radiositySettings.saturatedBoost);
        args.setUniform("unsaturatedLightBoost",    radiositySettings.unsaturatedBoost);
        LAUNCH_SHADER("lambertianOnly.*",  args);
    } rd->pop2D();
}


void App::makeAdvancedGUI() {
    createDeveloperHUD();

    debugWindow->setVisible(DEVELOPER_MODE);
    developerWindow->setVisible(DEVELOPER_MODE);
    developerWindow->cameraControlWindow->setVisible(DEVELOPER_MODE);
    developerWindow->sceneEditorWindow->setVisible(DEVELOPER_MODE);
    showRenderingStats = false;

    GuiPane* deepGBufferRadiosityPane    = debugPane->addPane("DeepGBufferRadiosity");
    GuiPane* basicSettingsPane  = deepGBufferRadiosityPane->addPane("Basic Settings");
    basicSettingsPane->addCheckBox("Enabled", &m_deepGBufferRadiositySettings.enabled);
    basicSettingsPane->addNumberBox("Samples", &m_deepGBufferRadiositySettings.numSamples, "", GuiTheme::LINEAR_SLIDER, 3, 99);
    basicSettingsPane->addNumberBox("Radius", &m_deepGBufferRadiositySettings.radius, "", GuiTheme::LINEAR_SLIDER, 0.0f, 20.0f);
    basicSettingsPane->addNumberBox("# Iterations", &m_deepGBufferRadiositySettings.numBounces, "", GuiTheme::LINEAR_SLIDER, 1, 3);
    basicSettingsPane->addNumberBox("Bias", &m_deepGBufferRadiositySettings.bias, "", GuiTheme::LINEAR_SLIDER, 0.0f, 0.05f);
    basicSettingsPane->addCheckBox("Use Mipmaps", &m_deepGBufferRadiositySettings.useMipMaps);
    basicSettingsPane->addNumberBox("Min MipLevel", &m_deepGBufferRadiositySettings.minMipLevel, "", GuiTheme::LINEAR_SLIDER, 0, 5);
    basicSettingsPane->addCheckBox("Use Tap Normal", &m_deepGBufferRadiositySettings.useTapNormal);

    basicSettingsPane->pack();
    GuiPane* reconstructionSettingsPane = deepGBufferRadiosityPane->addPane("Spatial Reconstruction Settings");
    reconstructionSettingsPane->moveRightOf(basicSettingsPane, 20.0f);

    reconstructionSettingsPane->addNumberBox("Blur Radius",     &m_deepGBufferRadiositySettings.blurRadius, "", GuiTheme::LINEAR_SLIDER, 0, 20);
    reconstructionSettingsPane->addNumberBox("Blur Step Size",  &m_deepGBufferRadiositySettings.blurStepSize, "", GuiTheme::LINEAR_SLIDER, 1, 4);
    GuiNumberBox<float>* floatBox = reconstructionSettingsPane->addNumberBox("Edge Sharpness",  &m_deepGBufferRadiositySettings.edgeSharpness, "", GuiTheme::LINEAR_SLIDER, 0.0f, 3.0f);
    floatBox->setCaptionWidth(100.0f);
    reconstructionSettingsPane->addCheckBox("Enforce Monotonic Kernel",  &m_deepGBufferRadiositySettings.monotonicallyDecreasingBilateralWeights);
    reconstructionSettingsPane->pack();

    GuiPane* secondLayerSettingsPane = deepGBufferRadiosityPane->addPane("2nd Layer Settings");
    secondLayerSettingsPane->moveRightOf(reconstructionSettingsPane, 20.0f);
    
    secondLayerSettingsPane->addCheckBox("Use 2nd Layer",  &m_deepGBufferRadiositySettings.useDepthPeelBuffer);
    secondLayerSettingsPane->addLabel("2nd Layer Separation");
    secondLayerSettingsPane->addNumberBox("      ",  &m_deepGBufferRadiositySettings.depthPeelSeparationHint, "m", GuiTheme::LINEAR_SLIDER, 0.0f, 2.0f);
    secondLayerSettingsPane->addCheckBox("Compute 2nd Layer Radiosity",  &m_deepGBufferRadiositySettings.computePeeledLayer);
    secondLayerSettingsPane->pack();

    GuiPane* temporalSettingsPane = deepGBufferRadiosityPane->addPane("Temporal Settings");
    temporalSettingsPane->moveRightOf(secondLayerSettingsPane, 20.0f);

    temporalSettingsPane->addCheckBox("Temporally Vary Samples", &m_deepGBufferRadiositySettings.temporallyVarySamples);
    temporalSettingsPane->addLabel("Temporal Alpha (0 is off)");
    temporalSettingsPane->addNumberBox("      ", &m_deepGBufferRadiositySettings.temporalFilterSettings.hysteresis, "", GuiTheme::LINEAR_SLIDER, 0.0f, 1.0f);
    temporalSettingsPane->addLabel("Propagation Damping (1 is no inter-frame propagation)");
    temporalSettingsPane->addNumberBox("      ", &m_deepGBufferRadiositySettings.propagationDamping, "", GuiTheme::LINEAR_SLIDER, 0.0f, 1.0f);
    temporalSettingsPane->pack();
    
    GuiPane* miscSettingsPane = deepGBufferRadiosityPane->addPane("Misc. Settings");
    miscSettingsPane->moveRightOf(temporalSettingsPane, 20.0f);

    miscSettingsPane->addLabel("Unsaturated Boost");
    miscSettingsPane->addNumberBox("      ", &m_deepGBufferRadiositySettings.unsaturatedBoost, "", GuiTheme::LINEAR_SLIDER, 0.0f, 5.0f);
    miscSettingsPane->addLabel("Saturated Boost");
    miscSettingsPane->addNumberBox("      ", &m_deepGBufferRadiositySettings.saturatedBoost, "", GuiTheme::LINEAR_SLIDER, 0.0f, 5.0f);
    miscSettingsPane->addCheckBox("Advanced Settings Mode", &m_demoSettings.advancedSettingsMode);
    miscSettingsPane->addCheckBox("Use Half Precision Color", &m_deepGBufferRadiositySettings.useHalfPrecisionColors);
    miscSettingsPane->addCheckBox("Use Oct16", &m_deepGBufferRadiositySettings.useOct16);
    miscSettingsPane->addLabel("Compute Guard Band Fraction");
    miscSettingsPane->addNumberBox("     ", &m_deepGBufferRadiositySettings.computeGuardBandFraction, "", GuiTheme::LINEAR_SLIDER, 0.0f, 1.0f);
    miscSettingsPane->pack();

    deepGBufferRadiosityPane->pack();

    debugPane->pack();
}


void App::makeGUI() {
    const float GUI_WIDTH = 305;
    m_gui = GuiWindow::create("", debugWindow->theme(), Rect2D::xywh(0, 0, GUI_WIDTH, float(window()->height())), GuiTheme::PANEL_WINDOW_STYLE, GuiWindow::NO_CLOSE);
    GuiPane* pane = m_gui->pane();

    const shared_ptr<GFont>& iconFont = GFont::fromFile(System::findDataFile("icon.fnt"));
    m_leftIcon = GuiText("3", iconFont);
    m_rightIcon = GuiText("4", iconFont);

    m_captionFont = GFont::fromFile(System::findDataFile("arial.fnt"));        
    m_titleFont = GFont::fromFile(System::findDataFile("times.fnt"));        

    pane->addLabel(GuiText("Deep G-Buffer Example", m_titleFont, 25));
    pane->addLabel(GuiText("based on the technical paper", m_titleFont, 11))->moveBy(3, -6);
    pane->addLabel(GuiText("\"Fast Global Illumination Approximations on Deep G-Buffers\" ", m_titleFont, 11))->moveBy(3, -10);
    pane->addLabel(GuiText("by M. Mara, M. McGuire, D. Nowrouzezahrai, and D. Luebke", m_titleFont, 11))->moveBy(3, -10);

	GuiTabPane* modePane = pane->addTabPane((int*)&m_demoSettings.demoMode);
	modePane->moveBy(-10, 0);

	GuiPane* aoPane = modePane->addTab("AO");

	aoPane->addLabel("Deep G-buffers make screen-space ambient")->moveBy(5, 0);
	aoPane->addLabel("occlusion robust to overlapping geometry")->moveBy(5, -8);
	aoPane->addLabel("and changing viewpoint.")->moveBy(5, -8);

	aoPane->addCheckBox("Enable Two-Layer Deep G-Buffer", &m_demoSettings.twoLayerAO)->moveBy(0, 10);

	GuiPane* radiosityPane = modePane->addTab("Radiosity");
	radiosityPane->addLabel("Deep G-buffers make screen space effects stable")->moveBy(5, 0);
	radiosityPane->addLabel("enough to upgrade an AO pass into full indirect")->moveBy(5, -8);
	radiosityPane->addLabel("radiosity lighting at little additional cost.")->moveBy(5, -8);

	GuiPane* variationPane = modePane->addTab("Variations");
	variationPane->addLabel("The AO and Radiosity modes have all useful")->moveBy(5, 0);
	variationPane->addLabel("lighting terms enabled. This panel exposes")->moveBy(5, -8);
	variationPane->addLabel("controls visualizing partial results.")->moveBy(5, -8);

	GuiPane* lightingPane = radiosityPane->addPane("", GuiTheme::NO_PANE_STYLE);
    lightingPane->moveBy(0, 10);
    lightingPane->addRadioButton("Deep G-Buffer Radiosity", DemoSettings::GlobalIlluminationMode::RADIOSITY,  &m_demoSettings.globalIlluminationMode);
    lightingPane->beginRow(); {
        const float h = 16.0f;
        const float w = 75.0f; 
        const float fontSize = 10;
        GuiLabel* label = lightingPane->addLabel(GuiText("Preset:", shared_ptr<GFont>(), fontSize + 1));
        label->moveBy(22, 0);
        label->setWidth(42);
        lightingPane->addRadioButton(GuiText("Performance", shared_ptr<GFont>(), fontSize), DemoSettings::QualityPreset::MAX_PERFORMANCE, &m_demoSettings.QualityPreset, GuiTheme::TOOL_RADIO_BUTTON_STYLE)->setSize(w, h);
        lightingPane->addRadioButton(GuiText("Balanced", shared_ptr<GFont>(), fontSize),    DemoSettings::QualityPreset::BALANCED,          &m_demoSettings.QualityPreset, GuiTheme::TOOL_RADIO_BUTTON_STYLE)->setSize(w, h);
        lightingPane->addRadioButton(GuiText("Quality", shared_ptr<GFont>(), fontSize),     DemoSettings::QualityPreset::MAX_QUALITY,     &m_demoSettings.QualityPreset, GuiTheme::TOOL_RADIO_BUTTON_STYLE)->setSize(w, h);
        label->moveBy(0, -5);
    } lightingPane->endRow();
    
    lightingPane->addRadioButton("Prerendered Light Probe", DemoSettings::GlobalIlluminationMode::STATIC_LIGHT_PROBE,   &m_demoSettings.globalIlluminationMode)->moveBy(0, 1);
    lightingPane->addRadioButton("Split Screen Comparison", DemoSettings::GlobalIlluminationMode::SPLIT_SCREEN,         &m_demoSettings.globalIlluminationMode);

	lightingPane->addCheckBox("Animated Light Rig", &m_demoSettings.dynamicLights)->moveBy(0, 15);

    lightingPane->pack();
    lightingPane->setWidth(GUI_WIDTH);

	radiosityPane->pack();
	modePane->pack();

	variationPane->addCheckBox("Enable Two-Layer Deep G-Buffer", &m_demoSettings.twoLayerRadiosity)->moveBy(0, 10);
    variationPane->addCheckBox("Ambient Obscurance", &m_demoSettings.aoEnabled);
    variationPane->addCheckBox("Light Probe Fallback", &m_demoSettings.lightProbeFallback);
	variationPane->addCheckBox("Animated Light Rig", &m_demoSettings.dynamicLights);
	variationPane->pack();

    GuiPane* cameraPane = pane->addPane("Camera", GuiTheme::SIMPLE_PANE_STYLE);
	cameraPane->moveBy(0, 5);
    cameraPane->addRadioButton("Static",  DemoSettings::CameraMode::STATIC,   &m_demoSettings.cameraMode);
    cameraPane->addRadioButton("Dynamic", DemoSettings::CameraMode::DYNAMIC,  &m_demoSettings.cameraMode);
    cameraPane->addRadioButton("Manual ", DemoSettings::CameraMode::FREE,     &m_demoSettings.cameraMode);

    const shared_ptr<Texture>& legend = Texture::fromFile(System::findDataFile("keyguide-small.png"));
    m_controlLabel = cameraPane->addLabel(GuiText(legend));
    m_controlLabel->moveBy(25, 0);

    cameraPane->pack();
    cameraPane->setWidth(GUI_WIDTH);

    m_performancePane = pane->addPane("Performance", GuiTheme::SIMPLE_PANE_STYLE);
    m_performancePane->addLabel(GLCaps::renderer());
    m_resolutionLabel = m_performancePane->addLabel(format("%d x %d pixels", window()->width(), window()->height()));
    m_resolutionLabel->moveBy(0, -3);
    m_performancePane->beginRow(); {
        m_performancePane->addLabel("Radiosity:")->setWidth(65);
        m_radiosityTimeLabel = m_performancePane->addLabel("??.??");
        m_radiosityTimeLabel->setWidth(35);
        m_radiosityTimeLabel->setXAlign(GFont::XALIGN_RIGHT);
        m_performancePane->addLabel("ms")->moveBy(5, 0);
    } m_performancePane->endRow();
    m_performancePane->beginRow(); {
        GuiControl* c = m_performancePane->addLabel("Filtering:");
        c->setWidth(65);
        c->moveBy(0, -8);
        m_filteringTimeLabel = m_performancePane->addLabel("??.??");
        m_filteringTimeLabel->setWidth(35);
        m_filteringTimeLabel->setXAlign(GFont::XALIGN_RIGHT);
        m_performancePane->addLabel("ms")->moveBy(5, 0);
    } m_performancePane->endRow();
    m_performancePane->pack();
    m_performancePane->setWidth(GUI_WIDTH);

    m_drawerButton = pane->addButton(m_leftIcon, GuiTheme::TOOL_BUTTON_STYLE);
    m_drawerButton->setSize(12, 18);
    m_drawerButton->setPosition(GUI_WIDTH - m_drawerButton->rect().width() - 2, (m_gui->rect().height() - m_drawerButton->rect().height()) / 2);

    m_gui->setRect(Rect2D::xywh(0, 0, GUI_WIDTH, float(window()->height())));

    addWidget(m_gui);
}


void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    GApp::onSimulation(rdt, sdt, idt);
    m_resolutionLabel->setCaption(format("%d x %d pixels", window()->width(), window()->height()));
    const Rect2D& oldRect = m_gui->rect();
    if (oldRect.height() != float(window()->height())) {
        m_gui->setRect(Rect2D::xywh(oldRect.x0(), oldRect.y0(), oldRect.width(), float(window()->height())));
    }

	// Update profiler GUI
    if (((m_demoSettings.globalIlluminationMode == DemoSettings::GlobalIlluminationMode::RADIOSITY) ||
         (m_demoSettings.globalIlluminationMode == DemoSettings::GlobalIlluminationMode::SPLIT_SCREEN)) && 
		 Profiler::enabled()) {

        Array<const Array<Profiler::Event>*> eventTreeArray;
        Profiler::getEvents(eventTreeArray);
        alwaysAssertM(eventTreeArray.size() > 0, "No profiler events on any thread");
        const Array<Profiler::Event>& eventTree = *(eventTreeArray[0]);

        m_radiosityTimeLabel->setCaption(format("%4.1f", eventTree.find("DeepGBufferRadiosity_DeepGBufferRadiosity.*")->gfxDuration() / (1 * units::milliseconds())));
        m_filteringTimeLabel->setCaption(format("%4.1f", eventTree.find("Reconstruction Filter")->gfxDuration() / (1 * units::milliseconds())));
    }
}


void App::computeGBuffers(RenderDevice* rd, Array<shared_ptr<Surface> >& all) {
    BEGIN_PROFILER_EVENT("App::computeGBuffers");

    m_gbuffer->prepare(rd, activeCamera(), 0, -(float)previousSimTimeStep(), m_settings.depthGuardBandThickness, m_settings.colorGuardBandThickness);
    m_peeledGBuffer->prepare(rd, activeCamera(), 0, -(float)previousSimTimeStep(), m_settings.depthGuardBandThickness, m_settings.colorGuardBandThickness);

    Array<shared_ptr<Surface> > sortedVisible;
    Surface::cull(activeCamera()->frame(), activeCamera()->projection(), rd->viewport(), all, sortedVisible);
    Surface::sortFrontToBack(sortedVisible, activeCamera()->frame().lookVector());
    glDisable(GL_DEPTH_CLAMP);

    Surface::renderIntoGBuffer(rd, sortedVisible, m_gbuffer, activeCamera()->previousFrame(), activeCamera()->expressivePreviousFrame());
    Surface::renderIntoGBuffer(rd, sortedVisible, m_peeledGBuffer, activeCamera()->previousFrame(), activeCamera()->expressivePreviousFrame(), m_gbuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL), m_deepGBufferRadiositySettings.depthPeelSeparationHint);
    END_PROFILER_EVENT();
}


void App::computeShadows(RenderDevice* rd, Array<shared_ptr<Surface> >& all, LightingEnvironment& environment) { 
    BEGIN_PROFILER_EVENT("App::computeShadows");
    environment = scene()->lightingEnvironment();

    m_ambientOcclusion->update(rd, environment.ambientOcclusionSettings, activeCamera(), m_framebuffer->texture(Framebuffer::DEPTH), m_depthPeelFramebuffer->texture(Framebuffer::DEPTH), m_gbuffer->texture(GBuffer::Field::CS_NORMAL), m_gbuffer->texture(GBuffer::Field::SS_POSITION_CHANGE),
        m_settings.depthGuardBandThickness - m_settings.colorGuardBandThickness);
    environment.ambientOcclusion = m_ambientOcclusion;

    static RealTime lastLightingChangeTime = 0;
    RealTime lightingChangeTime = max(scene()->lastEditingTime(), max(scene()->lastLightChangeTime(), scene()->lastVisibleChangeTime()));
    if (lightingChangeTime > lastLightingChangeTime) {
        lastLightingChangeTime = lightingChangeTime;
        Surface::renderShadowMaps(rd, environment.lightArray, all);
    }
    END_PROFILER_EVENT();
}


void App::deferredShade(RenderDevice* rd, const LightingEnvironment& environment) {
    BEGIN_PROFILER_EVENT("App::deferredShade");
    // Make a pass over the screen, performing shading
    rd->push2D(); {
        rd->setGuardBandClip2D(m_settings.colorGuardBandThickness);

        // Don't shade the skybox on this pass because it will be forward rendered
        rd->setDepthTest(RenderDevice::DEPTH_GREATER);
        Args args;

        environment.setShaderArgs(args);
        m_gbuffer->setShaderArgsRead(args, "gbuffer_"); 

		args.setUniform("saturatedLightBoost",      m_deepGBufferRadiositySettings.saturatedBoost);
        args.setUniform("unsaturatedLightBoost",    m_deepGBufferRadiositySettings.unsaturatedBoost);
        args.setMacro("USE_INDIRECT",				m_deepGBufferRadiositySettings.enabled);
		args.setMacro("NO_LIGHTPROBE",				(! m_demoSettings.lightProbeFallback) && (m_demoSettings.demoMode == DemoSettings::DemoMode::VARIATIONS));

        m_deepGBufferRadiosity->texture()->setShaderArgs(args, "indirectRadiosity_", Sampler::buffer());
        args.setRect(rd->viewport());

        LAUNCH_SHADER("deferred.pix", args);
    } rd->pop2D();
    END_PROFILER_EVENT();
}


bool App::onEvent(const GEvent& event) {
    if (GApp::onEvent(event)) {
        // Parent handled the event
        return true;
    } else if ((event.type == GEventType::GUI_ACTION) && (event.gui.control == m_drawerButton)) {
        // Drawer button
        if (m_gui->rect().x0() == 0) {
            // Collapse
            m_drawerButton->setCaption(m_rightIcon);
            m_gui->morphTo(m_gui->rect() - Vector2(m_drawerButton->rect().x0(), 0));
        } else {
            // Expand
            m_drawerButton->setCaption(m_leftIcon);
            m_gui->morphTo(m_gui->rect() - Vector2(m_gui->rect().x0(), 0));
        }
        return true;
    } else {
        return false;
    }
}


void App::forwardShade(RenderDevice* rd, Array<shared_ptr<Surface> >& all, const LightingEnvironment& environment) {
    static const Array<shared_ptr<Surface> > noNewShadowCasters;
    /*Surface::render(rd, activeCamera()->frame(), activeCamera()->projection(), all, 
        noNewShadowCasters, environment, false, m_settings.depthGuardBandThickness - m_settings.colorGuardBandThickness);*/

    drawDebugShapes();
}


void App::renderSplitScreen(RenderDevice* rd, Array<shared_ptr<Surface> >& all, const LightingEnvironment& environment) {
    // Super inefficient, but simple to implement split-screen
    static shared_ptr<Texture> leftScreen = Texture::createEmpty("SplitScreen::Left", 
        m_framebuffer->texture(0)->width(), m_framebuffer->texture(0)->height(),
        m_framebuffer->texture(0)->encoding());

    static shared_ptr<Texture> rightScreen = Texture::createEmpty("SplitScreen::Right", 
        m_framebuffer->texture(0)->width(), m_framebuffer->texture(0)->height(),
        m_framebuffer->texture(0)->encoding());

    leftScreen->resize(m_framebuffer->texture(0)->width(), m_framebuffer->texture(0)->height());
    rightScreen->resize(m_framebuffer->texture(0)->width(), m_framebuffer->texture(0)->height());

    Texture::copy(m_framebuffer->texture(0), rightScreen);

    m_deepGBufferRadiositySettings.enabled = false; {
        deferredShade(rd, environment); 
        forwardShade(rd, all, environment);
        m_depthOfField->apply(rd, m_framebuffer->texture(0), m_gbuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL), activeCamera(), m_settings.depthGuardBandThickness);
        
        m_motionBlur->apply(rd, m_framebuffer->texture(0), m_gbuffer->texture(GBuffer::Field::SS_EXPRESSIVE_MOTION),
                            m_gbuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL), activeCamera(), 
                            m_settings.depthGuardBandThickness);
    } m_deepGBufferRadiositySettings.enabled = true;

    Texture::copy(m_framebuffer->texture(0), leftScreen);

    rd->push2D(); {
        Args args;
        args.setUniform("separatorSize", 2.0f);
        args.setUniform("guardBandSize", settings().depthGuardBandThickness);
        leftScreen->setShaderArgs(args, "leftScreen_", Sampler::buffer());
        rightScreen->setShaderArgs(args, "rightScreen_", Sampler::buffer());
        args.setRect(rd->viewport());
        LAUNCH_SHADER("verticalSplitScreen.pix", args);    
    } rd->pop2D();
}


void App::convergeDeepGBufferRadiosity(RenderDevice* rd) {
    if ( isNull(scene()) ) {
        return;
    }
    DeepGBufferRadiositySettings previousSettings = m_deepGBufferRadiositySettings;
    m_deepGBufferRadiositySettings.temporalFilterSettings.hysteresis = 0.01f;
    m_deepGBufferRadiositySettings.propagationDamping = 0.99f;
    m_deepGBufferRadiositySettings.numSamples = 50;
    m_deepGBufferRadiositySettings.numBounces = 3;

    Array<shared_ptr<Surface> > all;
    scene()->onPose(all);

    m_gbuffer->setSpecification(m_gbufferSpecification);
    m_gbuffer->resize(m_framebuffer->width(), m_framebuffer->height());

    m_peeledGBuffer->setSpecification(m_peeledGBufferSpecification);
    m_peeledGBuffer->resize(m_framebuffer->width(), m_framebuffer->height());

    // Share the depth buffer with the forward-rendering pipeline
    m_framebuffer->set(Framebuffer::DEPTH, m_gbuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL));
    m_depthPeelFramebuffer->set(Framebuffer::DEPTH, m_peeledGBuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL));
    // Bind the main framebuffer
    rd->pushState(m_framebuffer); {
        rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
        rd->clear();

        LightingEnvironment environment;
        computeGBuffers(rd, all);
        computeShadows(rd, all, environment);
        
        renderLambertianOnly(rd, m_lambertianDirectBuffer, environment, m_gbuffer, m_deepGBufferRadiositySettings, m_gbuffer->texture(GBuffer::Field::SS_POSITION_CHANGE),
                                       m_deepGBufferRadiosity->texture(), m_previousDepthBuffer, 
                                       shared_ptr<Texture>(), shared_ptr<Texture>());

        renderLambertianOnly(rd, m_peeledLambertianDirectBuffer, environment, m_peeledGBuffer, m_deepGBufferRadiositySettings, m_gbuffer->texture(GBuffer::Field::SS_POSITION_CHANGE),
                                       m_deepGBufferRadiosity->texture(), m_previousDepthBuffer, 
                                       shared_ptr<Texture>(), shared_ptr<Texture>());

        m_deepGBufferRadiosity->update
           (rd,
            m_deepGBufferRadiositySettings, m_gbuffer,
            m_lambertianDirectBuffer->texture(0), 
            m_deepGBufferRadiositySettings.useDepthPeelBuffer ? m_peeledGBuffer : shared_ptr<GBuffer>(), 
            m_deepGBufferRadiositySettings.useDepthPeelBuffer ? m_peeledLambertianDirectBuffer->texture(0) : shared_ptr<Texture>(),
            m_settings.depthGuardBandThickness - m_settings.colorGuardBandThickness, 
            m_settings.colorGuardBandThickness,
            scene());

        if (m_deepGBufferRadiositySettings.enabled && m_deepGBufferRadiositySettings.propagationDamping < 1.0f) {
            m_gbuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL)->copyInto(m_previousDepthBuffer);
        } else {
            m_previousDepthBuffer       = shared_ptr<Texture>();
        }

    } rd->popState();
    m_deepGBufferRadiositySettings = previousSettings;
}


void App::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& all) {
    if (! scene()) {
        return;
    }

	if (m_demoSettings.demoMode == DemoSettings::DemoMode::VARIATIONS) {
		// Force the 
		m_demoSettings.globalIlluminationMode = DemoSettings::GlobalIlluminationMode::RADIOSITY;
		m_demoSettings.QualityPreset = DemoSettings::QualityPreset::BALANCED;
	}

	m_performancePane->setVisible(m_demoSettings.demoMode == DemoSettings::DemoMode::RADIOSITY);
    m_controlLabel->setVisible(m_demoSettings.cameraMode == DemoSettings::CameraMode::FREE);

    m_gbuffer->setSpecification(m_gbufferSpecification);
    m_gbuffer->resize(m_framebuffer->width(), m_framebuffer->height());

    m_peeledGBuffer->setSpecification(m_peeledGBufferSpecification);
    m_peeledGBuffer->resize(m_framebuffer->width(), m_framebuffer->height());

    // Share the depth buffer with the forward-rendering pipeline
    m_framebuffer->set(Framebuffer::DEPTH, m_gbuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL));
    m_depthPeelFramebuffer->set(Framebuffer::DEPTH, m_peeledGBuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL));
    // Bind the main framebuffer
    rd->pushState(m_framebuffer); {
        rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
        rd->clear();

        LightingEnvironment environment;
        computeGBuffers(rd, all);
        computeShadows(rd, all, environment);

        if (m_demoSettings.demoMode == DemoSettings::DemoMode::AO) {
            rd->push2D(); {
                Args args;
                environment.ambientOcclusion->texture()->setShaderArgs(args, "ao_", Sampler::buffer());
                args.setRect(rd->viewport());
                LAUNCH_SHADER("aoVisualization.pix", args);
            } rd->pop2D();

        } else {

            renderLambertianOnly(rd, m_lambertianDirectBuffer, environment, m_gbuffer, m_deepGBufferRadiositySettings, m_gbuffer->texture(GBuffer::Field::SS_POSITION_CHANGE),
                                           m_deepGBufferRadiosity->texture(), m_previousDepthBuffer, 
                                           shared_ptr<Texture>(), shared_ptr<Texture>());

            renderLambertianOnly(rd, m_peeledLambertianDirectBuffer, environment, m_peeledGBuffer, m_deepGBufferRadiositySettings, m_gbuffer->texture(GBuffer::Field::SS_POSITION_CHANGE),
                                           m_deepGBufferRadiosity->texture(), m_previousDepthBuffer, 
                                           shared_ptr<Texture>(), shared_ptr<Texture>());
            

            m_deepGBufferRadiosity->update
               (rd,
                m_deepGBufferRadiositySettings, m_gbuffer,
                m_lambertianDirectBuffer->texture(0), 
                m_deepGBufferRadiositySettings.useDepthPeelBuffer ? m_peeledGBuffer : shared_ptr<GBuffer>(), 
                m_deepGBufferRadiositySettings.useDepthPeelBuffer ? m_peeledLambertianDirectBuffer->texture(0) : shared_ptr<Texture>(),
                m_settings.depthGuardBandThickness - m_settings.colorGuardBandThickness, 
                m_settings.colorGuardBandThickness,
                scene());

            if (m_deepGBufferRadiositySettings.enabled && m_deepGBufferRadiositySettings.propagationDamping < 1.0f) {
                m_gbuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL)->copyInto(m_previousDepthBuffer);
            } else {
                m_previousDepthBuffer       = shared_ptr<Texture>();
            }

            // Remove everything that was in the G-buffer, except for the skybox, which is emissive
            // and benefits from a forward pass
            for (int i = 0; i < all.size(); ++i) {
                if (all[i]->canBeFullyRepresentedInGBuffer(m_gbuffer->specification()) && isNull(dynamic_pointer_cast<SkyboxSurface>(all[i]))) {
                    all.fastRemove(i);
                    --i;
                }
            }

            deferredShade(rd, environment); 
            forwardShade(rd, all, environment);
        
            m_depthOfField->apply(rd, m_framebuffer->texture(0), m_gbuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL), activeCamera(), m_settings.depthGuardBandThickness);
        
            m_motionBlur->apply(rd, m_framebuffer->texture(0), m_gbuffer->texture(GBuffer::Field::SS_EXPRESSIVE_MOTION),
                                m_gbuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL), activeCamera(), 
                                m_settings.depthGuardBandThickness);

            if (m_demoSettings.globalIlluminationMode == DemoSettings::GlobalIlluminationMode::SPLIT_SCREEN) {
                renderSplitScreen(rd, all, environment);
            }
        }

        
    } rd->popState();

    swapBuffers();
    rd->clear();
    FilmSettings filmSettings = activeCamera()->filmSettings();
    if (m_demoSettings.demoMode == DemoSettings::DemoMode::AO) {
		// Override the film settings to visualize AO
        filmSettings.setBloomStrength(0.0);
        filmSettings.setIdentityToneCurve();
    }
    m_film->exposeAndRender(rd, filmSettings, m_framebuffer->texture(0), 1);    
}


void App::onGraphics2D(RenderDevice* rd, Array< shared_ptr< Surface2D > >& surface2D) {
	if (m_demoSettings.demoMode == DemoSettings::DemoMode::AO) {
		rd->setFramebuffer(shared_ptr<Framebuffer>());
		const Point2 inColumnOffset(-10.0f, m_framebuffer->height() - float(settings().depthGuardBandThickness.y)*2.0f);
		float columnWidth = ((float)m_framebuffer->width() - 2.0f * settings().depthGuardBandThickness.x) / 2.0f;
		Point2 position = inColumnOffset;
		position.x += columnWidth;
		position.x += columnWidth;
		m_captionFont->draw2D(rd, String("Raw ") + (m_demoSettings.twoLayerAO ? "2-Layer Deep G-Buffer" : "1-Layer") + String(" Ambient Occlusion"), 
					position, 30, Color3::white(), Color3::black(), GFont::XALIGN_RIGHT, GFont::YALIGN_BOTTOM);
	} else {
		if (m_demoSettings.globalIlluminationMode == DemoSettings::GlobalIlluminationMode::SPLIT_SCREEN) {
			const Point2 inColumnOffset(-10.0f, m_framebuffer->height() - float(settings().depthGuardBandThickness.y) * 2.0f);
			float columnWidth = ((float)m_framebuffer->width() - 2.0f * settings().depthGuardBandThickness.x) / 2.0f;

			Point2 position = inColumnOffset;
			position.x += columnWidth;
			m_captionFont->draw2D(rd, "Prerendered Light Probe", 
						position, 30, Color3::white(), Color3::black(), GFont::XALIGN_RIGHT, GFont::YALIGN_BOTTOM);

			position.x += columnWidth;
			m_captionFont->draw2D(rd, "Deep G-Buffer Radiosity", 
						position, 30, Color3::white(), Color3::black(), GFont::XALIGN_RIGHT, GFont::YALIGN_BOTTOM);
		}
	}

    GApp::onGraphics2D(rd, surface2D);
}


void App::onAfterLoadScene(const Any& any, const String& stringName) {
    m_deepGBufferRadiositySettings               = any.get("deepGBufferRadiositySettings",              DeepGBufferRadiositySettings());
    m_maxPerformanceDeepGBufferRadiosityPresets  = any.get("maxPerformanceDeepGBufferRadiosityPresets", DeepGBufferRadiositySettings());
    m_maxQualityDeepGBufferRadiosityPresets      = any.get("maxQualityDeepGBufferRadiosityPresets",     DeepGBufferRadiositySettings());
    m_BALANCEDDeepGBufferRadiosityPresets        = any.get("BALANCEDDeepGBufferRadiosityPresets",       DeepGBufferRadiositySettings());
    evaluateDemoSettings();

    if (scene()) {
        convergeDeepGBufferRadiosity(renderDevice);
    }
}


void App::evaluateDemoSettings() {
    if ( ! m_demoSettings.advancedSettingsMode ) { // In advanced mode, you are allowed to tweak the settings yourself

        // Could be changed to happen on toggle instead of every frame
        switch(m_demoSettings.QualityPreset) {
        case DemoSettings::QualityPreset::MAX_PERFORMANCE:
            m_deepGBufferRadiositySettings = m_maxPerformanceDeepGBufferRadiosityPresets;
            break;

		case DemoSettings::QualityPreset::BALANCED:
            m_deepGBufferRadiositySettings = m_BALANCEDDeepGBufferRadiosityPresets;
            break;

        case DemoSettings::QualityPreset::MAX_QUALITY:
            m_deepGBufferRadiositySettings = m_maxQualityDeepGBufferRadiosityPresets;
            break;
        }

		m_deepGBufferRadiositySettings.useDepthPeelBuffer = m_demoSettings.twoLayerRadiosity || (m_demoSettings.demoMode != DemoSettings::DemoMode::VARIATIONS);

        m_deepGBufferRadiositySettings.enabled =  
            (m_demoSettings.globalIlluminationMode == DemoSettings::GlobalIlluminationMode::RADIOSITY) ||
            (m_demoSettings.globalIlluminationMode == DemoSettings::GlobalIlluminationMode::SPLIT_SCREEN);
        
        AmbientOcclusionSettings& aoSettings = scene()->lightingEnvironment().ambientOcclusionSettings;
		aoSettings.enabled             = m_demoSettings.aoEnabled || (m_demoSettings.demoMode != DemoSettings::DemoMode::VARIATIONS);

		if (m_demoSettings.demoMode == DemoSettings::DemoMode::AO) {
	        aoSettings.useDepthPeelBuffer  = m_demoSettings.twoLayerAO;
		} else {
	        aoSettings.useDepthPeelBuffer  = m_demoSettings.twoLayerRadiosity || (m_demoSettings.demoMode != DemoSettings::DemoMode::VARIATIONS);
		}
    }


    // Could be changed to happen on toggle instead of every frame
    Array< shared_ptr<Entity> > entityArray;
    scene()->getEntityArray(entityArray);
    for (int i = 0; i < entityArray.size(); ++i) {

        shared_ptr<VisibleEntity> visibleEntity = dynamic_pointer_cast<VisibleEntity>(entityArray[i]);
        shared_ptr<Light> light                 = dynamic_pointer_cast<Light>(entityArray[i]);
        if (notNull(visibleEntity)) {
            // Toggle only dynamic visible entities
            if (beginsWith(entityArray[i]->name(), "dynamic")) {
                visibleEntity->setVisible( m_demoSettings.dynamicLights );
            }
        }
        if (notNull(light)) {
            // Lights exclusively belong to either the dynamic or static set. Toggle all of them.
            light->setEnabled( beginsWith(entityArray[i]->name(), "dynamic") == m_demoSettings.dynamicLights );
        }
        
    }
    
    switch (m_demoSettings.cameraMode) {
    case DemoSettings::CameraMode::STATIC:
		if (m_demoSettings.demoMode == DemoSettings::DemoMode::AO) {
			setActiveCamera(scene()->typedEntity<Camera>("staticAOCamera"));
		} else {
			setActiveCamera(scene()->typedEntity<Camera>("staticStatueCamera"));
		}
		m_debugCamera->copyParametersFrom(activeCamera());
		m_cameraManipulator->setFrame(activeCamera()->frame());
        break;

    case DemoSettings::CameraMode::DYNAMIC:
		if (m_demoSettings.demoMode == DemoSettings::DemoMode::AO) {
			setActiveCamera(scene()->typedEntity<Camera>("pillarStrafeCamera"));
		} else {
			setActiveCamera(scene()->typedEntity<Camera>("statuteStrafeCamera"));
		}
		m_debugCamera->copyParametersFrom(activeCamera());
		m_cameraManipulator->setFrame(activeCamera()->frame());
        break;

	case DemoSettings::CameraMode::FREE:
        setActiveCamera(m_debugCamera);
        break;
    }

    if (m_demoSettings.significantRadiosityDifferences(m_previousDemoSettings)) {
        convergeDeepGBufferRadiosity(renderDevice);
    }

    m_previousDemoSettings = m_demoSettings;

}


void App::onUserInput(UserInput* ui) {
    GApp::onUserInput(ui);
    evaluateDemoSettings();
}


bool App::DemoSettings::significantRadiosityDifferences(const DemoSettings& other) {
    if (globalIlluminationMode == GlobalIlluminationMode::STATIC_LIGHT_PROBE) {
        return false;
    } 

    return  
		(demoMode != other.demoMode) ||
		(twoLayerAO != other.twoLayerAO) ||
		(twoLayerRadiosity != other.twoLayerRadiosity) ||
        (dynamicLights != other.dynamicLights) ||
        (cameraMode != other.cameraMode);            
}