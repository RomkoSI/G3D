/**
 \file App.cpp
 
 App that allows viewing of 2D and 3D assets
 
 \maintainer Morgan McGuire, http://graphics.cs.williams.edu
 \author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 \created 2007-05-31
 \edited  2014-10-03
 */
#include "App.h"
#include "ArticulatedViewer.h"
#include "TextureViewer.h"
#include "FontViewer.h"
#include "MD2Viewer.h"
#include "MD3Viewer.h"
#include "GUIViewer.h"
#include "EmptyViewer.h"
#include "VideoViewer.h"
#include "IconSetViewer.h"
#include "EventViewer.h"

App::App(const GApp::Settings& settings, const G3D::String& file) :
    GApp(settings),
    viewer(NULL),
    filename(file) {

    logPrintf("App()\n");

    m_debugTextColor = Color3::black();
    m_debugTextOutlineColor = Color3::white();
    m_debugCamera->filmSettings().setVignetteBottomStrength(0);
    m_debugCamera->filmSettings().setVignetteTopStrength(0);
    m_debugCamera->filmSettings().setVignetteSizeFraction(0);
    catchCommonExceptions = true;
}


void App::onInit() {
    GApp::onInit();


	renderDevice->setSwapBuffersAutomatically(true);
    logPrintf("App::onInit()\n");
    createDeveloperHUD();
    showRenderingStats = false;

    developerWindow->cameraControlWindow->setVisible(false);
    developerWindow->setVisible(false);
    developerWindow->videoRecordDialog->setCaptureGui(false);

    m_debugCamera->filmSettings().setBloomStrength(0.20f);
    m_debugCamera->filmSettings().setBloomRadiusFraction(0.017f);
    m_debugCamera->filmSettings().setAntialiasingEnabled(true);
    m_debugCamera->filmSettings().setCelluloidToneCurve();

    if (! filename.empty()) {
        window()->setCaption(filenameBaseExt(filename) + " - G3D Viewer");
    }

    lighting = shared_ptr<LightingEnvironment>(new LightingEnvironment());
    lighting->lightArray.clear();
    // The spot light is designed to just barely fit the 3D models.  Note that it has no attenuation
    lighting->lightArray.append(Light::spotTarget("Light", Point3(40, 120, 80), Point3::zero(), 10 * units::degrees(), Power3(50.0f), 1, 0, 0, true, 8192));
    lighting->lightArray.last()->shadowMap()->setBias(0.1f);

    Texture::Encoding e;
    e.readMultiplyFirst = Color4(Color3(0.5f));
    e.format = ImageFormat::RGB32F();

    lighting->environmentMapArray.append(Texture::fromFile(System::findDataFile("uffizi/uffizi-*.exr"), e, Texture::DIM_CUBE_MAP));
    lighting->ambientOcclusionSettings.numSamples   = 24;
    lighting->ambientOcclusionSettings.radius       = 0.75f * units::meters();
    lighting->ambientOcclusionSettings.intensity    = 2.0f;
    lighting->ambientOcclusionSettings.bias         = 0.06f * units::meters();
    lighting->ambientOcclusionSettings.useDepthPeelBuffer = true;

    m_debugCamera->setFarPlaneZ(-finf());
    m_debugCamera->setNearPlaneZ(-0.05f);

    // Don't clip to the near plane
    glDisable(GL_DEPTH_CLAMP);	
    colorClear = Color3::white() * 0.9f;

    //modelController = ThirdPersonManipulator::create();
    m_gbufferSpecification.encoding[GBuffer::Field::CS_POSITION_CHANGE].format = NULL;
    gbuffer()->setSpecification(m_gbufferSpecification);

    setViewer(filename);
    developerWindow->sceneEditorWindow->setVisible(false);
    logPrintf("Done App::onInit()\n");
}


void App::onCleanup() {
    delete viewer;
    viewer = NULL;
}


