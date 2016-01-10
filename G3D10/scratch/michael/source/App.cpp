#include "App.h"
#include "AZDORenderer.h"
G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
    GApp::Settings settings(argc, argv);

    settings.window.width = 1280;
    settings.window.height = 720;
    settings.window.caption = "AZDO Rendering?";
    settings.colorGuardBandThickness = Vector2int16(0, 0);
    settings.depthGuardBandThickness = Vector2int16(64, 64);

#   ifdef G3D_WINDOWS
    if (!FileSystem::exists("UniversalSurface_depthCombined.vrt", false)) {
        // Running on Windows, building from the G3D.sln project
        chdir("../scratch/michael/data-files");
    }
#   endif

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {}


void App::onInit() {
    GApp::onInit();

    m_gbufferSpecification.encoding[GBuffer::Field::CS_FACE_NORMAL].format = NULL;

    m_gbufferSpecification.encoding[GBuffer::Field::CS_NORMAL] = Texture::Encoding(ImageFormat::RGB10A2(), FrameName::CAMERA, 2.0f, -1.0f);

    m_gbufferSpecification.encoding[GBuffer::Field::SS_EXPRESSIVE_MOTION] =
        Texture::Encoding(GLCaps::supportsTexture(ImageFormat::RG8()) ? ImageFormat::RG8() : ImageFormat::RGBA8(),
        FrameName::SCREEN, 128.0f, -64.0f);

    m_gbufferSpecification.encoding[GBuffer::Field::EMISSIVE] =
        GLCaps::supportsTexture(ImageFormat::RGB5()) ?
        Texture::Encoding(ImageFormat::RGB5(), FrameName::NONE, 3.0f, 0.0f) :
        Texture::Encoding(ImageFormat::R11G11B10F());

    m_gbufferSpecification.encoding[GBuffer::Field::LAMBERTIAN] = ImageFormat::RGB8();
    m_gbufferSpecification.encoding[GBuffer::Field::GLOSSY] = ImageFormat::RGBA8();
    m_gbufferSpecification.encoding[GBuffer::Field::DEPTH_AND_STENCIL] = ImageFormat::DEPTH32F();
    m_gbufferSpecification.depthEncoding = DepthEncoding::HYPERBOLIC;

    // Update the actual m_gbuffer before makeGUI creates the buffer visualizer
    m_gbuffer->setSpecification(m_gbufferSpecification);
    m_gbuffer->resize(renderDevice->width() + m_settings.depthGuardBandThickness.x * 2, renderDevice->height() + m_settings.depthGuardBandThickness.y * 2);
    dynamic_pointer_cast<DefaultRenderer>(m_renderer)->setDeferredShading(true);
    m_otherRenderer = m_renderer;
    m_renderer = AZDORenderer::create();
    dynamic_pointer_cast<AZDORenderer>(m_renderer)->setDeferredShading(true);
    

    renderDevice->setSwapBuffersAutomatically(false);

    makeGUI();
    loadScene("G3D Sponza");// developerWindow->sceneEditorWindow->selectedSceneName());
    //loadScene("C:/Users/Michael/Projects/mcguire-assets/model/sanMiguel/sanMiguel.Scene.Any");
}


void App::makeGUI() {
    createDeveloperHUD();

    debugWindow->setVisible(true);
    developerWindow->setVisible(false);
    developerWindow->cameraControlWindow->setVisible(false);
    showRenderingStats = false;

    debugPane->addButton("Swap Renderers", [&](){shared_ptr<Renderer> temp = m_renderer; m_renderer = m_otherRenderer; m_otherRenderer = temp; });
    debugPane->pack();
}

