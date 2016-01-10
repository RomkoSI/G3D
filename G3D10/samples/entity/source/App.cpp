#include "App.h"
#include "DemoScene.h"
#include "PlayerEntity.h"

G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
    G3D::G3DSpecification spec;
    spec.audio = true;
    initGLG3D(spec);

    GApp::Settings settings(argc, argv);
    
    settings.window.caption     = "G3D Entity Sample";
    settings.window.width       = 1280; 
    settings.window.height      = 720;
    try {
        settings.window.defaultIconFilename = System::findDataFile("icon/rocket/icon.png");
    } catch (...) {
        debugPrintf("Could not find icon\n");
        // Ignore exception if we can't find the icon
    }

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

    try {
        const String musicFile = System::findDataFile("music/cdk_-_above_All_(Original_RumbleStep_Mix).mp3", true);
        m_backgroundMusic = Sound::create(musicFile, true);
        m_backgroundMusic->play();
    } catch (...) {
        msgBox("This sample requires the 'game' asset pack to be installed in order to play the sound files", "Assets Missing");
    }

    setFrameDuration(1.0f / 60.0f);

    // Replace the default Scene instance
    m_scene = DemoScene::create(m_ambientOcclusion);

    // Allowing custom Entity subclasses to be parsed from .Scene.Any files
    m_scene->registerEntitySubclass("PlayerEntity", &PlayerEntity::create);
    setScene(m_scene);

    makeGUI();

    loadScene(System::findDataFile("space.Scene.Any"));

    // Enforce correct simulation order by placing constraints on objects
    m_scene->setOrder("player", "camera");
    m_scene->spawnAsteroids();
    setActiveCamera(m_scene->typedEntity<Camera>("camera"));
}


void App::makeGUI() {
    // Initialize the developer HUD
    createDeveloperHUD();

    debugWindow->setVisible(false);
    developerWindow->setVisible(false);
    developerWindow->videoRecordDialog->setEnabled(true);
    developerWindow->sceneEditorWindow->setVisible(false);
    developerWindow->cameraControlWindow->setVisible(false);

    debugWindow->pack();
    debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));

    developerWindow->cameraControlWindow->moveTo(Point2(developerWindow->cameraControlWindow->rect().x0(), 0));
}


void App::onUserInput(UserInput* ui) {
    GApp::onUserInput(ui);

    if (! m_debugController->enabled()) {

        const shared_ptr<PlayerEntity>& player = m_scene->typedEntity<PlayerEntity>("player");
        if (notNull(player)) {
            player->setDesiredOSVelocity(Vector3(ui->getX() * 100, -ui->getY() * 100, 0.0f));
        }
    }
}


void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    GApp::onSimulation(rdt, sdt, idt);

    // Update the follow-camera (this logic could be placed on the camera if it were
    // a subclass of GCamera)
    const shared_ptr<Entity>& camera = m_scene->entity("camera");
    const shared_ptr<Entity>& player = m_scene->entity("player");
    if (notNull(camera) && notNull(player)) {
        const CFrame& playerFrame = player->frame();
        CFrame cameraFrame;
        cameraFrame.translation.x = playerFrame.translation.x / 2;
        cameraFrame.translation.y = playerFrame.translation.y / 2 + 2.0f;
        cameraFrame.translation.z = playerFrame.translation.z + 14.0f;

        float yaw, pitch, roll;
        playerFrame.rotation.toEulerAnglesXYZ(yaw, pitch, roll);
        cameraFrame.rotation = Matrix3::fromAxisAngle(Vector3::unitX(), -0.15f) * Matrix3::fromAxisAngle(Vector3::unitZ(), roll / 5);
        camera->setFrame(cameraFrame);
    }
}

