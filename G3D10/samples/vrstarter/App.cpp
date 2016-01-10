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

    VRApp::Settings settings(argc, argv);
    settings.vr.debugMirrorMode = //VRApp::DebugMirrorMode::NONE;//
    VRApp::DebugMirrorMode::POST_DISTORTION;
    settings.vr.disablePostEffectsIfTooSlow = false;

    settings.window.caption             = argv[0];

    // The debugging on-screen window, and the size of the 2D HUD virtual layer in VR in pixels.
    // Because DK2 is relatively low resolution, don't make this too large.
    settings.window.width               = 1280; settings.window.height       = 700;

    // Full screen minimizes latency (we think), but when debugging (even in release mode) 
    // it is convenient to not have the screen flicker and change focus when launching the app.
    settings.window.fullScreen          = false;
    settings.window.resizable           = false;
    settings.window.framed              = ! settings.window.fullScreen;

    // Oculus already provides a huge guard band
    settings.depthGuardBandThickness    = Vector2int16(0, 0);
    settings.colorGuardBandThickness    = Vector2int16(0, 0);
    settings.window.asynchronous        = true;

    settings.renderer.deferredShading   = true;
    settings.renderer.orderIndependentTransparency = true;

    settings.dataDir                    = FileSystem::currentDirectory();
    settings.screenshotDirectory        = "";

    const int result = App(settings).run();
    
    return result;
}


App::App(const GApp::Settings& settings) : VRApp(settings) {
}


// Called before the application loop begins.  Load data here and
// not in the constructor so that common exceptions will be
// automatically caught.
void App::onInit() {
    VRApp::onInit();
    // Call setScene(shared_ptr<Scene>()) or setScene(MyScene::create()) to replace
    // the default scene here.
    
    showRenderingStats      = true;

    makeGUI();
    // For higher-quality screenshots:
    // developerWindow->videoRecordDialog->setScreenShotFormat("PNG");
    // developerWindow->videoRecordDialog->setCaptureGui(false);
    developerWindow->cameraControlWindow->moveTo(Point2(developerWindow->cameraControlWindow->rect().x0(), 0));
    loadScene(
        "G3D Holodeck"
		//"G3D Cornell Box" // Load something simple
        //developerWindow->sceneEditorWindow->selectedSceneName()  // Load the first scene encountered 
        );
}


void App::makeGUI() {
    // Initialize the developer HUD (using the existing scene)
    createDeveloperHUD();
    debugWindow->setVisible(false);
    developerWindow->videoRecordDialog->setEnabled(true);
    developerWindow->cameraControlWindow->setVisible(false);

    if (false) {
        developerWindow->profilerWindow->setVisible(true);
        Profiler::setEnabled(true);
    }

    debugWindow->pack();
    debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));
}


void App::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& allSurfaces) {
    // Write your onGraphics3D here!
    GApp::onGraphics3D(rd, allSurfaces);
}


void App::onAfterLoadScene(const Any &any, const String &sceneName) {
    setActiveCamera(debugCamera());
}


bool App::onEvent(const GEvent& event) {
    // Handle super-class events
    if (VRApp::onEvent(event)) { return true; }

    /*
    // For debugging effect levels:
    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == 'i')) { 
        decreaseEffects();
        return true; 
    }
    */

    return false;
}

