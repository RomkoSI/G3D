/** \file App.cpp */
#include "App.h"

void PhysXWorld::TriTree::setContents
(const Array<shared_ptr<Surface>>&  surfaceArray, 
 ImageStorage                       newStorage) {

    clear();

    const bool computePrevPosition = false;
    Surface::getTris(surfaceArray, m_cpuVertexArray, m_triArray, computePrevPosition);
    alwaysAssertM(m_cpuVertexArray.vertex.size() == m_cpuVertexArray.vertex.capacity(), 
                  "Allocated too much memory for the vertex array");
   
    if (newStorage != IMAGE_STORAGE_CURRENT) {
        for (int i = 0; i < m_triArray.size(); ++i) {
            const Tri& tri = m_triArray[i];
            const shared_ptr<Material>& material = tri.material();
            material->setStorage(newStorage);
        }
    }

    if (m_cpuVertexArray.size() == 0) {
        return;
    }

    PxTriangleMeshDesc meshDesc;
    meshDesc.points.count           = m_cpuVertexArray.size();
    meshDesc.points.stride          = sizeof(CPUVertexArray::Vertex);
    meshDesc.points.data            = &m_cpuVertexArray.vertex[0].position;

    // Triangle indices are not packed with uniform stride in the m_triArray, so 
    // we must copy them here.
    meshDesc.triangles.count        = m_triArray.size();
    meshDesc.triangles.stride       = sizeof(int);
    {
        int* indexPtr = new int[m_triArray.size() * 3];
        meshDesc.triangles.data = indexPtr;
        for (int t = 0; t < m_triArray.size(); ++t) {
            const Tri& tri = m_triArray[t];
            for (int i = 0; i < 3; ++i, ++indexPtr) {
                *indexPtr = tri.index[i];
            }
        }
    }

    PxDefaultMemoryOutputStream writeBuffer;
    const bool status = m_world->cooking->cookTriangleMesh(meshDesc, writeBuffer);
    if (! status) {
        throw "Unable to cook triangle mesh";
    }

    PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
    PxTriangleMesh* mesh = m_world->physics->createTriangleMesh(readBuffer);

    // Free the copied indices used for construction
    delete[] meshDesc.triangles.data;
    meshDesc.triangles.data = nullptr;

    m_geometry = new PxTriangleMeshGeometry(mesh);
}

shared_ptr<Surfel> PhysXWorld::TriTree::intersectRay(const Ray& ray, float& distance, bool exitOnAnyHit, bool twoSided) const {
    Tri::Intersector intersector;
    if (intersectRay(ray, intersector, distance, exitOnAnyHit, twoSided)) {
        return intersector.tri->material()->sample(intersector);
    } else {
        return shared_ptr<Surfel>();
    }
}


bool PhysXWorld::TriTree::intersectRay(const Ray &ray, Tri::Intersector& intersectCallback, float& distance, bool exitOnAnyHit, bool twoSided) const {
    PxRaycastHit hitInfo;
    const PxU32 maxHits = 1;
    const PxHitFlags hitFlags = PxHitFlag::eDISTANCE | (exitOnAnyHit ? PxHitFlag::eMESH_ANY : PxHitFlag::Enum(0));
    const PxU32 hitCount = PxGeometryQuery::raycast(toPxVec3(ray.origin()), toPxVec3(ray.direction()), *const_cast<TriTree*>(this)->m_geometry, PxTransform(PxVec3(0, 0, 0)), distance, hitFlags, maxHits, &hitInfo, exitOnAnyHit);
    if (hitCount > 0) {
        // TODO: run the intersector
        // TODO: if the intersector reports no intersection and hitInfo.distance < distance, continue the ray

        distance = hitInfo.distance;

        // TODO: Do we have to run the face index through the remapper table?
        intersectCallback.primitiveIndex = hitInfo.faceIndex;
        intersectCallback.cpuVertexArray = &m_cpuVertexArray;
        return true;
    } else {
        return false;
    }
}


void PhysXWorld::TriTree::clear() {
    m_triArray.fastClear();
    m_cpuVertexArray.clear();

    if (notNull(m_geometry)) {
        m_geometry->triangleMesh->release();
        delete m_geometry;
        m_geometry = nullptr;
    }
}