bool App::onEvent(const GEvent& e) {
    if (viewer != NULL) {
        if (viewer->onEvent(e, this)) {
            // Viewer consumed the event
            return true;
        }
    }

    switch (e.type) {
    case GEventType::FILE_DROP:
        {
            Array<G3D::String> fileArray;
            window()->getDroppedFilenames(fileArray);
            setViewer(fileArray[0]);
            return true;
        }

    case GEventType::KEY_DOWN:
        if (e.key.keysym.sym == GKey::F5) { 
            Shader::reloadAll();
            return true;
        } else if (e.key.keysym.sym == GKey::F3) {
            showDebugText = ! showDebugText;
            return true;
        } else if (e.key.keysym.sym == GKey::F8) {
            Array<shared_ptr<Texture> > output;
            renderCubeMap(renderDevice, output, m_debugCamera, shared_ptr<Texture>(), 2048);

            const CubeMapConvention::CubeMapInfo& cubeMapInfo = Texture::cubeMapInfo(CubeMapConvention::DIRECTX);
            for (int f = 0; f < 6; ++f) {
                const CubeMapConvention::CubeMapInfo::Face& faceInfo = cubeMapInfo.face[f];

                shared_ptr<Image> temp = Image::fromPixelTransferBuffer(output[f]->toPixelTransferBuffer(ImageFormat::RGB8()));
                temp->flipVertical();
                temp->rotateCW(toRadians(90.0) * (-faceInfo.rotations));
                if (faceInfo.flipY) { temp->flipVertical();   }
                if (faceInfo.flipX) { temp->flipHorizontal(); }
                temp->save(format("cube-%s.png", faceInfo.suffix.c_str()));
            }
            return true;
        } else if (e.key.keysym.sym == 'v') {
            // Event viewer
            if (isNull(dynamic_cast<EventViewer*>(viewer))) {
                setViewer("<events>");
                return true;
            }
        }
        break;
        
    default:;
    }

    // Must call after processing events to prevent default .ArticulatedModel.Any file-drop functionality
    if (GApp::onEvent(e)) {
        return true;
    }
    
    return false;    
}


void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    GApp::onSimulation(rdt, sdt, idt);

    // Make the camera spin when the debug controller is not active
    if (false) {
        static float angle = 0;
        angle += (float)rdt;
        const float radius = 5.5f;
        m_debugCamera->setPosition(Vector3(cos(angle), 0, sin(angle)) * radius);
        m_debugCamera->lookAt(Vector3(0,0,0));
    }

    // let viewer sim with time step if needed
    if (viewer != NULL) {
        viewer->onSimulation(rdt, sdt, idt);
    }
}


void App::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& posed3D) {
	rd->pushState(m_framebuffer); {
		shared_ptr<LightingEnvironment> localLighting = lighting;
        localLighting->ambientOcclusion = m_ambientOcclusion;
		rd->setProjectionAndCameraMatrix(m_debugCamera->projection(), m_debugCamera->frame());

		rd->setColorClearValue(colorClear);
		rd->clear(true, true, true);

		// Render the file that is currently being viewed
		if (viewer != NULL) {
			viewer->onGraphics3D(rd, this, localLighting, posed3D);
		}

	} rd->popState();

    // Perform gamma correction, bloom, and SSAA, and write to the native window frame buffer
    m_film->exposeAndRender(rd, m_debugCamera->filmSettings(), m_framebuffer->texture(0), settings().hdrFramebuffer.colorGuardBandThickness.x + settings().hdrFramebuffer.depthGuardBandThickness.x, settings().hdrFramebuffer.colorGuardBandThickness.x); 

}

void App::onPose(Array<shared_ptr<Surface> >& posed3D, Array<shared_ptr<Surface2D> >& posed2D) {
    GApp::onPose(posed3D, posed2D);

    // Append any models to the arrays that you want to later be rendered by onGraphics()
    if (viewer != NULL) {
        viewer->onPose(posed3D, posed2D);
    }
}


void App::onGraphics2D(RenderDevice *rd, Array< shared_ptr<Surface2D> > &surface2D) {
    viewer->onGraphics2D(rd, this);
    GApp::onGraphics2D(rd, surface2D);

}


