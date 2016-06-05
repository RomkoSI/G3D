/**
  \file VideoRecordDialog.cpp
  
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2008-10-18
  \edited  2014-07-24

 G3D Innovation Engine
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */ 
#include "G3D/platform.h"
#include "G3D/fileutils.h"
#include "G3D/Log.h"
#include "GLG3D/Draw.h"
#include "GLG3D/GApp.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/GuiCheckBox.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/VideoRecordDialog.h"
#include "G3D/svn_info.h"
#include "G3D/svnutils.h"
#include "G3D/FileSystem.h"
#include "GLG3D/ScreenshotDialog.h"

namespace G3D {

shared_ptr<VideoRecordDialog> VideoRecordDialog::create(const shared_ptr<GuiTheme>& theme, const String& prefix, GApp* app) {
    return shared_ptr<VideoRecordDialog>(new VideoRecordDialog(theme, prefix, app));
}


shared_ptr<VideoRecordDialog> VideoRecordDialog::create(const String& prefix, GApp* app) {
    alwaysAssertM(app, "GApp may not be NULL");
    return shared_ptr<VideoRecordDialog>(new VideoRecordDialog(app->debugWindow->theme(), prefix, app));
}


VideoRecordDialog::VideoRecordDialog(const shared_ptr<GuiTheme>& theme, const String& prefix, GApp* app) : 
    GuiWindow("Screen Capture", theme, Rect2D::xywh(0, 100, 320, 200),
              GuiTheme::DIALOG_WINDOW_STYLE, GuiWindow::HIDE_ON_CLOSE),
    m_app(app),
    m_templateIndex(0),
    m_playbackFPS(30),
    m_recordFPS(30),
    m_halfSize(true),
    m_enableMotionBlur(false),
    m_motionBlurFrames(10),
    m_screenshotPending(false),
    m_quality(1.0),
    m_framesBox(NULL),
    m_captureGUI(true),
    m_showCursor(false),
    m_filenamePrefix(prefix) {
    m_hotKey = GKey::F6;
    m_hotKeyMod = GKeyMod::NONE;
    m_hotKeyString = m_hotKey.toString();

    m_ssHotKey = GKey::F4;
    m_ssHotKeyMod = GKeyMod::NONE;
    m_ssHotKeyString = m_ssHotKey.toString();

    m_settingsTemplate.append(VideoOutput::Settings::MPEG4(640, 680));
    m_settingsTemplate.append(VideoOutput::Settings::WMV(640, 680));
    //m_settingsTemplate.append(VideoOutput::Settings::CinepakAVI(640, 680));
    m_settingsTemplate.append(VideoOutput::Settings::rawAVI(640, 680));

    // Remove unsupported formats and build a drop-down list
    for (int i = 0; i < m_settingsTemplate.size(); ++i) {
        if (! VideoOutput::supports(m_settingsTemplate[i].codec)) {
            m_settingsTemplate.remove(i);
            --i;
        } else {
            m_formatList.append(m_settingsTemplate[i].description);
        }
    }

    m_templateIndex = 0;
    // Default to MPEG4 since that combines quality and size
    for (int i = 0; i < m_settingsTemplate.size(); ++i) {
        if (m_settingsTemplate[i].codec == VideoOutput::CODEC_ID_MPEG4) {
            m_templateIndex = i;
            break;
        }
    }

    m_font = GFont::fromFile(System::findDataFile("arial.fnt"));

    makeGUI();
    
    m_recorder.reset(new Recorder());
    m_recorder->dialog = this;
}


void VideoRecordDialog::makeGUI() {
    pane()->addCheckBox("Record GUI (Surface2D)", &m_captureGUI);

    pane()->addLabel(GuiText("Video", shared_ptr<GFont>(), 12));
    GuiPane* moviePane = pane()->addPane("", GuiTheme::ORNATE_PANE_STYLE);

    GuiLabel* label = NULL;
    GuiDropDownList* formatList = moviePane->addDropDownList("Format", m_formatList, &m_templateIndex);

    const float width = 300.0f;
    // Increase caption size to line up with the motion blur box
    const float captionSize = 90.0f;

    formatList->setWidth(width);
    formatList->setCaptionWidth(captionSize);

    moviePane->addNumberBox("Quality", &m_quality, "", GuiTheme::LOG_SLIDER, 0.1f, 25.0f);
    
    if (false) {
        // For future expansion
        GuiCheckBox*  motionCheck = moviePane->addCheckBox("Motion Blur",  &m_enableMotionBlur);
        m_framesBox = moviePane->addNumberBox("", &m_motionBlurFrames, "frames", GuiTheme::LINEAR_SLIDER, 2, 20);
        m_framesBox->setUnitsSize(46);
        m_framesBox->moveRightOf(motionCheck);
        m_framesBox->setWidth(210);
    }

    GuiNumberBox<float>* recordBox   = moviePane->addNumberBox("Record as if",      &m_recordFPS, "fps", GuiTheme::NO_SLIDER, 1.0f, 120.0f, 0.1f);
    recordBox->setCaptionWidth(captionSize);

    GuiNumberBox<float>* playbackBox = moviePane->addNumberBox("Playback at",    &m_playbackFPS, "fps", GuiTheme::NO_SLIDER, 1.0f, 120.0f, 0.1f);
    playbackBox->setCaptionWidth(captionSize);

    const OSWindow* window = OSWindow::current();
    int w = window->width() / 2;
    int h = window->height() / 2;
    moviePane->addCheckBox(format("Half-size (%d x %d)", w, h), &m_halfSize);

    if (false) {
        // For future expansion
        moviePane->addCheckBox("Show cursor", &m_showCursor);
    }

    label = moviePane->addLabel("Hot key:");
    label->setWidth(captionSize);
    moviePane->addLabel(m_hotKeyString)->moveRightOf(label);

    // Add record on the same line as previous hotkey box
    m_recordButton = moviePane->addButton("Record Now (" + m_hotKeyString + ")");
    m_recordButton->moveBy(moviePane->rect().width() - m_recordButton->rect().width() - 5, -27);
    moviePane->pack();
    moviePane->setWidth(pane()->rect().width());

    ///////////////////////////////////////////////////////////////////////////////////
    pane()->addLabel(GuiText("Screenshot", shared_ptr<GFont>(), 12));
    GuiPane* ssPane = pane()->addPane("", GuiTheme::ORNATE_PANE_STYLE);

    m_ssFormatList.append("JPG", "PNG", "BMP", "TGA");
    GuiDropDownList* ssFormatList = ssPane->addDropDownList("Format", m_ssFormatList, &m_ssFormatIndex);
    m_ssFormatIndex = 0;

    ssFormatList->setWidth(width);
    ssFormatList->setCaptionWidth(captionSize);

    label = ssPane->addLabel("Hot key:");
    label->setWidth(captionSize);
    ssPane->addLabel(m_ssHotKeyString)->moveRightOf(label);

    ssPane->pack();
    ssPane->setWidth(pane()->rect().width());

    ///////////////////////////////////////////////////////////////////////////////////

    pack();
    setRect(Rect2D::xywh(rect().x0(), rect().y0(), rect().width() + 5, rect().height() + 2));
}


void VideoRecordDialog::onPose(Array<shared_ptr<Surface> >& posedArray, Array<shared_ptr<Surface2D> >& posed2DArray) {
    GuiWindow::onPose(posedArray, posed2DArray);
    if (m_video || m_screenshotPending) {
        if (m_captureGUI) {
            // Register with the App for a callback
            m_app->m_activeVideoRecordDialog = this;
        } else {
            posed2DArray.append(m_recorder);
            m_app->m_activeVideoRecordDialog = NULL;
        }
    }
}


void VideoRecordDialog::onAI () {
    if (m_framesBox) {
        m_framesBox->setEnabled(m_enableMotionBlur);
    }
}


void VideoRecordDialog::startRecording() {
    debugAssert(isNull(m_video));

    // Create the video file
    VideoOutput::Settings settings = m_settingsTemplate[m_templateIndex];
    OSWindow* window = const_cast<OSWindow*>(OSWindow::current());
    settings.width = window->width();
    settings.height = window->height();

    if (m_halfSize) {
        settings.width /= 2;
        settings.height /= 2;
    }

    double kps = 1000;
    double baseRate = 1500;
    if (settings.codec == VideoOutput::CODEC_ID_WMV2) {
        // WMV is lower quality
        baseRate = 3000;
    }
    settings.bitrate = iRound(m_quality * baseRate * kps * settings.width * settings.height / (640 * 480));
    settings.fps = m_playbackFPS;

    const String& filename = ScreenshotDialog::nextFilenameBase(m_filenamePrefix) + "." + m_settingsTemplate[m_templateIndex].extension;
    m_video = VideoOutput::create(filename, settings);

    if (m_app) {
        m_oldSimTimeStep = m_app->simStepDuration();
        m_oldRealTimeTargetDuration = m_app->realTimeTargetDuration();

        m_app->setFrameDuration(1.0f / m_recordFPS, GApp::MATCH_REAL_TIME_TARGET);
    }

    m_recordButton->setCaption("Stop (" + m_hotKeyString + ")");
    setVisible(false);

    // Change the window caption as well
    const String& c = window->caption();
    const String& appendix = " - Recording " + m_hotKeyString + " to stop";
    if (! endsWith(c, appendix)) {
        window->setCaption(c + appendix);
    }
}


void VideoRecordDialog::recordFrame(RenderDevice* rd) {
    debugAssert(m_video);

    if (m_halfSize) {
        // Half-size: downsample
        if (isNull(m_downsampleSrc)) {
            bool generateMipMaps = false;
            m_downsampleSrc = Texture::createEmpty("Downsample Source", 16, 16,
                                                   TextureFormat::RGB8(), Texture::DIM_2D, generateMipMaps);
        }
        RenderDevice::ReadBuffer old = rd->readBuffer();
        rd->setReadBuffer(RenderDevice::READ_BACK);
        rd->copyTextureFromScreen(m_downsampleSrc, Rect2D::xywh(0,0,float(rd->width()), float(rd->height())));
        rd->setReadBuffer(old);

        if (isNull(m_downsampleFBO)) {
            m_downsampleFBO = Framebuffer::create("Downsample Framebuffer");
        }

        if (isNull(m_downsampleDst) || 
            (m_downsampleDst->width() != m_downsampleSrc->width() / 2) || 
            (m_downsampleDst->height() != m_downsampleSrc->height() / 2)) {
            // Allocate destination
            const bool generateMipMaps = false;
            m_downsampleDst = 
                Texture::createEmpty("Downsample Destination", m_downsampleSrc->width() / 2, 
                                     m_downsampleSrc->height() / 2, 
                                     ImageFormat::RGB8(), Texture::DIM_2D, generateMipMaps);
            m_downsampleFBO->set(Framebuffer::COLOR0, m_downsampleDst);
        }

        // Downsample (use bilinear for fast filtering
        rd->push2D(m_downsampleFBO);
        {
            const Vector2& halfPixelOffset = Vector2(0.5f, 0.5f) / m_downsampleDst->vector2Bounds();
            Draw::rect2D(m_downsampleDst->rect2DBounds() + halfPixelOffset, rd, Color3::white(), m_downsampleSrc);
        }
        rd->pop2D();

        // Write downsampled texture to the video
        m_video->append(m_downsampleDst, rd->invertY());

    } else {
        // Full-size: grab directly from the screen
        m_video->append(rd, true);
    }

#   ifndef G3D_OSX
    // Draw "REC" on the screen. OS X glReadBuffer fails to set the
    // read buffer correctly to the back buffer, so we don't do this
    // there.
    rd->push2D();
    {
        static RealTime t0 = System::time();
        bool toggle = isEven((int)((System::time() - t0) * 2));
        m_font->draw2D(rd, "REC", Vector2(float(rd->width() - 100), 5.0f), 35, 
                       toggle ? Color3::black() : Color3::white(), Color3::black());
        m_font->draw2D(rd, m_hotKeyString + " to stop", Vector2(float(rd->width() - 100), 45.0f), 16, 
                       Color3::white(), Color4(0,0,0,0.45f));
    }
    rd->pop2D();
#   endif
}


static void saveMessage(const String& filename) {
    debugPrintf("Saved %s\n", filename.c_str());
    logPrintf("Saved %s\n", filename.c_str());
    consolePrintf("Saved %s\n", filename.c_str());
}


void VideoRecordDialog::stopRecording() {
    debugAssert(m_video);

    // Save the movie
    m_video->commit();
    String oldFilename = m_video->filename();
    String newFilename = oldFilename;
    m_video.reset();
    bool addToSVN = false;
    if (ScreenshotDialog::create(window(), theme(), FilePath::parent(newFilename))->getFilename(newFilename, addToSVN, "Save Movie")) {
        newFilename = trimWhitespace(newFilename);

        if (newFilename.empty()) {
            // Cancelled--delete the file
            FileSystem::removeFile(oldFilename);
        } else {
            if (oldFilename != newFilename) {
                FileSystem::rename(oldFilename, newFilename);
            }
            if (addToSVN) {
                G3D::svnAdd(FileSystem::resolve(newFilename));
            }
            saveMessage(newFilename);
        }
    }

    if (m_app) {
        // Restore the app state
        m_app->setFrameDuration(m_oldRealTimeTargetDuration, m_oldSimTimeStep);
    }

    // Reset the GUI
    m_recordButton->setCaption("Record Now (" + m_hotKeyString + ")");

    // Restore the window caption as well
    OSWindow* window = const_cast<OSWindow*>(OSWindow::current());
    const String& c = window->caption();
    const String& appendix = " - Recording " + m_hotKeyString + " to stop";
    if (endsWith(c, appendix)) {
        window->setCaption(c.substr(0, c.size() - appendix.size()));
    }
}


bool VideoRecordDialog::onEvent(const GEvent& event) {
    if (GuiWindow::onEvent(event)) {
        // Base class handled the event
        return true;
    }

    if (enabled()) {
        // Video
        const bool buttonClicked = (event.type == GEventType::GUI_ACTION) && (event.gui.control == m_recordButton);
        const bool hotKeyPressed = (event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == m_hotKey) && (event.key.keysym.mod == m_hotKeyMod);

        if (buttonClicked || hotKeyPressed) {
            if (m_video) {
                stopRecording();
            } else {
                startRecording();
            }
            return true;
        }

        const bool ssHotKeyPressed = (event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == m_ssHotKey) && (event.key.keysym.mod == m_ssHotKeyMod);

        if (ssHotKeyPressed) {
            takeScreenshot();
            return true;
        }

    }

    return false;
}


void VideoRecordDialog::takeScreenshot() {
    m_screenshotPending = true;
}


void VideoRecordDialog::maybeRecord(RenderDevice* rd) {
    if (m_video) {
        recordFrame(rd);
    }

    if (m_screenshotPending) {
        screenshot(rd);
        m_screenshotPending = false;
    }
}


void VideoRecordDialog::screenshot(RenderDevice* rd) {
    const shared_ptr<Texture>& texture = Texture::createEmpty("Screenshot", rd->width(), rd->height(), ImageFormat::RGB8());

    texture->copyFromScreen(rd->viewport());
    texture->visualization = Texture::Visualization::sRGB();

    String filename = ScreenshotDialog::nextFilenameBase(m_filenamePrefix) + "." + toLower(m_ssFormatList[m_ssFormatIndex]);
    const shared_ptr<Image>& image = texture->toImage();
    if (rd->invertY()) {
        image->flipVertical();
    }

    bool addToSVN = false;
    if (ScreenshotDialog::create(window(), theme(), FilePath::parent(filename))->getFilename(filename, addToSVN, "Save Screenshot", texture)) {
        filename = trimWhitespace(filename);
        if (! filename.empty()) {
            image->save(filename);
            saveMessage(filename);
            if (addToSVN) {
                G3D::svnAdd(FileSystem::resolve(filename));
            }
        }
    }
}


void VideoRecordDialog::Recorder::render(RenderDevice* rd) const {
    dialog->maybeRecord(rd);
}

}
