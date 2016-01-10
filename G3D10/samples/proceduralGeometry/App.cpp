#include "App.h"

G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
#   ifdef G3D_WINDOWS
    if (! FileSystem::exists("ground.Scene.Any", false)) {
        // Running on Windows, building from the G3D.sln project
        chdir("../samples/proceduralGeometry");
    }
#   endif

    GApp::Settings settings(argc, argv);

    settings.window.caption            = "G3D CPU Procedural Geometry Sample";
    settings.window.width              = 1024; settings.window.height       = 768;
    settings.window.resizable          = true;
    settings.dataDir                   = FileSystem::currentDirectory();
    settings.screenshotDirectory       = FileSystem::currentDirectory();

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {
}


shared_ptr<Model> App::createTorusModel() {
    const shared_ptr<ArticulatedModel>& model = ArticulatedModel::createEmpty("torusModel");

    ArticulatedModel::Part*     part      = model->addPart("root");
    ArticulatedModel::Geometry* geometry  = model->addGeometry("geom");
    ArticulatedModel::Mesh*     mesh      = model->addMesh("mesh", part, geometry);

    // Assign a material
    mesh->material = UniversalMaterial::create(
        PARSE_ANY(
        UniversalMaterial::Specification {
            lambertian = Texture::Specification {
                filename = "image/checker-32x32-1024x1024.png";
                // Orange
                encoding = Color3(1.0, 0.7, 0.15);
            };

            glossy     = Color4(Color3(0.01), 0.2);
        }));

    // Create the vertices and faces in the following unwrapped pattern:
    //     ___________
    //    |  /|  /|  /|   
    //    |/__|/__|/__|
    // ^  |  /|  /|  /|
    // |  |/__|/__|/__|
    // p    
    //    t ->

    const int   smallFaces  = 20;
    const float smallRadius = 0.5f * units::meters();

    const int   largeFaces  = 50;
    const float largeRadius = 2.0f * units::meters();

    Array<CPUVertexArray::Vertex>& vertexArray = geometry->cpuVertexArray.vertex;
    Array<int>& indexArray = mesh->cpuIndexArray;

    for (int t = 0; t <= largeFaces; ++t) {
        
        const float   thetaDegrees   = 360.0f * t / float(largeFaces);
        const CFrame& smallRingFrame = (Matrix4::yawDegrees(thetaDegrees) * Matrix4::translation(largeRadius, 0.0f, 0.0f)).approxCoordinateFrame();

        for (int p = 0; p <= smallFaces; ++p) {
            const float phi = 2.0f * pif() * p / float(smallFaces);

            CPUVertexArray::Vertex& v = vertexArray.next();
            v.position = smallRingFrame.pointToWorldSpace(Point3(cos(phi), sin(phi), 0.0f) * smallRadius);

            v.texCoord0 = Point2(4.0f * t / float(largeFaces), p / float(smallFaces));

            // Set to NaN to trigger automatic vertex normal and tangent computation
            v.normal  = Vector3::nan();
            v.tangent = Vector4::nan();

            if ((t < largeFaces) && (p < smallFaces)) {
                // Create the corresponding face out of two triangles.
                // Because the texture coordinates are unique, we can't
                // wrap the geometry around and instead duplicate vertices
                // along the two seams.
                //
                // D-----C
                // |   / |
                // | /   |
                // A-----B
                const int A = (t + 0) * (smallFaces + 1) + (p + 0);
                const int B = (t + 1) * (smallFaces + 1) + (p + 0);
                const int C = (t + 1) * (smallFaces + 1) + (p + 1);
                const int D = (t + 0) * (smallFaces + 1) + (p + 1);
                indexArray.append(
                    A, B, C,
                    C, D, A);
            }
        } // p 
    } // t

    // Tell the ArticulatedModel to generate bounding boxes, GPU vertex arrays,
    // normals and tangents automatically. We already ensured correct
    // topology, so avoid the vertex merging optimization.
    ArticulatedModel::CleanGeometrySettings geometrySettings;
    geometrySettings.allowVertexMerging = false;
    model->cleanGeometry(geometrySettings);

    return model;
}


void App::addTorusToScene() {
    // Replace any existing torus model. Models don't 
    // have to be added to the model table to use them 
    // with a VisibleEntity.
    const shared_ptr<Model>& torusModel = createTorusModel();
    if (scene()->modelTable().containsKey(torusModel->name())) {
        scene()->removeModel(torusModel->name());
    }
    scene()->insert(torusModel);

    // Replace any existing torus entity that has the wrong type
    shared_ptr<Entity> torus = scene()->entity("torus");
    if (notNull(torus) && isNull(dynamic_pointer_cast<VisibleEntity>(torus))) {
        logPrintf("The scene contained an Entity named 'torus' that was not a VisibleEntity\n");
        scene()->remove(torus);
        torus.reset();
    }

    if (isNull(torus)) {
        // There is no torus entity in this scene, so make one.
        //
        // We could either explicitly instantiate a VisibleEntity or simply
        // allow the Scene parser to construct one. The second approach
        // has more consise syntax for this case, since we are using all constant
        // values in the specification.
        torus = scene()->createEntity("torus",
            PARSE_ANY(
                VisibleEntity {
                    model = "torusModel";
                };
            ));
    } else {
        // Change the model on the existing torus entity
        dynamic_pointer_cast<VisibleEntity>(torus)->setModel(torusModel);
    }

    torus->setFrame(CFrame::fromXYZYPRDegrees(0.0f, 1.8f, 0.0f, 45.0f, 45.0f));
}


void App::onInit() {
    GApp::onInit();
    setFrameDuration(1.0f / 60.0f);
    showRenderingStats      = false;
    createDeveloperHUD();
    developerWindow->cameraControlWindow->moveTo(Point2(developerWindow->cameraControlWindow->rect().x0(), 0));

    loadScene("Ground");
    addTorusToScene();
}
