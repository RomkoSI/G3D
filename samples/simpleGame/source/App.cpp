#include "App.h"
#include "PhysicsScene.h"
#include "PlayerEntity.h"

G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
    initGLG3D();

    GApp::Settings settings(argc, argv);
    
    settings.window.caption     = argv[0];
    settings.window.width       = 1280; 
    settings.window.height      = 720;

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {
}


void App::onInit() {
    GApp::onInit();

    m_gbufferSpecification.encoding[GBuffer::Field::SS_EXPRESSIVE_MOTION] = 
		Texture::Encoding(GLCaps::supportsTexture(ImageFormat::RG8()) ? ImageFormat::RG8() : ImageFormat::RGBA8(),
			FrameName::SCREEN, Color4::one() * 128.0f, Color4::one() * -64.0f);

    m_gbufferSpecification.encoding[GBuffer::Field::DEPTH_AND_STENCIL]  = ImageFormat::DEPTH32();
    m_gbufferSpecification.depthEncoding = DepthEncoding::HYPERBOLIC;

    m_gbufferSpecification.encoding[GBuffer::Field::WS_NORMAL] = ImageFormat::RGB16F();

    // Called before the application loop begins.  Load data here and
    // not in the constructor so that common exceptions will be
    // automatically caught.
    showRenderingStats    = false;

    // For higher-quality screenshots:
    // developerWindow->videoRecordDialog->setScreenShotFormat("PNG");
    // developerWindow->videoRecordDialog->setCaptureGui(false);

    m_scene = PhysicsScene::create(m_ambientOcclusion);
    // Allowing custom Entity subclasses to be parsed from .Scene.Any files
    m_scene->registerEntitySubclass("PlayerEntity", &PlayerEntity::create);
    setScene(m_scene);

    m_firstPersonMode = true;

    m_playerName = "player";

    makeGUI();

    developerWindow->cameraControlWindow->moveTo(Point2(developerWindow->cameraControlWindow->rect().x0(), 0));
    loadScene("Level");
    setActiveCamera(m_scene->typedEntity<Camera>("camera"));
    developerWindow->sceneEditorWindow->setPreventEntitySelect(true);
}



void App::makeGUI() {
    createDeveloperHUD();

    debugWindow->setVisible(false);
    developerWindow->videoRecordDialog->setEnabled(true);
    developerWindow->sceneEditorWindow->setVisible(false);
    developerWindow->cameraControlWindow->setVisible(false);

    debugWindow->pack();
    debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));
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

    if (m_firstPersonMode) {
        const shared_ptr<PlayerEntity>& p = m_scene->typedEntity<PlayerEntity>(m_playerName);
        if (notNull(p)) {
            CFrame c = p->frame();
            c.translation += Vector3(0, 0.6f, 0); // Get up to head height
            c.rotation = c.rotation * Matrix3::fromAxisAngle(Vector3::unitX(), p->headTilt());
            activeCamera()->setFrame(c);
        }
    }


    // Example GUI dynamic layout code.  Resize the debugWindow to fill
    // the screen horizontally.
    debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));

}


bool App::onEvent(const GEvent& event) {
    // Handle super-class events
    if (GApp::onEvent(event)) { return true; }

    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::TAB)) {
        m_firstPersonMode = ! m_firstPersonMode;
        const shared_ptr<Camera>& camera = m_firstPersonMode ? m_scene->defaultCamera() : debugCamera();
        setActiveCamera(camera);
    }

    return false;
}


void App::onUserInput(UserInput* ui) {
    
    GApp::onUserInput(ui);
    ui->setPureDeltaMouse(m_firstPersonMode);
    if (m_firstPersonMode) {
        const shared_ptr<PlayerEntity>& player = m_scene->typedEntity<PlayerEntity>(m_playerName);
        if (notNull(player)) {
            const float   walkSpeed = 10.0f * units::meters() / units::seconds();
            const float   pixelsPerRevolution = 30;
            const float   turnRatePerPixel  = -pixelsPerRevolution * units::degrees() / (units::seconds()); 
            const float   tiltRatePerPixel  = -0.2f * units::degrees() / (units::seconds()); 

            const Vector3& forward = -Vector3::unitZ();
            const Vector3& right   =  Vector3::unitX();

            Vector3 linear  = Vector3::zero();
            float   yaw = 0.0f;
            float   pitch = 0.0f;
            linear += forward * ui->getY() * walkSpeed;
            linear += right   * ui->getX() * walkSpeed;
            
            yaw     = ui->mouseDX() * turnRatePerPixel;
            pitch   = ui->mouseDY() * tiltRatePerPixel;

            static const Vector3 jumpVelocity(0, 40 * units::meters() / units::seconds(), 0);

            if (ui->keyPressed(GKey::SPACE)) {
                linear += jumpVelocity;
            } else {
                linear += Vector3(0, player->desiredOSVelocity().y, 0);
            }
        
            player->setDesiredOSVelocity(linear);
            player->setDesiredAngularVelocity(yaw, pitch);
        }
    }
}


void App::onPose(Array<shared_ptr<Surface> >& posed3D, Array<shared_ptr<Surface2D> >& posed2D) {
    m_widgetManager->onPose(posed3D, posed2D);
    if (m_firstPersonMode) {
        m_scene->poseExceptExcluded(posed3D, m_playerName);
    } else {
        scene()->onPose(posed3D);
    }    
    screenPrintf("WASD to move");
    screenPrintf("Mouse to turn");
    screenPrintf("Space to jump");
}
