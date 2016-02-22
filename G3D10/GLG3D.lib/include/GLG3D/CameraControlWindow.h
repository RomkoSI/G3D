/**
  \file CameraControlWindow.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2002-07-28
  \edited  2012-10-06

 G3D Library http://g3d.cs.williams.edu
 Copyright 2000-2015, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
 Use permitted under the BSD license
*/
#ifndef GLG3D_CameraControlWindow_h
#define GLG3D_CameraControlWindow_h

#include "G3D/platform.h"
#include "GLG3D/Widget.h"
#include "GLG3D/UprightSplineManipulator.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiLabel.h"
#include "GLG3D/GuiCheckBox.h"
#include "GLG3D/GuiDropDownList.h"
#include "GLG3D/GuiTextBox.h"
#include "GLG3D/GuiButton.h"
#include "GLG3D/FirstPersonManipulator.h"
#include "GLG3D/Film.h"
#include "GLG3D/GuiNumberBox.h"

namespace G3D {

class Camera;
class GuiFrameBox;

/**
 Gui used by DeveloperWindow default for recording camera position and making splines.
 \sa G3D::DeveloperWindow, G3D::GApp
 */
//
// If you are looking for an example of how to create a straightforward
// GUI in G3D do not look at this class!  CameraControlWindow uses a number
// of unusual tricks to provide a fancy compact interface that you do not
// need in a normal program.  The GUI code in this class is more complex
// than what you would have to write for a less dynamic UI.
class CameraControlWindow : public GuiWindow {
protected:

    static const Vector2 sDefaultWindowSize;
    static const Vector2 sExpandedWindowSize;

    /** Array of all .trk files in the current directory */
    Array<String>          m_trackFileArray;

    /** Index into trackFileArray */
    int                         m_trackFileIndex;

    GuiDropDownList*            m_trackList;

    enum {NO_BOOKMARK = -1};

    shared_ptr<GuiMenu>         m_menu;

    shared_ptr<Camera>          m_camera;

    GuiRadioButton*             m_playButton;
    GuiRadioButton*             m_stopButton;
    GuiRadioButton*             m_recordButton;

    GuiRadioButton*             m_linearSplineButton;
    GuiRadioButton*             m_clampedSplineButton;
    GuiRadioButton*             m_cyclicSplineButton;

    /** The manipulator from which the camera is copying its frame */
    Pointer<shared_ptr<Manipulator> >  m_cameraManipulator;

    shared_ptr<FirstPersonManipulator>  m_manualManipulator;
    shared_ptr<UprightSplineManipulator> m_trackManipulator;

    GuiCheckBox*                m_visibleCheckBox;
    

    /** Button to expand and contract additional manual controls. */
    GuiButton*                  m_drawerButton;

    /** The button must be in its own pane so that it can float over
        the expanded pane. */
    GuiPane*                    m_drawerButtonPane;
    GuiText                     m_drawerExpandCaption;
    GuiText                     m_drawerCollapseCaption;

    GuiButton*                  m_saveButton;

    GuiLabel*                   m_helpLabel;

    GuiText                     m_manualHelpCaption;
    GuiText                     m_autoHelpCaption;
    GuiText                     m_recordHelpCaption;
    GuiText                     m_playHelpCaption;

    /** If true, the window is big enough to show all controls */
    bool                        m_expanded;

    /** True when the user has chosen to override program control of
        the camera. */
    bool                        m_manualOperation;

    GuiNumberBox<float>*        m_farPlaneZSlider;

    CFrame cameraFrame() const;

    void setCameraFrame(const CFrame& f);

    /** Parses either prettyprinted or CFrame version */
    void setCameraLocation(const String& s);

    void setFocusZ(float z);
    float focusZ() const;

    void setLensRadius(float r);
    float lensRadius() const;

    void setDepthOfFieldModel(int e);
    int depthOfFieldModel() const;

    CameraControlWindow
    (const shared_ptr<FirstPersonManipulator>&    manualManipulator, 
     const UprightSplineManipulatorRef&  trackManipulator, 
     const Pointer<shared_ptr<Manipulator> >&    cameraManipulator,
     const shared_ptr<Camera>&                    camera,
     const shared_ptr<Film>&                    film,
     const shared_ptr<GuiTheme>&                  theme);

    /** Sets the controller for the cameraManipulator. */
    //void setSource(Source s);

    /** Control source that the Gui thinks should be in use */
    //Source desiredSource() const;

    void sync();

    void saveSpline(const String& filename);
    void loadSpline(const String& filename);

    /** Updates the trackFileArray from the list of track files */
    void updateTrackFiles();

    void copyToClipboard();

public:

    virtual void setManager(WidgetManager* manager);

    /** True if either the manual manipulator or the spline playback manipulator is currently
     driving the camera */
    bool manipulatorEnabled() const {
        return m_manualManipulator->enabled() ||
            (m_trackManipulator->mode() == UprightSplineManipulator::PLAY_MODE);
    }

    /**
     \param cameraManipulator The manipulator that should drive the camera.  This will be assigned to
     as the program runs.
     */
    static shared_ptr<CameraControlWindow> create
       (const shared_ptr<FirstPersonManipulator>&    manualManipulator,
        const UprightSplineManipulatorRef&  trackManipulator,
        const Pointer<shared_ptr<Manipulator> >&   cameraManipulator,
        const shared_ptr<Camera>&           camera,
        const shared_ptr<Film>&             film,
        const shared_ptr<GuiTheme>&         theme);

    virtual bool onEvent(const GEvent& event);
    virtual void onUserInput(UserInput*);
    virtual void setRect(const Rect2D& r);
};

}

#endif
