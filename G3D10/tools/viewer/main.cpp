/**
 \file viewer/main.cpp
 
 App that allows viewing of 3D assets, either as a command line argument or after dragging and dropping
 onto the window
 
 \maintainer Morgan McGuire
 \author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 \created 2007-05-31
 \edited  2014-10-04
 */
#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "App.h"

#if defined(G3D_VER) && (G3D_VER < 100000)
#   error Requires G3D 10.00
#endif

G3D_START_AT_MAIN();

int main(int argc, char** argv) {
    // Create the log file in the directory of the executable, not the data file
    Log::common(FilePath::concat(FilePath::parent(System::currentProgramFilename()), "log.txt"));
	{
		G3DSpecification g3dSpec;
		g3dSpec.audio = false;
		initGLG3D(g3dSpec);
	}

    String filename;
    
    if (argc > 1) {
        filename = argv[1];
    }

    // Force the log to start and write out information before we hit the first
    // System::findDataFile call
    logLazyPrintf("Launch command: %s %s\n", argv[0], filename.c_str());
    char b[2048];
    (void)getcwd(b, 2048);
    logPrintf("cwd = %s\n\n", b);
    
    GApp::Settings settings;

    settings.writeLicenseFile = false;
    settings.window.resizable = true;

#   ifdef G3D_OSX
        settings.window.defaultIconFilename = System::findDataFile("G3D-128.png", false);
#   else
        settings.window.defaultIconFilename = System::findDataFile("G3D-64.png", false);
#   endif
    settings.window.width     = 1024;
    settings.window.height    = 768;
    settings.window.caption   = "G3D Viewer";
    
    settings.renderer.deferredShading = true;
    settings.renderer.orderIndependentTransparency = true;

    logLazyPrintf("---------------------------------------------------------------------\n\n");
    logPrintf("Invoking App constructor\n");

    try {
        return App(settings, filename).run();
    } catch (const FileNotFound& e) {
        logPrintf("Uncaught exception at main(): %s\n", e.message.c_str());
        alwaysAssertM(false, e.message);
    } catch (const G3D::String& e) {
        logPrintf("Uncaught exception at main(): %s\n", e.c_str());
        alwaysAssertM(false, e);
    } catch (...) {
        logPrintf("Uncaught exception at main().\n");
    }
}
