/**
 \file MD3Viewer.cpp
 
 Viewer for Quake3 .md3 models
 
 \maintainer Daniel Evangelakos
 \author     Daniel Evangelakos 
 
 \created     June 18, 2013
 \edited      June 18, 2013
 */
#include "MD3Viewer.h"

static const float START_Y = 6.0f;
static const float START_Z = 13.0f;
static const float START_YAW = -90.0f;

void MD3Viewer::pose(RealTime dt){
    
    model->simulatePose(currentPose, dt);

}

void MD3Viewer::onPose(Array<shared_ptr<Surface> >& posed3D, Array<shared_ptr<Surface2D> >& posed2D) {
    model->pose(posed3D, cframe, currentPose);
}

void MD3Viewer::onInit(const G3D::String& filename) {
    cframe = CFrame::fromXYZYPRDegrees(0, START_Y, START_Z, START_YAW);
	m_skybox     = Texture::fromFile(FilePath::concat(System::findDataFile("whiteroom"), "whiteroom-*.png"), ImageFormat::SRGB8(), Texture::DIM_CUBE_MAP);
    MD3Model::Specification spec;
    size_t pos = filename.find_last_of("\\");
    G3D::String dir = filename.substr(0, pos);
    spec.directory = dir;
    spec.defaultSkin = MD3Model::Skin::create(dir, "default");
    model = MD3Model::create(spec);
}


void MD3Viewer::onGraphics3D(RenderDevice* rd, App* app, const shared_ptr<LightingEnvironment>& lighting, Array<shared_ptr<Surface> >& posed3D) {
    pose(app->previousSimTimeStep());
        
    Draw::skyBox(rd, m_skybox);

    LightingEnvironment env;

    // Cull and sort
    Array<shared_ptr<Surface> > sortedVisibleSurfaces;
    Surface::cull(app->activeCamera()->frame(), app->activeCamera()->projection(), rd->viewport(), posed3D, sortedVisibleSurfaces);
    Surface::sortBackToFront(sortedVisibleSurfaces, app->activeCamera()->frame().lookVector());

    // Early-Z pass
    Surface::renderDepthOnly(rd, sortedVisibleSurfaces, CullFace::BACK);

    // Compute AO
    app->ambientOcclusion()->update(rd, lighting->ambientOcclusionSettings, app->activeCamera(), rd->framebuffer()->get(Framebuffer::DEPTH)->texture());

    env.lightArray = lighting->lightArray;
    env.ambientOcclusion = app->ambientOcclusion();
    for (int i = sortedVisibleSurfaces.size() - 1; i >= 0; --i) {
        sortedVisibleSurfaces[i]->render(rd, env, RenderPassType::OPAQUE_SAMPLES, "");
    }

    rd->setDepthWrite(false);
    for (int i = 0; i < sortedVisibleSurfaces.size(); ++i) {
        sortedVisibleSurfaces[i]->render(rd, env, RenderPassType::MULTIPASS_BLENDED_SAMPLES, Surface::defaultWritePixelDeclaration());
    }
    float x, y, z, yaw, pitch, roll;
    app->activeCamera()->frame().getXYZYPRDegrees(x,y,z,yaw, pitch, roll);
    screenPrintf("[Camera position: Translation(%f, %f, %f) Rotation(%f, %f, %f)]\n", x,y,z,yaw,pitch,roll);
}
