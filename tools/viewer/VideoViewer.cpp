/**
   @file FontViewer.cpp
 
   Viewer for supported video files.
 
*/
#include "VideoViewer.h"


VideoViewer::VideoViewer() {
}


void VideoViewer::onInit(const G3D::String& filename) {

    m_video = VideoInput::fromFile(filename, VideoInput::Settings());
    m_frame = 0;
}


void VideoViewer::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {

    if (notNull(m_video) && ! m_video->finished()) {
        m_video->readFromIndex(++m_frame, m_videoTexture, true);
    }
}


void VideoViewer::onGraphics2D(RenderDevice* rd, App* app) {
    // set clear color
    app->colorClear = Color3::white();
    if (notNull(m_videoTexture)) {
        Draw::rect2D(rd->viewport().largestCenteredSubRect(float(m_videoTexture->width()), float(m_videoTexture->height())), 
	        rd, Color3::white(), m_videoTexture);
    }

    if (m_video) {
        screenPrintf("Video: %d x %d", m_video->width(), m_video->height());
    } else {
        screenPrintf("Video: not supported");
    }

    screenPrintf("Window: %d x %d", rd->width(), rd->height());
}
