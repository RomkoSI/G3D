/** \file App.cpp */
#include "App.h"

G3D_START_AT_MAIN();

#ifdef G3D_OSX
#  error "This sample is not supported on OS X because that operating system does not support OpenGL 4.2"
#endif

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    GApp::Settings settings;
    
    settings.window.width       = 960; 
    settings.window.height      = 600;
    settings.window.caption     = "All Stages Shader Demo";

#   ifdef G3D_WINDOWS
        // On Unix operating systems, icompile automatically copies data files.  
        // On Windows, we just run from the data directory.
        if (FileSystem::exists("data-files")) {
            chdir("data-files");
        } else if (FileSystem::exists("../samples/allStagesShader/data-files")) {
            chdir("../samples/allStagesShader/data-files");
        }

#   endif

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings), m_innerTessLevel(1.0f), m_outerTessLevel(1.0f) {}

void App::makeGUI() {
    createDeveloperHUD();
    debugWindow->setVisible(true);
    
    GuiNumberBox<float>* innerSlider = debugPane->addNumberBox("Inner Tesselation Level", &m_innerTessLevel, "", GuiTheme::LINEAR_SLIDER, 1.0f, 20.f);
    innerSlider->setWidth(290.0f);
    innerSlider->setCaptionWidth(140.0f);
    GuiNumberBox<float>* outerSlider = debugPane->addNumberBox("Outer Tesselation Level", &m_outerTessLevel, "", GuiTheme::LINEAR_SLIDER, 1.0f, 20.f);
    outerSlider->setCaptionWidth(140.0f);
    outerSlider->setWidth(290.0f);
    debugPane->pack();
    debugWindow->pack();
    debugWindow->setRect(Rect2D::xywh(0, window()->height()-debugWindow->rect().height(), 300, debugWindow->rect().height()));
}


void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    GApp::onSimulation(rdt, sdt, idt);
}


void App::onInit() {
    ArticulatedModel::Specification spec;
    spec.filename       = System::findDataFile("icosahedron/icosahedron.obj");
	renderDevice->setSwapBuffersAutomatically(true);

    shared_ptr<ArticulatedModel> model = ArticulatedModel::create(spec);
    model->pose(m_sceneGeometry);

    m_allStagesShader = Shader::fromFiles("geodesic.vrt", "geodesic.ctl", "geodesic.evl", "geodesic.geo", "geodesic.pix");

    makeGUI();
}


void App::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface3D) {
    // Bind the main framebuffer
    rd->pushState(m_framebuffer); {

        rd->setColorClearValue(Color3::white() * 0.3f);
        rd->clear();
        
        
        rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
        Args args;
        args.setUniform("TessLevelInner", m_innerTessLevel);
        args.setUniform("TessLevelOuter", m_outerTessLevel);
        args.setPrimitiveType(PrimitiveType::PATCHES);
        args.patchVertices = 3;
        rd->setDepthTest(RenderDevice::DEPTH_LEQUAL);
        rd->setProjectionAndCameraMatrix(m_debugCamera->projection(), m_debugCamera->frame());
        
        for (int i = 0; i < m_sceneGeometry.size(); ++i) {
            const shared_ptr<UniversalSurface>&  surface = dynamic_pointer_cast<UniversalSurface>(m_sceneGeometry[i]);
            if (isNull(surface)) {
                debugPrintf("Surface %d, not a supersurface.\n", i);
                continue;
            }
            const shared_ptr<UniversalSurface::GPUGeom>& gpuGeom = surface->gpuGeom();
            args.setAttributeArray("Position", gpuGeom->vertex);
            args.setIndexStream(gpuGeom->index);
            
            CoordinateFrame cf;
            surface->getCoordinateFrame(cf);
            rd->setObjectToWorldMatrix(cf);
            
            rd->apply(m_allStagesShader, args);
        }
        
    } rd->popState();
    
    // Perform gamma correction, bloom, and SSAA, and write to the native window frame buffer
    m_film->exposeAndRender(rd, m_debugCamera->filmSettings(), m_framebuffer->texture(0), 1);
}


bool App::onEvent(const GEvent& event) {
    if (GApp::onEvent(event)) {
        return true;
    }

    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == 'r')) { 
        m_allStagesShader->reload(); 
        return true; 
    }

    return false;
}
