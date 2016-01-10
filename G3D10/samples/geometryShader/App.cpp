/** \file App.cpp */
#include "App.h"

G3D_START_AT_MAIN();

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    GApp::Settings settings;
    
    settings.window.width       = 960; 
    settings.window.height      = 600;
    settings.window.caption     = "Geometry Shader Demo";

#   ifdef G3D_WINDOWS
        // On Unix operating systems, icompile automatically copies data files.  
        // On Windows, we just run from the data directory.
        if (FileSystem::exists("data-files")) {
            chdir("data-files");
        } else if (FileSystem::exists("../samples/geometryShader/data-files")) {
            chdir("../samples/geometryShader/data-files");
        }

#   endif

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {}


void App::onInit() {
    ArticulatedModel::Specification spec;
    spec.filename       = System::findDataFile("teapot/teapot.obj");
    spec.stripMaterials = true;
    spec.scale          = 0.035f;
	renderDevice->setSwapBuffersAutomatically(true);

    shared_ptr<ArticulatedModel> model = ArticulatedModel::create(spec);
    model->pose(m_sceneGeometry, Point3(0, -1.7f, 0));

    createDeveloperHUD();
}


void App::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface3D) {
    rd->setColorClearValue(Color3::white() * 0.3f);
    rd->clear();

    // Draw the extruded geometry as colored wireframe with "glass" interior
    rd->pushState();
    rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);
    rd->setDepthWrite(false);
    Args args;
    args.setUniform("intensity", 0.1f); 
    CFrame cframe;
    for (int i = 0; i < m_sceneGeometry.size(); ++i) {
        const shared_ptr<UniversalSurface>& surface = dynamic_pointer_cast<UniversalSurface>(m_sceneGeometry[i]);
        if (surface) {
            surface->getCoordinateFrame(cframe);
            args.setUniform("MVP", 
                rd->invertYMatrix() * rd->projectionMatrix() * (rd->cameraToWorldMatrix().inverse() * 
                cframe));
            surface->gpuGeom()->setShaderArgs(args);
            LAUNCH_SHADER("extrude.*", args);
        }
    }
    rd->popState();

    rd->pushState();
    rd->setRenderMode(RenderDevice::RENDER_WIREFRAME);
    rd->setCullFace(CullFace::NONE);

    args.setUniform("intensity", 1.0f); 
    for (int i = 0; i < m_sceneGeometry.size(); ++i) {
        const shared_ptr<UniversalSurface>& surface = dynamic_pointer_cast<UniversalSurface>(m_sceneGeometry[i]);
        if (notNull(surface)) {

            surface->getCoordinateFrame(cframe);
            args.setUniform("MVP", rd->invertYMatrix() * rd->projectionMatrix() * (rd->cameraToWorldMatrix().inverse() * cframe));
            surface->gpuGeom()->setShaderArgs(args);

            LAUNCH_SHADER("extrude.*", args);
        }
    }
    rd->popState();

    drawDebugShapes();
}
