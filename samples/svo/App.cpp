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
    settings.window.caption             = argv[0];
    // settings.window.debugContext     = true;

    // settings.window.width              =  854; settings.window.height       = 480;
    // settings.window.width            = 1024; settings.window.height       = 768;
    settings.window.width            = 1280; settings.window.height       = 720;

//    settings.window.width               = 1920; settings.window.height       = 1080;
    // settings.window.width            = OSWindow::primaryDisplayWindowSize().x; settings.window.height = OSWindow::primaryDisplayWindowSize().y;
    settings.window.fullScreen          = false;
    settings.window.resizable           = ! settings.window.fullScreen;
    settings.window.framed              = ! settings.window.fullScreen;

#	ifdef G3D_DEBUG
		settings.window.debugContext = true;
#	endif

    // Set to true for a significant performance boost if your app can't render at 60fps,
    // or if you *want* to render faster than the display.
    settings.window.asynchronous        = false;

    settings.hdrFramebuffer.depthGuardBandThickness    = Vector2int16(0, 0);
    settings.hdrFramebuffer.colorGuardBandThickness    = Vector2int16(0, 0);
    settings.dataDir                    = FileSystem::currentDirectory();
    settings.screenshotDirectory        = "";

	//settings.window.defaultIconFilename		= "icon.png";

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {
}


