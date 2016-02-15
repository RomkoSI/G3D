/** \file App.cpp */
#include "App.h"

G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
    VRApp::Settings settings(argc, argv);

    settings.window.caption             = "G3D GPU Ray Marching Sample";
    settings.window.width               = 1200; settings.window.height       = 650;

    // Shadertoy small window size:
    //settings.window.width               = 560; settings.window.height       = 320;

    settings.window.fullScreen          = false;
    settings.window.resizable           = ! settings.window.fullScreen;
    settings.window.framed              = ! settings.window.fullScreen;
    settings.depthGuardBandThickness    = Vector2int16(0, 0);
    settings.colorGuardBandThickness    = Vector2int16(0, 0);
    settings.dataDir                    = FileSystem::currentDirectory();

    return App(settings).run();
}


App::App(const VRApp::Settings& settings) : super(settings) {}


void App::onInit() {
    super::onInit();
    setFrameDuration(1.0f / 60.0f);
    showRenderingStats  = false;
    createDeveloperHUD();
    developerWindow->sceneEditorWindow->setVisible(false);
    developerWindow->setVisible(false);
    developerWindow->cameraControlWindow->setVisible(false);
    developerWindow->cameraControlWindow->moveTo(Point2(developerWindow->cameraControlWindow->rect().x0(), 0));
    // Just load the camera settings
    loadScene("Camera");
    m_debugController->setMoveRate(0.2f);
    setActiveCamera(m_debugCamera);
}


void App::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& allSurfaces) {
    rd->push2D(m_framebuffer); {
        rd->setDepthWrite(true);
        Args args;

        args.setUniform("cameraToWorldMatrix", activeCamera()->frame());
        args.setUniform("tanHalfFieldOfViewY",  float(tan(activeCamera()->projection().fieldOfViewAngle() / 2.0f)));

        // Projection matrix, for writing to the depth buffer
        Matrix4 projectionMatrix;
        activeCamera()->getProjectUnitMatrix(rd->viewport(), projectionMatrix);
        args.setUniform("projectionMatrix22", projectionMatrix[2][2]);
        args.setUniform("projectionMatrix23", projectionMatrix[2][3]);

        args.setRect(rd->viewport());
        LAUNCH_SHADER("shader.pix", args);
    } rd->pop2D();

    swapBuffers();

    rd->clear();
    m_film->exposeAndRender(rd, activeCamera()->filmSettings(), m_framebuffer->texture(0));
}