void App::setViewer(const G3D::String& newFilename) {
    logPrintf("App::setViewer(\"%s\")\n", filename.c_str());
    drawMessage("Loading " + newFilename);
    filename = newFilename;

    m_debugCamera->setFrame(CFrame::fromXYZYPRDegrees(-11.8f,  25.2f,  31.8f, -23.5f, -39.0f,   0.0f));
    m_debugController->setFrame(m_debugCamera->frame());

    //modelController->setFrame(CoordinateFrame(Matrix3::fromAxisAngle(Vector3(0,1,0), toRadians(180))));
    delete viewer;
    viewer = NULL;

    if (filename == "<events>") {
        viewer = new EventViewer();
    } else {
        G3D::String ext = toLower(filenameExt(filename));
        G3D::String base = toLower(filenameBase(filename));
        
        if ((ext == "3ds")  ||
            (ext == "ifs")  ||
            (ext == "obj")  ||
            (ext == "ply2") ||
            (ext == "off")  ||
            (ext == "ply")  ||
            (ext == "bsp")  ||
            (ext == "stl")  ||
            (ext == "lwo")  ||
            (ext == "stla") ||
            (ext == "dae")  ||
            (ext == "fbx")  ||
            (ext == "any" && endsWith(base, ".material")) ||
            (ext == "any" && endsWith(base, ".universalmaterial")) ||
            ((ext == "any" && endsWith(base, ".am")) ||
             (ext == "any" && endsWith(base, ".articulatedmodel")))) {
            
            showDebugText = false;
            viewer = new ArticulatedViewer();
            
        } else if (Texture::isSupportedImage(filename)) {
            
            // Images can be either a Texture or a Sky, TextureViewer will figure it out
            viewer = new TextureViewer();
            
            // Angle the camera slightly so a sky/cube map doesn't see only 1 face
            m_debugController->setFrame(Matrix3::fromAxisAngle(Vector3::unitY(), (float)halfPi() / 2.0f) * Matrix3::fromAxisAngle(Vector3::unitX(), (float)halfPi() / 2.0f));
            
        } else if (ext == "fnt") {
            
            viewer = new FontViewer(debugFont);
            
/*        } else if (ext == "bsp") {
            
            viewer = new BSPViewer();
  */          
        } else if (ext == "md2") {
            
            viewer = new MD2Viewer();
            
        } else if (ext == "md3") {

            viewer = new MD3Viewer();

        } else if (ext == "gtm") {
            
            viewer = new GUIViewer(this);
            
        } else if (ext == "icn") {
            
            viewer = new IconSetViewer(debugFont);
            
        } else if (ext == "pk3") {
            // Something in Quake format - figure out what we should load
            Array <String> files;
            bool set = false;
            
            // First, try for a .bsp map
            G3D::String search = filename + "/maps/*";
            FileSystem::getFiles(search, files, true);
            
            for (int t = 0; t < files.length(); ++t) {
                
                if (filenameExt(files[t]) == "bsp") {
                    
                    filename = files[t];
                    viewer = new ArticulatedViewer();
                    set = true;
                }
            }
            if (! set) {
                viewer = new EmptyViewer();
            }
            
        } else if (ext == "avi" || ext == "wmv" || ext == "mp4" || ext == "asf" || 
                   (ext == "mov") || (ext == "dv") || (ext == "qt") || (ext == "asf") ||
                   (ext == "mpg")) {
            viewer = new VideoViewer();
            
        } else {
            
            viewer = new EmptyViewer();
            
        }
    }
    
    if (viewer != NULL) {
        viewer->onInit(filename);
    }
    
    if (filename != "") {
        if (filename == "<events>") {
            window()->setCaption("Events - G3D Viewer");
        } else {
            window()->setCaption(filenameBaseExt(filename) + " - G3D Viewer");
        } 
    }
    
    logPrintf("Done App::setViewer(...)\n");
}
