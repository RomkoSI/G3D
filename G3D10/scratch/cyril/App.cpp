/** \file App.cpp */
#include "App.h"

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
    initGLG3D();

    GApp::Settings settings(argc, argv);

    // Change the window and other startup parameters by modifying the
    // settings class.  For example:
    settings.window.caption      = argv[0];
    //settings.window.width        = 1024; settings.window.height       = 768;
    settings.window.width        = 1280; settings.window.height       = 720;
//     settings.window.width        = 1920; settings.window.height       = 1000;
    settings.guardBandThickness  = Vector2int16(0, 0);
    // settings.screenshotDirectory = "../journal/";
    
    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {
}


void App::onInit() {
    GApp::onInit();
    // Called before the application loop begins.  Load data here and
    // not in the constructor so that common exceptions will be
    // automatically caught.
    
    showRenderingStats    = false;
    m_showWireframe       = false;
    m_visualizeVoxelFragments = true;
    m_visualizeVoxelTree  = true;
	m_visualizeVoxelRaycasting	= false;

    m_visualizeTreeLevel  = 1;
    m_scene = Scene::create();

    makeGBuffer();

    {
        SVO::Specification spec;
        spec.format[GBuffer::Field::LAMBERTIAN] = ImageFormat::RGBA8();
        m_svo = SVO::create(spec);
    }

    makeGUI();

    // For higher-quality screenshots:
    // developerWindow->videoRecordDialog->setScreenShotFormat("PNG");
    // developerWindow->videoRecordDialog->setCaptureGui(false);

    developerWindow->cameraControlWindow->moveTo(Point2(developerWindow->cameraControlWindow->rect().x0(), 0));
    loadScene(developerWindow->sceneEditorWindow->selectedSceneName());
}


void App::makeGBuffer() {
    // If you do not use motion blur or deferred shading, you can avoid allocating the GBuffer here to save resources
    GBuffer::Specification specification;

    specification.format[GBuffer::Field::SS_POSITION_CHANGE]    = GLCaps::supportsTexture(ImageFormat::RG8()) ? ImageFormat::RG8() : ImageFormat::RGBA8();
    specification.encoding[GBuffer::Field::SS_POSITION_CHANGE]  = Vector2(128.0f, -64.0f);

    specification.format[GBuffer::Field::CS_FACE_NORMAL]        = ImageFormat::RGB8();
    specification.encoding[GBuffer::Field::CS_FACE_NORMAL]      = Vector2(2.0f, -1.0f);

    specification.format[GBuffer::Field::DEPTH_AND_STENCIL]     = ImageFormat::DEPTH32();
    specification.depthEncoding = DepthEncoding::HYPERBOLIC;

    m_gbuffer = GBuffer::create(specification);

    m_gbuffer->resize(renderDevice->width(), renderDevice->height());
    m_gbuffer->texture(GBuffer::Field::SS_POSITION_CHANGE)->visualization = Texture::Visualization::unitVector();

    // Share the depth buffer with the forward-rendering pipeline
    m_depthBuffer = m_gbuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL);
    m_framebuffer->set(Framebuffer::DEPTH, m_depthBuffer);
}


void App::makeGUI() {
	// Initialize the developer HUD
    createDeveloperHUD();

#if 0	//Test
	//////////////
	shared_ptr<GFont>      arialFont   = GFont::fromFile(System::findDataFile("arial.fnt"));
	shared_ptr<GuiTheme>   theme       = GuiTheme::fromFile(System::findDataFile("osx-10.7.gtm"), arialFont);
	m_profilerResultWindow = ProfilerResultWindow::create( m_svo->m_profiler, theme );
	m_profilerResultWindow->setVisible(true);
	m_profilerResultWindow->setRect(Rect2D(Vector2(500, 500)));
	addWidget(m_profilerResultWindow);
	/////////////
#endif

    debugWindow->setVisible(true);
    developerWindow->videoRecordDialog->setEnabled(true);

    GuiPane* infoPane = debugPane->addPane("Info", GuiTheme::ORNATE_PANE_STYLE);
    infoPane->addCheckBox("Show wireframe", &m_showWireframe);

    // Example of how to add debugging controls
    infoPane->addCheckBox("Voxel Fragments", &m_visualizeVoxelFragments);
    infoPane->addCheckBox("Oct Tree", &m_visualizeVoxelTree);
	infoPane->addCheckBox("Raycasting", &m_visualizeVoxelRaycasting);
	
    infoPane->addNumberBox("    Level", &m_visualizeTreeLevel, "", GuiTheme::LINEAR_SLIDER, int(1), int(20), 1);
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


void App::loadScene(const std::string& sceneName) {
    // Use immediate mode rendering to force a simple message onto the screen
    drawMessage("Loading " + sceneName + "...");

    const std::string oldSceneName = m_scene->name();

    // Load the scene
    try {
        const Any& any = m_scene->load(sceneName);

        // TODO: Parse extra fields that you've added to the .scn.any
        // file here.
        (void)any;

        // If the debug camera was active and the scene is the same as before, retain the old camera.  Otherwise,
        // switch to the default camera specified by the scene.

        if ((oldSceneName != m_scene->name()) || (activeCamera()->name() != "(Debug Camera)")) {

            // Because the CameraControlWindow is hard-coded to the
            // m_debugCamera, we have to copy the camera's values here
            // instead of assigning a pointer to it.
            m_debugCamera->copyParametersFrom(m_scene->defaultCamera());
            m_debugController->setFrame(m_debugCamera->frame());

            setActiveCamera(m_scene->defaultCamera());
        }

    } catch (const ParseError& e) {
        const std::string& msg = e.filename + format(":%d(%d): ", e.line, e.character) + e.message;
        debugPrintf("%s", msg.c_str());
        drawMessage(msg);
        System::sleep(5);
        m_scene->clear();
    }
}


void App::saveScene() {
    // Called when the "save" button is pressed
    if (m_scene) {
        Any a = m_scene->toAny();
        const std::string& filename = a.source().filename;
        if (filename != "") {
            a.save(filename);
            debugPrintf("Saved %s\n", filename.c_str());
        } else {
            debugPrintf("Could not save: empty filename");
        }
    }
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
    m_scene->onSimulation(sdt);

    // Example GUI dynamic layout code.  Resize the debugWindow to fill
    // the screen horizontally.
    debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));
}


