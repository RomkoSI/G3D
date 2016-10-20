/** \file App.cpp */
#include "App.h"

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();
/*
Copyright 2011 Erik Gorset. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of
      conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice, this list
      of conditions and the following disclaimer in the documentation and/or other materials
      provided with the distribution.

      THIS SOFTWARE IS PROVIDED BY Erik Gorset ``AS IS'' AND ANY EXPRESS OR IMPLIED
      WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
      FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Erik Gorset OR
      CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
      CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
      SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
      ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
      NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
      ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

      The views and conclusions contained in the software and documentation are those of the
      authors and should not be interpreted as representing official policies, either expressed
      or implied, of Erik Gorset.
*/

#include <iostream>
#include <algorithm>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

typedef float real32;
void __cdecl herf_radix_sort(real32 *farray, real32 *sorted, uint32 elements);

void insertion_sort(int *array, int offset, int end) {
    int x, y, temp;
    for (x=offset; x<end; ++x) {
        for (y=x; y>offset && array[y-1]>array[y]; y--) {
            temp = array[y];
            array[y] = array[y-1];
            array[y-1] = temp;
        }
    }
}

// https://github.com/gorset/radix/blob/master/radix.cc
void gorset_radix_sort(int32* array, int offset, int end, int shift) {
    int x, y, value, temp;
    int last[256] = { 0 }, pointer[256];

    for (x=offset; x<end; ++x) {
        ++last[(array[x] >> shift) & 0xFF];
    }

    last[0] += offset;
    pointer[0] = offset;
    for (x=1; x<256; ++x) {
        pointer[x] = last[x-1];
        last[x] += last[x-1];
    }

    for (x=0; x<256; ++x) {
        while (pointer[x] != last[x]) {
            value = array[pointer[x]];
            y = (value >> shift) & 0xFF;
            while (x != y) {
                temp = array[pointer[y]];
                array[pointer[y]++] = value;
                value = temp;
                y = (value >> shift) & 0xFF;
            }
            array[pointer[x]++] = value;
        }
    }

    if (shift > 0) {
        shift -= 8;
        for (x=0; x<256; ++x) {
            temp = x > 0 ? pointer[x] - pointer[x-1] : pointer[0] - offset;
            if (temp > 64) {
                gorset_radix_sort(array, pointer[x] - temp, pointer[x], shift);
            } else if (temp > 1) {
                //std::sort(array + (pointer[x] - temp), array + pointer[x]);
                insertion_sort(array, pointer[x] - temp, pointer[x]);
            }
        }
    }
}

void gorset_radix_sort(int32* array, int numElements) {
    gorset_radix_sort(array, 0, numElements, 24);
}

// For qsort
int intcmp(const void *aa, const void *bb) {
    const int *a = (int *)aa, *b = (int *)bb;
    return (*a < *b) ? -1 : (*a > *b);
}

void timeit() {
    int N = 50000;
    int *random = (int *)malloc(sizeof(int) * N);
    int *array = (int *)malloc(sizeof(int) * N);
    int *extra = (int *)malloc(sizeof(int) * N);
    alwaysAssertM(array, "malloc failed!");

    for (int i=0; i<4; ++i) {
        srand(1);
        for (int n = 0; n < N; ++n) {
            array[n] = random[n] = rand();
        }

        double start, end, mtime;
        start = System::time();
        System::memcpy(array, random, sizeof(int) * N);
        for (int j = 0; j < 100; ++j) {
            switch (i) {
            case 0: std::sort((float*)array, (float*)(array+N)); break;
            case 1: qsort(array, N, sizeof(int), intcmp); break;
            case 2: gorset_radix_sort(array, N); break;
            case 3: herf_radix_sort(reinterpret_cast<float*>(array), reinterpret_cast<float*>(extra), N); break;
            }
        }
        end = System::time();

        for (int n = 0; n < N - 1; ++n) {
            alwaysAssertM(array[n] <= array[n + 1], format("sorting failed for algorithm %d\n", i));
        }

        mtime = end - start;

        switch(i) {
        case 0: logPrintf("std::sort         %f\n", mtime); break;
        case 1: logPrintf("qsort             %f\n", mtime); break;
        case 2: logPrintf("gorset_radix_sort %f\n", mtime); break;
        case 3: logPrintf("herf_radix_sort   %f\n", mtime); break;
        }
    }
    free(array);
    array = NULL;
}

