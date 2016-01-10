/**
  \file GLG3D/VideoRecordDialog.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2008-10-14
  \edited  2013-07-29

 G3D Innovation Engine
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */
#ifndef G3D_VideoRecordDialog_h
#define G3D_VideoRecordDialog_h

#include "G3D/platform.h"
#include "GLG3D/Widget.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiCheckBox.h"
#include "GLG3D/GuiDropDownList.h"
#include "GLG3D/GuiButton.h"
#include "GLG3D/GuiNumberBox.h"
#include "GLG3D/VideoOutput.h"

namespace G3D {

// forward declare heavily dependent classes
class RenderDevice;


/**
   @brief A dialog that allows the user to launch recording of the
   on-screen image to a movie.

   The playback rate is the frames-per-second value to be stored in
   the movie file.  The record rate <code>1 / G3D::GApp::simTimeStep</code>.

   Set enabled to false to prevent hot-key handling.
 */
class VideoRecordDialog : public GuiWindow {
public:
    friend class GApp;
    friend class Texture;
protected:
    GApp*                        m_app;

    /** For drawing messages on the screen */
    shared_ptr<GFont>            m_font;

    Array<VideoOutput::Settings> m_settingsTemplate;

    /** Parallel array to m_settingsTemplate of the descriptions for
        use with a drop-down list. */
    Array<String>                m_formatList;

    Array<String>                m_ssFormatList;

    /** Index into m_settingsTemplate and m_formatList */
    int                          m_templateIndex;

    /** Index into m_ssFormatList */
    int                          m_ssFormatIndex;

    float                        m_playbackFPS;
    float                        m_recordFPS;

    bool                         m_halfSize;
    bool                         m_enableMotionBlur;
    int                          m_motionBlurFrames;

    /** Recording modifies the GApp::simTimeStep; this is the old value */
    SimTime                      m_oldSimTimeStep;
    RealTime                     m_oldRealTimeTargetDuration;

    /** Tells the invisible window to record a screenshot when the next frame is rendered.*/
    bool                         m_screenshotPending;

    float                        m_quality;

    /** For downsampling */
    shared_ptr<Texture>          m_downsampleSrc;
    shared_ptr<Texture>          m_downsampleDst;
    shared_ptr<Framebuffer>      m_downsampleFBO;
    
    /** Motion blur frames */
    GuiNumberBox<int>*           m_framesBox;

    bool                         m_captureGUI;

    /** Draw a software cursor on the frame after capture, since the
     hardware cursor will not be visible.*/
    bool                         m_showCursor;

    GuiButton*                   m_recordButton;

    /** Key to start/stop recording even when the GUI is not
        visible.
      */
    GKey                         m_hotKey;
    GKeyMod                      m_hotKeyMod;

    /** Hotkey + mod as a human readable string */
    String                       m_hotKeyString;
    
    // Screenshot keys
    GKey                         m_ssHotKey;
    GKeyMod                      m_ssHotKeyMod;
    String                       m_ssHotKeyString;

    /** May include a directory */
    String                       m_filenamePrefix;

    /** 
        Inserts itself into the bottom of the Posed2D model drawing
        list to call recordFrame so that the rest of the GUI is not
        yet visible.
     */
    class Recorder : public Surface2D {
    public:
        VideoRecordDialog*       dialog;

        virtual Rect2D bounds () const {
            return Rect2D::xywh(0,0,1,1);
        }
            
        virtual float depth () const {
            // Lowest possible depth
            return (float)inf();
        }

        virtual void render (RenderDevice *rd) const;
    };

    shared_ptr<Recorder>            m_recorder;

    /** Non-NULL while recording */
    shared_ptr<VideoOutput>         m_video;

    VideoRecordDialog(const shared_ptr<GuiTheme>& theme, const String& prefix, GApp* app);

    /** Called from constructor */
    void makeGUI();

    /** Actually write a video frame */
    void recordFrame(RenderDevice* rd);

    /** Actually take a screen shot */
    void screenshot(RenderDevice* rd);
    
    /** Calls recordFrame() when video recording is in progress and 
       screenshot() when a shot is pending. */
    void maybeRecord(RenderDevice* rd);

public:

    /** Returns true if the format is supported.  e.g., PNG, JPG, BMP */
    bool setScreenShotFormat(const String& fmt) {
        int i = m_ssFormatList.findIndex(fmt);
        if (i > -1) {
            m_ssFormatIndex = i;
            return true;
        } else {
            return false;
        }
    }

    String screenShotFormat() {
        return m_ssFormatList[m_ssFormatIndex];
    }

    /**
       \param app If not NULL, the VideoRecordDialog will set the app's
       simTimeStep.

       \param prefix Prefix, which may include a path, of where to store screenshots.
     */
    static shared_ptr<VideoRecordDialog> create(const shared_ptr<GuiTheme>& theme, const String& prefix = "", GApp* app = NULL);
    static shared_ptr<VideoRecordDialog> create(const String& prefix, GApp* app);

    /** Automatically invoked when the record button or hotkey is pressed. 
        Can be called explicitly to force recording.*/
    void startRecording();
    void stopRecording();

	bool isRecording(){
		return m_video ? true : false;
	}

    /**
       When false, the screen is captured at the beginning of 
       Posed2DModel rendering from the back buffer, which may 
       slow down rendering.
       
       When true, the screen is captured from the the previous 
       frame, which will not introduce latency into rendering.
    */    
    bool captureGui() const {
        return m_captureGUI;
    }

    /** \copydoc captureGui() */
    void setCaptureGui(bool b) {
        m_captureGUI = b;
    }

    float quality() const {
        return m_quality;
    }

    /** Scales the default bit rate */
    void setQuality(float f) {
        m_quality = f;
    }

    /** Programmatically set the video recording to half size
        (defaults to true).  This can also be changed through the
        GUI. */
    void setHalfSize(bool b) {
        m_halfSize = b;
    }

    bool halfSize() const {
        return m_halfSize;
    }

    /** Automatically invoked when the hotkey is pressed. 
        Can be called explicitly to force a screenshot. 
        The actual screenshot will be captured on rendering 
        of the next frame.*/
    void takeScreenshot();

    virtual void onPose(Array<shared_ptr<Surface> >& posedArray, Array< Surface2DRef > &posed2DArray);

    virtual void onAI();

    virtual bool onEvent(const GEvent& event);
};

}

#endif
