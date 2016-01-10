/** \file App.cpp */
#include "App.h"

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
    (void)argc; (void)argv;
    GApp::Settings settings(argc, argv);
    
    // Change the window and other startup parameters by modifying the
    // settings class.  For example:
    settings.window.width       = 512; 
    settings.window.height      = 512;

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {
    renderDevice->setColorClearValue(Color3::white());
}


void App::onInit() {
    GApp::onInit();
    //load the texture_2D_Array from files
    waterArray = Texture::fromFile(System::findDataFile("gobo\\waterCaustic\\waterCaustic_*.jpg"),ImageFormat::SRGB8());
    time = 0;
}


bool App::onEvent(const GEvent& e) {
    if (GApp::onEvent(e)) {
        return true;
    }
    return false;
}


void App::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface3D) {
    rd->push2D(m_framebuffer); {
        Args args;
        time += 0.15f;
        //bind the texture_2D_Array as a normal buffer
        args.setUniform("textureArray", waterArray, Sampler::buffer());
        args.setUniform("time", time);
        args.setUniform("bounds", m_framebuffer->vector2Bounds());
        args.setRect(m_framebuffer->rect2DBounds());
        LAUNCH_SHADER("TextureArraySample.pix", args);
    } rd->pop2D();
    swapBuffers();
    m_film->exposeAndRender(rd, activeCamera()->filmSettings(), m_framebuffer->texture(0));
}


void App::onGraphics2D(RenderDevice* rd, Array<Surface2D::Ref>& posed2D) {
    // Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction
    Surface2D::sortAndRender(rd, posed2D);
}