bool App::onEvent(const GEvent& event) {
    // Handle super-class events
    if (GApp::onEvent(event)) { return true; }

    if (event.type == GEventType::FILE_DROP) {
        Array<std::string> fileArray;
        window()->getDroppedFilenames(fileArray);

        if (endsWith(toLower(fileArray[0]), ".scn.any")) {
            // Load a scene
            loadScene(fileArray[0]);
            return true;
        } else if (endsWith(toLower(fileArray[0]), ".am.any")) {

            // Trace a ray from the drop point
            Model::HitInfo hitInfo;
            m_scene->intersectEyeRay(activeCamera(), Vector2(event.drop.x + 0.5f, event.drop.y + 0.5f), renderDevice->viewport(), settings().guardBandThickness, false, Array<shared_ptr<Entity> >(), hitInfo);

            if (hitInfo.point.isNaN()) {
                // The drop wasn't on a surface, so choose a point in front of the camera at a reasonable distance
                CFrame cframe = activeCamera()->frame();
				//Broken by HitInfo members becoming const...
				//hitInfo.point = G3D::Point3(cframe.lookVector() * 4 + cframe.translation);
                //hitInfo.normal = Vector3::unitY();
            }

            // Insert a Model
            Any modelAny;
            modelAny.load(fileArray[0]);
            int nameModifier = -1;
            Array<std::string> entityNames;
            m_scene->getEntityNames(entityNames);
            
            // creates a unique name in order to avoid conflicts from multiple models being dragged in
            std::string name = FilePath::base(fileArray[0]);
            if(entityNames.contains(name)) { 
                do {
                    ++nameModifier;
                } while (entityNames.contains(name + (format("%d", nameModifier))));
                name.append(format("%d", nameModifier));
            }

            const std::string newModelName  = name;
            const std::string newEntityName = name;
            
            m_scene->createModel(modelAny, newModelName);

            Any entityAny(Any::TABLE, "VisibleEntity");
            // Insert an Entity for that model
            entityAny["frame"] = CFrame(hitInfo.point);
            entityAny["model"] = newModelName;

            m_scene->createEntity("VisibleEntity", newEntityName, entityAny);
            return true;
        }
    }

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


void App::onPose(Array<shared_ptr<Surface> >& posed3D, Array<shared_ptr<Surface2D> >& posed2D) {
    GApp::onPose(posed3D, posed2D);

    // Append any models to the arrays that you want to later be rendered by onGraphics()
    m_scene->onPose(posed3D);
}


void App::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& allSurfaces) {
    // Bind the main framebuffer
    rd->pushState(m_framebuffer); {
		BEGIN_PROFILER_EVENT("Frame");

        rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());

        m_gbuffer->resize(rd->width(), rd->height());
        m_gbuffer->prepare(rd, activeCamera(),  0, -(float)previousSimTimeStep(), m_settings.guardBandThickness);
        rd->clear();

        static bool first = true;

        if (first || userInput->keyPressed(' ') ) {

#if 1	
			CoordinateFrame frame;
			frame.rotation=Matrix3::fromAxisAngle(-Vector3(1,0.5,1), 1.5f);
			frame.translation=Point3(4.0,1.0,1.0);

            Box octtreeBounds = Box(Vector3(-4,-4,-4), Vector3(4,4,4) /*, frame*/ );
#else		
            Box octtreeBounds = activeCamera()->frustum(rd->viewport()).boundingBox(50);		//Not working
#endif

            m_svo->prepare(rd, activeCamera(), octtreeBounds, 0, -(float)previousSimTimeStep(), 134217728/4, 9, 134217728);

            Array<shared_ptr<Surface> > visibleSurfaces;
            Surface::cull(activeCamera()->frame(), activeCamera()->projection(), rd->viewport(), allSurfaces, visibleSurfaces);
            Surface::renderIntoSVO(rd, visibleSurfaces, m_svo);

            // TODO: Make leaf pointers
            // TODO: Fragment shader conservative 3-layer code
            // TODO: Debug camera frustum bounds
            // TODO: Disable parallax mapping during SVO write
            // TODO: Host enable MSAA option (in GBuffer specification...in Texture settings)

            m_svo->complete(rd);

            first = false;
        }

        // Cull and sort
        Array<shared_ptr<Surface> > sortedVisibleSurfaces;
        Surface::cull(activeCamera()->frame(), activeCamera()->projection(), rd->viewport(), allSurfaces, sortedVisibleSurfaces);
        Surface::sortBackToFront(sortedVisibleSurfaces, activeCamera()->frame().lookVector());
        
        // Depth pre-pass
        static const bool renderTransmissiveSurfaces = false;
        Surface::renderDepthOnly(rd, sortedVisibleSurfaces, CullFace::BACK, renderTransmissiveSurfaces);
        LightingEnvironment environment = m_scene->lightingEnvironment();
        

        if (! m_settings.guardBandThickness.isZero()) {
            rd->setGuardBandClip2D(m_settings.guardBandThickness);
        }

        if (m_visualizeVoxelFragments) {
            m_svo->visualizeFragments(rd);
		}

        if (m_visualizeVoxelTree) {
            m_svo->visualizeNodes(rd, m_visualizeTreeLevel);
        }

        // Render G-buffer if needed.  In this default implementation, it is needed if motion blur is enabled (for velocity) or
        // if face normals have been allocated and ambient occlusion is enabled.
        if (activeCamera()->motionBlurSettings().enabled() || 
            (environment.ambientOcclusionSettings.enabled && 
             notNull(m_gbuffer) && 
             notNull(m_gbuffer->texture(GBuffer::Field::CS_FACE_NORMAL)))) {
            //rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
            rd->setDepthWrite(false); { 
                // We've already rendered the depth
                Surface::renderIntoGBuffer(rd, sortedVisibleSurfaces, m_gbuffer, activeCamera()->previousFrame());
            } rd->setDepthWrite(true);
        }


        // Compute AO
        m_ambientOcclusion->update
            (rd, 
            environment.ambientOcclusionSettings, 
            activeCamera(), 
            m_depthBuffer, 
            shared_ptr<Texture>(), 
            notNull(m_gbuffer) ? m_gbuffer->texture(GBuffer::Field::CS_FACE_NORMAL) : shared_ptr<Texture>(),
	     notNull(m_gbuffer) ? Vector2(m_gbuffer->specification().encoding[GBuffer::Field::CS_FACE_NORMAL]) : Vector2(1, 0),
            m_settings.guardBandThickness);


        // Compute shadow maps and forward-render visible surfaces
        environment.ambientOcclusion = m_ambientOcclusion;
        Surface::render(rd, activeCamera()->frame(), activeCamera()->projection(), sortedVisibleSurfaces, allSurfaces, environment);
        
        if (m_showWireframe) {
            Surface::renderWireframe(rd, sortedVisibleSurfaces);
        }
        

		if (m_visualizeVoxelRaycasting) {
			m_svo->renderRaycasting(rd, m_visualizeTreeLevel);
        }


        //////////////////////////////////////////////////////
        // Sample (deprecated) OpenGL fixed-function rendering code
        if (false) {
            Light::bindFixedFunction(rd, environment.lightArray, environment.environmentMapArray[0].texture->mean().rgb() * environment.environmentMapArray[0].constant);
            
            // Make any OpenGL calls that you want, or use high-level primitives
            
            Draw::sphere(Sphere(Vector3(2.5f, 0.5f, 0), 0.5f), rd, Color3::white(), Color4::clear());
            Draw::box(AABox(Vector3(-2.0f, 0.0f, -0.5f), Vector3(-1.0f, 1.0f, 0.5f)), rd, Color4(Color3::orange(), 0.25f), Color3::black());
            
            Light::unbindFixedFunction();
        }
        
        // Call to make the App show the output of debugDraw(...)
        drawDebugShapes();
        m_scene->visualize(rd, sceneVisualizationSettings());

        // Post-process special effects
        m_depthOfField->apply(rd, m_colorBuffer0, m_depthBuffer, activeCamera(), m_settings.guardBandThickness, m_settings.guardBandThickness);
        
        m_motionBlur->apply(rd, m_colorBuffer0, m_gbuffer->texture(GBuffer::Field::SS_POSITION_CHANGE), 
                            m_gbuffer->specification().encoding[GBuffer::Field::SS_POSITION_CHANGE], m_depthBuffer, activeCamera(), 
                            m_settings.guardBandThickness, m_settings.guardBandThickness);


		END_PROFILER_EVENT();
    } rd->popState();

    // Perform gamma correction, bloom, and SSAA, and write to the native window frame buffer
    m_film->exposeAndRender(rd, activeCamera()->filmSettings(), m_colorBuffer0);


	/////////////
	END_PROFILER_EVENT();

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
