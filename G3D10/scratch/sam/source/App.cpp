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

    // Set to true for a significant performance boost if your app can't render at 60fps,
    // or if you *want* to render faster than the display.
    settings.window.asynchronous        = false;

    settings.depthGuardBandThickness    = Vector2int16(64, 64);
    settings.colorGuardBandThickness    = Vector2int16(0, 0);
    settings.dataDir                    = FileSystem::currentDirectory();
    settings.screenshotDirectory        = "../journal/";

    settings.renderer.deferredShading = false;
    settings.renderer.orderIndependentTransparency = false;


    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {
}

// Called before the application loop begins.  Load data here and
// not in the constructor so that common exceptions will be
// automatically caught.
void App::onInit() {
    RenderDevice* rd = renderDevice;

    Vector2 destSize(1024, 1024);
    const Rect2D& dest = Rect2D::xywh(Vector2(0, 0), destSize);

    Args args;
//    args.appendToPreamble("#define KERNEL_RADIUS 9\nfloat gaussCoef[KERNEL_RADIUS] = float[KERNEL_RADIUS](0.00194372, 0.00535662, 0.01289581, 0.02712094, 0.04982645, 0.07996757, 0.11211578, 0.13731514, 0.14691596);");

    rd->push2D(dest); {
        args.setRect(dest);
        LAUNCH_SHADER("apply.*", args);
    } rd->pop2D();

    // Or Equivalently:
    //GaussianBlur::apply(renderDevice, Texture::createEmpty("test",1024,1024));
}





