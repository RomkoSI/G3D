#include "G3D/G3DAll.h"
#include "testassert.h"
#include "App.h"


void testFullRender(bool generateGoldStandard) {
    initGLG3D();

    GApp::Settings settings;

    settings.window.caption			= "Test Renders";
    settings.window.width        = 1280; settings.window.height       = 720;
    settings.film.preferredColorFormats.clear();
    settings.film.preferredColorFormats.append(ImageFormat::RGB32F());

	// Enable vsync.  Disable for a significant performance boost if your app can't render at 60fps,
	// or if you *want* to render faster than the display.
	settings.window.asynchronous	= false;
    settings.depthGuardBandThickness    = Vector2int16(64, 64);
    settings.colorGuardBandThickness    = Vector2int16(16, 16);
    settings.dataDir				= FileSystem::currentDirectory();
    if(generateGoldStandard) { // Warning! Do not change these directories without changing the App... it relies on these directories to tell what mode we are in
        settings.screenshotDirectory	= "../data-files/RenderTest/GoldStandard";
    } else {
        settings.screenshotDirectory	= "../data-files/RenderTest/Results";
    }  
    int result = App(settings).run();
    testAssertM(result == 0 ,"App failed to run");
}

void perfFullRender(bool generateGoldStandard) {

}