// Called before the application loop begins.  Load data here and
// not in the constructor so that common exceptions will be
// automatically caught.
void App::onInit() {
    GApp::onInit();
	debugAssertGLOk();

    setFrameDuration(1.0f / 120.0f);

    // Call setScene(shared_ptr<Scene>()) or setScene(MyScene::create()) to replace
    // the default scene here.
    
    showRenderingStats      = false;

	SVO::Specification spec;

	spec.encoding[GBuffer::Field::LAMBERTIAN].format = ImageFormat::RGBA16F();
	spec.encoding[GBuffer::Field::WS_NORMAL].format  = ImageFormat::RGBA16F();
	spec.encoding[GBuffer::Field::GLOSSY].format     = ImageFormat::RGBA16F();

	//spec.encoding[GBuffer::Field::WS_FACE_NORMAL].format  = ImageFormat::RG16F();	//Test. 2xFP16 for atomic min

#	if 0 //def G3D_DEBUG
		// Not used, but the program crashes without this
		spec.encoding[GBuffer::Field::WS_POSITION].format = ImageFormat::RGBA16F();
#	endif

	spec.dimension = Texture::DIM_3D;
	
	debugAssertGLOk();


	m_svo = SVO::create(spec, "SVO", true);
	
    makeGUI();
    // For higher-quality screenshots:
    // developerWindow->videoRecordDialog->setScreenShotFormat("PNG");
    // developerWindow->videoRecordDialog->setCaptureGui(false);
    developerWindow->cameraControlWindow->moveTo(Point2(developerWindow->cameraControlWindow->rect().x0(), 0));
    loadScene(
        //"G3D Sponza"
		//"G3D Cornell Box" // Load something simple
		"Test Scene"
        //developerWindow->sceneEditorWindow->selectedSceneName()  // Load the first scene encountered 
        );

    dynamic_pointer_cast<DefaultRenderer>(m_renderer)->setOrderIndependentTransparency(false);
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
	debugPane->beginRow(); {
		debugPane->addCheckBox("Cone trace", &m_enableSVO);
		debugPane->addCheckBox("Fragments", &m_debugSVOFragments);
		debugPane->addCheckBox("Nodes", &m_debugSVONodes);
		debugPane->addNumberBox("Level", &m_debugSVONodeLevel, "", GuiTheme::LINEAR_SLIDER, 0, SVO_MAX_DEPTH, 1);
	} debugPane->endRow();

	debugPane->beginRow(); {

		GuiSlider<float> *slider1 = debugPane->addSlider("Cone aperture: ", &m_voxelConeAperture, 0.0f, 64.0f);
		slider1->setWidth(600);

	
	} debugPane->endRow();

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
    m_gbuffer->prepare(rd, activeCamera(), 0, -(float)previousSimTimeStep(), m_settings.hdrFramebuffer.depthGuardBandThickness, m_settings.hdrFramebuffer.colorGuardBandThickness);

	
	m_renderer->render(rd, m_framebuffer, m_depthPeelFramebuffer, scene()->lightingEnvironment(), m_gbuffer, allSurfaces);
	

    // Debug visualizations and post-process effects
    rd->pushState(m_framebuffer); {

		if (m_enableSVO) {
			rd->clear();
			//rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());

			rd->push2D();
			const Vector2int32 guardBand(m_settings.hdrFramebuffer.depthGuardBandThickness - m_settings.hdrFramebuffer.colorGuardBandThickness);
			const Vector2int32 colorRegionExtent = Vector2int32(m_framebuffer->vector2Bounds()) - guardBand * 2;

			Args args;
			rd->setGuardBandClip2D(Vector2int16(guardBand));
			args.setRect(rd->viewport());

			Matrix4 proj;
			activeCamera()->getProjectUnitMatrix(m_framebuffer->rect2DBounds(), proj);
			float focalLength = proj[0][0];

			m_svo->setCurSvoId(0);
			args.setUniform("guardBand", guardBand);

			args.setUniform("focalLength", focalLength);
			args.setUniform("renderRes", Vector2(colorRegionExtent));
			args.setUniform("renderResI", colorRegionExtent);
			args.setUniform("screenRatio", float(colorRegionExtent.y) / float(colorRegionExtent.x));

			m_svo->connectToShader(args, Access::READ, m_svo->maxDepth(), m_svo->maxDepth());

			rd->setColorWrite(true);
			rd->setDepthWrite(false);
			
			const Matrix4& cameraToVoxelMatrix = Matrix4(m_svo->svoToWorldMatrix()).inverse() * activeCamera()->frame();

			args.setUniform("cameraToVoxelMatrix", cameraToVoxelMatrix);
			args.setUniform("voxelToWorldMatrix", m_svo->svoToWorldMatrix());
			args.setUniform("worldToVoxelMatrix", m_svo->worldToSVOMatrix());
			args.setUniform("wsCameraPos", activeCamera()->frame().translation);
			scene()->lightingEnvironment().setShaderArgs(args);
			args.setUniform("raycastingConeFactor", m_voxelConeAperture);
			

			rd->setDepthTest(RenderDevice::DEPTH_ALWAYS_PASS); // TODO: write gl_FragDepth and use a regular depth test here
			m_gbuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL)->setShaderArgs(args, "depth_", Sampler::buffer());
			//rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
			
			LAUNCH_SHADER("raycast.pix", args);
			rd->pop2D();
		}

		// Call to make the App show the output of debugDraw(...)
		rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
		drawDebugShapes();
		const shared_ptr<Entity>& selectedEntity = (notNull(developerWindow) && notNull(developerWindow->sceneEditorWindow)) ? developerWindow->sceneEditorWindow->selectedEntity() : shared_ptr<Entity>();
        scene()->visualize(rd, selectedEntity, allSurfaces, sceneVisualizationSettings(), activeCamera());		

		rd->setPolygonOffset(-0.2f);
		if (m_debugSVONodes) {
			m_svo->visualizeNodes(rd, m_debugSVONodeLevel);
		}
		if (m_debugSVOFragments) {
			m_svo->visualizeFragments(rd);
		}
		rd->setPolygonOffset(0.0f);

        // Post-process special effects
        m_depthOfField->apply(rd, m_framebuffer->texture(0), m_framebuffer->texture(Framebuffer::DEPTH), activeCamera(), m_settings.hdrFramebuffer.depthGuardBandThickness - m_settings.hdrFramebuffer.colorGuardBandThickness);
        
        m_motionBlur->apply(rd, m_framebuffer->texture(0), m_gbuffer->texture(GBuffer::Field::SS_EXPRESSIVE_MOTION), 
                            m_framebuffer->texture(Framebuffer::DEPTH), activeCamera(), 
                            m_settings.hdrFramebuffer.depthGuardBandThickness - m_settings.hdrFramebuffer.colorGuardBandThickness);



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

    // Perform gamma correction, bloom, and SSAA, and write to the native window frame buffer
    m_film->exposeAndRender(rd, activeCamera()->filmSettings(), m_framebuffer->texture(0));
}


void App::onAfterLoadScene(const Any& any, const String& sceneName) {
	Array<shared_ptr<Surface>> allSurfaces;
	Array<shared_ptr<Surface2D>> ignore;
	onPose(allSurfaces, ignore);

	G3D::AABox aabox;
	Surface::getBoxBounds(allSurfaces, aabox);
	const float diameter = aabox.extent().max();
	const Point3 center = aabox.center();
	const float pad = 0.10f;
	const Vector3 extent = Vector3::one() * diameter * (1.0f + pad);
	aabox = AABox(center - extent / 2, center + extent / 2);

	Box octtreeBounds(aabox);

	m_svo->init(renderDevice, size_t(SVO_POOL_SIZE) * 1024 * 1024, SVO_MAX_DEPTH, size_t(SVO_FRAGBUFFER_SIZE) * 1024 * 1024);

	m_svo->prepare(renderDevice, activeCamera(), octtreeBounds, 0.0f, /*-(float)previousSimTimeStep()*/ -0.016667f);

	Surface::renderIntoSVO(renderDevice, allSurfaces, m_svo);

	m_svo->complete(renderDevice, "SVO_downsampleValues.glc");

}