PhysXWorld::TriTree::~TriTree() {
    clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PxTriangleMesh* PhysXWorld::cookTriangleMesh(const Array<Vector3>& vertices, const Array<int>& indices) {
    // http://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/guide/Manual/Geometry.html
    PxTriangleMeshDesc meshDesc;
    meshDesc.points.count           = vertices.size();
    meshDesc.points.stride          = sizeof(Vector3);
    meshDesc.points.data            = vertices.getCArray();

    meshDesc.triangles.count        = indices.size() / 3;
    meshDesc.triangles.stride       = 3 * sizeof(int);
    meshDesc.triangles.data         = indices.getCArray();

    debugPrintf("vertices.size() = %d\n", vertices.size());
    debugPrintf("indices.size() = %d\n",  indices.size());

    PxDefaultMemoryOutputStream writeBuffer;
    const bool status = cooking->cookTriangleMesh(meshDesc, writeBuffer);
    if (! status) {
        alwaysAssertM(false, "Unable to cook triangle mesh");
        return nullptr;
    }

    PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
    return physics->createTriangleMesh(readBuffer);
}


PhysXWorld::PhysXWorld() {
    static PxDefaultErrorCallback gDefaultErrorCallback;
    static PxDefaultAllocator gDefaultAllocatorCallback;

    foundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback, gDefaultErrorCallback);

    alwaysAssertM(notNull(foundation), "PxCreateFoundation failed!");

    profileZoneManager = &PxProfileZoneManager::createProfileZoneManager(foundation);
    alwaysAssertM(notNull(profileZoneManager), "PxProfileZoneManager::createProfileZoneManager failed!");

    PxTolerancesScale scale;
    scale.length = 1.0f;        // typical length of an object
    scale.speed  = 9.81f;       // typical speed of an object, gravity*1s is a reasonable choice

    const bool recordMemoryAllocations = false;
    physics = PxCreateBasePhysics(PX_PHYSICS_VERSION, *foundation, scale, recordMemoryAllocations, profileZoneManager);
    alwaysAssertM(notNull(physics), "PxCreatePhysics failed!");

    cooking = PxCreateCooking(PX_PHYSICS_VERSION, *foundation, PxCookingParams(scale));
    alwaysAssertM(notNull(cooking), "PxCreateCooking failed!");

    PxSceneDesc sceneDesc(physics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    //customizeSceneDesc(sceneDesc);

    const int threadCount =
#       ifdef G3D_DEBUG
            1;
#       else
            max(2, GThread::numCores());
#       endif

    if (! sceneDesc.cpuDispatcher) {
        cpuDispatcher = PxDefaultCpuDispatcherCreate(threadCount);
        alwaysAssertM(notNull(cpuDispatcher),"PxDefaultCpuDispatcherCreate failed!");
        sceneDesc.cpuDispatcher    = cpuDispatcher;
    }

    if (! sceneDesc.filterShader) {
        sceneDesc.filterShader    = PxDefaultSimulationFilterShader;
    }

    scene = physics->createScene(sceneDesc);
    alwaysAssertM(notNull(scene), "createScene failed!");
    defaultMaterial = physics->createMaterial(0.5f, 0.5f, 0.6f);
}


PhysXWorld::~PhysXWorld() {
    physics->release();
    foundation->release();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

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

    // settings.window.width            =  854; settings.window.height       = 480;
    // settings.window.width            = 1024; settings.window.height       = 768;
     settings.window.width            = 1280; settings.window.height       = 720;
    //settings.window.width               = 1920; settings.window.height       = 1080;
    // settings.window.width            = OSWindow::primaryDisplayWindowSize().x; settings.window.height = OSWindow::primaryDisplayWindowSize().y;
    settings.window.fullScreen          = false;
    settings.window.resizable           = ! settings.window.fullScreen;
    settings.window.framed              = ! settings.window.fullScreen;

    // Set to true for a significant performance boost if your app can't render at 60fps,
    // or if you *want* to render faster than the display.
    settings.window.asynchronous        = false;

    settings.hdrFramebuffer.depthGuardBandThickness = Vector2int16(64, 64);
    settings.hdrFramebuffer.colorGuardBandThickness = Vector2int16(0, 0);
    settings.dataDir                    = FileSystem::currentDirectory();
//    settings.screenshotDirectory        = ".";

    settings.renderer.deferredShading = true;
    settings.renderer.orderIndependentTransparency = false;


    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {
}


// Called before the application loop begins.  Load data here and
// not in the constructor so that common exceptions will be
// automatically caught.
void App::onInit() {
    GApp::onInit();
    setFrameDuration(1.0f / 120.0f);

    m_physXWorld = shared_ptr<PhysXWorld>(new PhysXWorld());

    // Call setScene(shared_ptr<Scene>()) or setScene(MyScene::create()) to replace
    // the default scene here.
    
    showRenderingStats      = true;

    makeGUI();
    // For higher-quality screenshots:
    // developerWindow->videoRecordDialog->setScreenShotFormat("PNG");
    // developerWindow->videoRecordDialog->setCaptureGui(false);
    developerWindow->cameraControlWindow->moveTo(Point2(developerWindow->cameraControlWindow->rect().x0(), 0));
    loadScene(
         //"G3D Sponza"
        "G3D Cornell Box" // Load something simple
        //developerWindow->sceneEditorWindow->selectedSceneName()  // Load the first scene encountered 
        );
}


void App::makeGUI() {
    // Initialize the developer HUD (using the existing scene)
    createDeveloperHUD();
    debugWindow->setVisible(true);
    developerWindow->videoRecordDialog->setEnabled(true);

    GuiPane* infoPane = debugPane->addPane("Info", GuiTheme::ORNATE_PANE_STYLE);
    
    // Example of how to add debugging controls
    infoPane->addLabel("You can add GUI controls");
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
    if (!scene()) {
        if ((submitToDisplayMode() == SubmitToDisplayMode::MAXIMIZE_THROUGHPUT) && (!rd->swapBuffersAutomatically())) {
            swapBuffers();
        }
        rd->clear();
        rd->pushState(); {
            rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
            drawDebugShapes();
        } rd->popState();
        return;
    }

    GBuffer::Specification gbufferSpec = m_gbufferSpecification;
    extendGBufferSpecification(gbufferSpec);
    m_gbuffer->setSpecification(gbufferSpec);
    m_gbuffer->resize(m_framebuffer->width(), m_framebuffer->height());
    m_gbuffer->prepare(rd, activeCamera(), 0, -(float)previousSimTimeStep(), m_settings.hdrFramebuffer.depthGuardBandThickness, m_settings.hdrFramebuffer.colorGuardBandThickness);

    m_renderer->render(rd, m_framebuffer, scene()->lightingEnvironment().ambientOcclusionSettings.enabled ? m_depthPeelFramebuffer : shared_ptr<Framebuffer>(),
        scene()->lightingEnvironment(), m_gbuffer, allSurfaces);

    // Debug visualizations and post-process effects
    rd->pushState(m_framebuffer); {
        // Call to make the App show the output of debugDraw(...)
        rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
        drawDebugShapes();
        const shared_ptr<Entity>& selectedEntity = (notNull(developerWindow) && notNull(developerWindow->sceneEditorWindow)) ? developerWindow->sceneEditorWindow->selectedEntity() : shared_ptr<Entity>();
        scene()->visualize(rd, selectedEntity, allSurfaces, sceneVisualizationSettings(), activeCamera());

        // Post-process special effects
        m_depthOfField->apply(rd, m_framebuffer->texture(0), m_framebuffer->texture(Framebuffer::DEPTH), activeCamera(), m_settings.hdrFramebuffer.depthGuardBandThickness - m_settings.hdrFramebuffer.colorGuardBandThickness);

        m_motionBlur->apply(rd, m_framebuffer->texture(0), m_gbuffer->texture(GBuffer::Field::SS_EXPRESSIVE_MOTION),
            m_framebuffer->texture(Framebuffer::DEPTH), activeCamera(),
            m_settings.hdrFramebuffer.depthGuardBandThickness - m_settings.hdrFramebuffer.colorGuardBandThickness);
    } rd->popState();

    // We're about to render to the actual back buffer, so swap the buffers now.
    // This call also allows the screenshot and video recording to capture the
    // previous frame just before it is displayed.
    if (submitToDisplayMode() == SubmitToDisplayMode::MAXIMIZE_THROUGHPUT) {
        swapBuffers();
    }

    // Clear the entire screen (needed even though we'll render over it, since
    // AFR uses clear() to detect that the buffer is not re-used.)
    rd->clear();

    // Perform gamma correction, bloom, and SSAA, and write to the native window frame buffer
    m_film->exposeAndRender(rd, activeCamera()->filmSettings(), m_framebuffer->texture(0), settings().hdrFramebuffer.colorGuardBandThickness.x + settings().hdrFramebuffer.depthGuardBandThickness.x, settings().hdrFramebuffer.depthGuardBandThickness.x);
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