int main(int argc, const char* argv[]) {
    timeit();
    return 0;
    initGLG3D();

    GApp::Settings settings(argc, argv);

    // Change the window and other startup parameters by modifying the
    // settings class.  For example:
    settings.window.caption			= argv[0];

    // Some popular resolutions:
    // settings.window.width        = 640;  settings.window.height       = 400;
    // settings.window.width		= 1024; settings.window.height       = 768;
    settings.window.width         = 1280; settings.window.height       = 720;
    // settings.window.width        = 1920; settings.window.height       = 1080;
    // settings.window.width		= OSWindow::primaryDisplayWindowSize().x; settings.window.height = OSWindow::primaryDisplayWindowSize().y;

    // Set to true for a significant performance boost if your app can't render at 60fps,
    // or if you *want* to render faster than the display.
    settings.window.asynchronous	    = true;
    settings.depthGuardBandThickness    = Vector2int16(64, 64);
    settings.colorGuardBandThickness    = Vector2int16(16, 16);
    settings.dataDir			        = FileSystem::currentDirectory();
    settings.screenshotDirectory	    = "../journal/";

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {
}


// Called before the application loop begins.  Load data here and
// not in the constructor so that common exceptions will be
// automatically caught.
void App::onInit() {
    GApp::onInit();

    // This program renders to texture for most 3D rendering, so it can
    // explicitly delay calling swapBuffers until the Film::exposeAndRender call,
    // since that is the first call that actually affects the back buffer.  This
    // reduces frame tearing without forcing vsync on.
    renderDevice->setSwapBuffersAutomatically(false);

    setFrameDuration(1.0f / 30.0f);

    // Call setScene(shared_ptr<Scene>()) or setScene(MyScene::create()) to replace
    // the default scene here.
    
    showRenderingStats      = false;
    m_showWireframe         = false;
    m_depthPeelTexture      = Texture::createEmpty("Depth Peel Texture", m_depthBuffer->width(), m_depthBuffer->height(), 
                                 m_depthBuffer->format(), m_depthBuffer->dimension(), m_depthBuffer->settings());
    m_depthPeelFramebuffer  = Framebuffer::create(m_depthPeelTexture);

    makeGUI();

    // For higher-quality screenshots:
    // developerWindow->videoRecordDialog->setScreenShotFormat("PNG");
    // developerWindow->videoRecordDialog->setCaptureGui(false);
    developerWindow->cameraControlWindow->moveTo(Point2(developerWindow->cameraControlWindow->rect().x0(), 0));
    loadScene(
              // "Cornell Box" // Load something simple
              developerWindow->sceneEditorWindow->selectedSceneName()  // Load the first scene encountered 
        );
}


void App::makeGUI() {
    // Initialize the developer HUD (using the existing scene)
    createDeveloperHUD();
    debugWindow->setVisible(true);
    developerWindow->videoRecordDialog->setEnabled(true);

    GuiPane* infoPane = debugPane->addPane("Info", GuiTheme::ORNATE_PANE_STYLE);
    infoPane->addCheckBox("Show wireframe", &m_showWireframe);

    // Example of how to add debugging controls
    infoPane->addLabel("You can add more GUI controls");
    infoPane->addLabel("in App::onInit().");
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


void App::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& allSurfaces) {
    if (! scene()) {
        return;
    }
    
    m_gbuffer->setSpecification(m_gbufferSpecification);
    m_gbuffer->resize(m_frameBuffer->width(), m_frameBuffer->height());

    // Share the depth buffer with the forward-rendering pipeline
    m_depthBuffer = m_gbuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL);
    m_frameBuffer->set(Framebuffer::DEPTH, m_depthBuffer);

    m_depthPeelTexture->resize(m_depthBuffer->width(), m_depthBuffer->height());

    // Bind the main frameBuffer
    rd->pushState(m_frameBuffer); {
        rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());

        m_gbuffer->prepare(rd, activeCamera(),  0, -(float)previousSimTimeStep(), m_settings.depthGuardBandThickness, m_settings.colorGuardBandThickness);
        rd->clear();
        
        // Cull and sort
        Array<shared_ptr<Surface> > sortedVisibleSurfaces;
        Surface::cull(activeCamera()->frame(), activeCamera()->projection(), rd->viewport(), allSurfaces, sortedVisibleSurfaces);
        Surface::sortBackToFront(sortedVisibleSurfaces, activeCamera()->frame().lookVector());
        
        const bool renderTransmissiveSurfaces = false;

        // Intentionally copy the lighting environment for mutation
        LocalLightingEnvironment environment = scene()->localLightingEnvironment();
        environment.ambientOcclusion = m_ambientOcclusion;
       
        // Render z-prepass and G-buffer.
        Surface::renderIntoGBuffer(rd, sortedVisibleSurfaces, m_gbuffer, activeCamera()->previousFrame(), renderTransmissiveSurfaces);

        // This could be the OR of several flags; the starter begins with only one motivating algorithm for depth peel
        const bool needDepthPeel = environment.ambientOcclusionSettings.useDepthPeelBuffer;
        if (needDepthPeel) {
            rd->pushState(m_depthPeelFramebuffer); {
                rd->clear();
                rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
                Surface::renderDepthOnly(rd, sortedVisibleSurfaces, CullFace::BACK, renderTransmissiveSurfaces, m_depthBuffer, environment.ambientOcclusionSettings.depthPeelSeparationHint);
            } rd->popState();
        }
        

        if (! m_settings.colorGuardBandThickness.isZero()) {
            rd->setGuardBandClip2D(m_settings.colorGuardBandThickness);
        }        

        // Compute AO
        m_ambientOcclusion->update(rd, environment.ambientOcclusionSettings, activeCamera(), m_frameBuffer->texture(Framebuffer::DEPTH), m_depthPeelTexture, m_gbuffer->texture(GBuffer::Field::CS_FACE_NORMAL), m_gbuffer->specification().encoding[GBuffer::Field::CS_FACE_NORMAL], m_settings.depthGuardBandThickness - m_settings.colorGuardBandThickness);

        // No need to write depth, since it was covered by the gbuffer pass
        //rd->setDepthWrite(false);
        // Compute shadow maps and forward-render visible surfaces
        Surface::render(rd, activeCamera()->frame(), activeCamera()->projection(), sortedVisibleSurfaces, allSurfaces, environment, Surface::ALPHA_BINARY, true, m_settings.depthGuardBandThickness - m_settings.colorGuardBandThickness);
        
        if (m_showWireframe) {
            Surface::renderWireframe(rd, sortedVisibleSurfaces);
        }
                
        // Call to make the App show the output of debugDraw(...)
        drawDebugShapes();
        scene()->visualize(rd, sceneVisualizationSettings());

        // Post-process special effects
        m_depthOfField->apply(rd, m_frameBuffer->texture(0), m_depthBuffer, activeCamera(), m_settings.depthGuardBandThickness - m_settings.colorGuardBandThickness);
        
        m_motionBlur->apply(rd, m_frameBuffer->texture(0), m_gbuffer->texture(GBuffer::Field::SS_POSITION_CHANGE), 
                            m_gbuffer->specification().encoding[GBuffer::Field::SS_POSITION_CHANGE], m_depthBuffer, activeCamera(), 
                            m_settings.depthGuardBandThickness - m_settings.colorGuardBandThickness);

    } rd->popState();

    // We're about to render to the actual back buffer, so swap the buffers now.
    // This call also allows the screenshot and video recording to capture the
    // previous frame just before it is displayed.
    swapBuffers();

	// Clear the entire screen (needed even though we'll render over it, since
    // AFR uses clear() to detect that the buffer is not re-used.)
    rd->clear();

    // Perform gamma correction, bloom, and SSAA, and write to the native window frame buffer
    m_film->exposeAndRender(rd, activeCamera()->filmSettings(), m_frameBuffer->texture(0));
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

    // Example GUI dynamic layout code.  Resize the debugWindow to fill
    // the screen horizontally.
    debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));
}


bool App::onEvent(const GEvent& event) {
    // Handle super-class events
    if (GApp::onEvent(event)) { return true; }

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


void App::onPose(Array<shared_ptr<Surface> >& surface, Array<shared_ptr<Surface2D> >& surface2D) {
    GApp::onPose(surface, surface2D);

    // Append any models to the arrays that you want to later be rendered by onGraphics()
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
