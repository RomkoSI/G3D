/**
  \file CameraControlWindow.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2007-06-01
  \edited  2013-07-25

 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */
#include "G3D/platform.h"
#include "G3D/Any.h"
#include "GLG3D/Camera.h"
#include "G3D/prompt.h"
#include "G3D/Rect2D.h"
#include "G3D/fileutils.h"
#include "G3D/FileSystem.h"
#include "G3D/Pointer.h"
#include "GLG3D/Camera.h"
#include "GLG3D/CameraControlWindow.h"
#include "GLG3D/FileDialog.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/GuiTabPane.h"
#include "GLG3D/GuiFrameBox.h"

namespace G3D {

const Vector2 CameraControlWindow::sDefaultWindowSize(286 + 16, 46);
const Vector2 CameraControlWindow::sExpandedWindowSize(286 + 16, 46 + 423);

static const String noSpline = "< None >";
static const String untitled = "< Unsaved >";


void CameraControlWindow::setFocusZ(float z) {
    m_camera->depthOfFieldSettings().setFocusPlaneZ(z);
}

float CameraControlWindow::focusZ() const {
    return m_camera->depthOfFieldSettings().focusPlaneZ();
}

void CameraControlWindow::setDepthOfFieldModel(int e) {
    m_camera->depthOfFieldSettings().setModel((DepthOfFieldModel)e);
}

int CameraControlWindow::depthOfFieldModel() const {
    return m_camera->depthOfFieldSettings().model();
}

void CameraControlWindow::setLensRadius(float r) {
    m_camera->depthOfFieldSettings().setLensRadius(r);
}

float CameraControlWindow::lensRadius() const {
    return m_camera->depthOfFieldSettings().lensRadius();
}

CFrame CameraControlWindow::cameraFrame() const {
    return m_camera->frame();
}

void CameraControlWindow::setCameraFrame(const CFrame& f) {
    if (notNull(m_camera)) {
        m_camera->setFrame(f);
    }

    if (notNull(m_manualManipulator)) {
        m_manualManipulator->setFrame(f);
    }
}


CameraControlWindow::CameraControlWindow
   (const shared_ptr<FirstPersonManipulator>&      manualManipulator, 
    const shared_ptr<UprightSplineManipulator>&    trackManipulator, 
    const Pointer<shared_ptr<Manipulator> >&     cameraManipulator,
    const shared_ptr<Camera>&             camera,
    const shared_ptr<Film>&               film,
    const shared_ptr<GuiTheme>&           skin) : 
    GuiWindow("Debug Camera", 
              skin, 
              Rect2D::xywh(5, 54, 200, 0),
              GuiTheme::TOOL_WINDOW_STYLE,
              GuiWindow::HIDE_ON_CLOSE),
    m_trackFileIndex(0),
    m_camera(camera),
    m_cameraManipulator(cameraManipulator),
    m_manualManipulator(manualManipulator),
    m_trackManipulator(trackManipulator),
    m_drawerButton(NULL),
    m_drawerButtonPane(NULL),
    m_expanded(false)
{
    const int tabCaptionSize = 11;

    m_manualOperation = m_manualManipulator->enabled();

    updateTrackFiles();

    GuiPane* pane = GuiWindow::pane();

    const char* CLIPBOARD = "\xA4";
    float h = 20;
    shared_ptr<GFont> iconFont = GFont::fromFile(System::findDataFile("icon.fnt"));
    shared_ptr<GFont> greekFont = GFont::fromFile(System::findDataFile("greek.fnt"));

    GuiFrameBox* cframeBox = pane->addFrameBox(Pointer<CFrame>(this, &CameraControlWindow::cameraFrame, &CameraControlWindow::setCameraFrame));
    cframeBox->moveBy(-2, -2);
    cframeBox->setWidth(270);

    GuiButton* copyButton = 
        pane->addButton(GuiText(CLIPBOARD, iconFont, 16),
                        GuiControl::Callback(this, &CameraControlWindow::copyToClipboard),
                        GuiTheme::TOOL_BUTTON_STYLE);
    copyButton->setSize(26, h);
    copyButton->moveBy(0, 2);


    GuiTabPane* tabPane = pane->addTabPane();
    tabPane->moveBy(-9, 5);

    /////////////////////////////////////////////////////////////////////////////////////////
    shared_ptr<GFont> defaultFont;
    GuiPane* filmPane = tabPane->addTab(GuiText("Film", defaultFont, (float)tabCaptionSize));
    {
        const float sliderWidth = 260,  indent = 2.0f;        
        if (film) {
            filmPane->moveBy(0, 5);
            camera->filmSettings().makeGui(filmPane, 10.0f, sliderWidth, indent);
        }
        filmPane->pack();
        filmPane->setWidth(286 + 10);
    }


#   define INDENT_SLIDER(n) n->setWidth(275); n->moveBy(15, 0); n->setCaptionWidth(100);

    GuiPane* focusPane = tabPane->addTab(GuiText("Depth of Field", defaultFont, (float)tabCaptionSize));
    {        
        focusPane->moveBy(0, 5);

        Pointer<int> dofPtr(this, &CameraControlWindow::depthOfFieldModel, &CameraControlWindow::setDepthOfFieldModel);
        focusPane->addCheckBox
            ("Enabled", 
             Pointer<bool>(&m_camera->depthOfFieldSettings(),
                           &DepthOfFieldSettings::enabled,
                           &DepthOfFieldSettings::setEnabled));

        focusPane->addRadioButton("None (Pinhole)", (int)DepthOfFieldModel::NONE, dofPtr, GuiTheme::NORMAL_RADIO_BUTTON_STYLE);
        focusPane->addRadioButton("Physical Lens", (int)DepthOfFieldModel::PHYSICAL, dofPtr, GuiTheme::NORMAL_RADIO_BUTTON_STYLE);
        {
            GuiControl* n = focusPane->addNumberBox
                ("Focus Dist.", 
                 NegativeAdapter<float>::create(Pointer<float>(this, &CameraControlWindow::focusZ, &CameraControlWindow::setFocusZ)),
                 "m", GuiTheme::LOG_SLIDER, 0.01f, 200.0f);
            INDENT_SLIDER(n);
            
            n = focusPane->addNumberBox
                ("Lens Radius", 
                 Pointer<float>(this, &CameraControlWindow::lensRadius, &CameraControlWindow::setLensRadius),
                 "m", GuiTheme::LOG_SLIDER, 0.0f, 0.5f);
            INDENT_SLIDER(n);
        }
        
        focusPane->addRadioButton("Artist Custom", 
                                  (int)DepthOfFieldModel::ARTIST, dofPtr, GuiTheme::NORMAL_RADIO_BUTTON_STYLE);

        {
            GuiControl* n;

            n = focusPane->addNumberBox
                ("Nearfield Blur", 
                 PercentageAdapter<float>::create(Pointer<float>(&m_camera->depthOfFieldSettings(),
                                                          &DepthOfFieldSettings::nearBlurRadiusFraction,
                                                          &DepthOfFieldSettings::setNearBlurRadiusFraction)),
                 "%", GuiTheme::LINEAR_SLIDER, 0.0f, 4.0f, 0.01f);
            INDENT_SLIDER(n);

            n = focusPane->addNumberBox
                ("Near Blur Dist.",  
                 NegativeAdapter<float>::create(Pointer<float>(&m_camera->depthOfFieldSettings(),
                                                               &DepthOfFieldSettings::nearBlurryPlaneZ, 
                                                               &DepthOfFieldSettings::setNearBlurryPlaneZ)),
                 "m", GuiTheme::LOG_SLIDER, 0.01f, 400.0f, 0.01f);
            INDENT_SLIDER(n);

            n = focusPane->addNumberBox("Near Sharp Dist.",
                 NegativeAdapter<float>::create(Pointer<float>(&m_camera->depthOfFieldSettings(),
                                                               &DepthOfFieldSettings::nearSharpPlaneZ, 
                                                               &DepthOfFieldSettings::setNearSharpPlaneZ)),
                                        "m", GuiTheme::LOG_SLIDER, 0.01f, 400.0f, 0.01f);
            INDENT_SLIDER(n);

            n = focusPane->addNumberBox("Far Sharp Dist.", 
                 NegativeAdapter<float>::create(Pointer<float>(&m_camera->depthOfFieldSettings(),
                                                               &DepthOfFieldSettings::farSharpPlaneZ, 
                                                               &DepthOfFieldSettings::setFarSharpPlaneZ)),
                                        "m", GuiTheme::LOG_SLIDER, 0.01f, 400.0f, 0.01f);
            INDENT_SLIDER(n);

            n = focusPane->addNumberBox("Far Blur Dist.",  
                 NegativeAdapter<float>::create(Pointer<float>(&m_camera->depthOfFieldSettings(),
                                                               &DepthOfFieldSettings::farBlurryPlaneZ, 
                                                               &DepthOfFieldSettings::setFarBlurryPlaneZ)),
                                        "m", GuiTheme::LOG_SLIDER, 0.01f, 400.0f, 0.01f);
            INDENT_SLIDER(n);

            n = focusPane->addNumberBox
                ("Farfield Blur",  
                 PercentageAdapter<float>::create(Pointer<float>(&m_camera->depthOfFieldSettings(),
                                                          &DepthOfFieldSettings::farBlurRadiusFraction,
                                                          &DepthOfFieldSettings::setFarBlurRadiusFraction)),
                 "%", GuiTheme::LINEAR_SLIDER, 0.0f, 4.0f, 0.01f);
            INDENT_SLIDER(n);

        }
        focusPane->pack();
    }

#   undef INDENT_SLIDER
#   define INDENT_SLIDER(n) n->setWidth(275);  n->setCaptionWidth(100);

    GuiPane* motionPane = tabPane->addTab(GuiText("Motion Blur", defaultFont, (float)tabCaptionSize));
    {
        motionPane->moveBy(0, 5);
        motionPane->addCheckBox
            ("Enabled", 
             Pointer<bool>(&m_camera->motionBlurSettings(),
                           &MotionBlurSettings::enabled,
                           &MotionBlurSettings::setEnabled));

        GuiControl* n;

        n = motionPane->addNumberBox
            ("Camera Motion", 
                PercentageAdapter<float>::create(Pointer<float>(&m_camera->motionBlurSettings(),
                                                        &MotionBlurSettings::cameraMotionInfluence,
                                                        &MotionBlurSettings::setCameraMotionInfluence)),
                "%", GuiTheme::LOG_SLIDER, 0.0f, 200.0f, 1.0f);
        INDENT_SLIDER(n);

        n = motionPane->addNumberBox
            ("Exposure", 
                PercentageAdapter<float>::create(Pointer<float>(&m_camera->motionBlurSettings(),
                                                        &MotionBlurSettings::exposureFraction,
                                                        &MotionBlurSettings::setExposureFraction)),
                "%", GuiTheme::LOG_SLIDER, 0.0f, 200.0f, 1.0f);
        INDENT_SLIDER(n);

        n = motionPane->addNumberBox
            ("Max Diameter", 
                PercentageAdapter<float>::create(Pointer<float>(&m_camera->motionBlurSettings(),
                                                        &MotionBlurSettings::maxBlurDiameterFraction,
                                                        &MotionBlurSettings::setMaxBlurDiameterFraction)),
                "%", GuiTheme::LOG_SLIDER, 0.0f, 20.0f, 0.01f);
        INDENT_SLIDER(n);

        n = motionPane->addNumberBox
            ("Samples/Pixel", 
               Pointer<int>(&m_camera->motionBlurSettings(),
                            &MotionBlurSettings::numSamples,
                            &MotionBlurSettings::setNumSamples),
                "", GuiTheme::LOG_SLIDER, 1, 63, 1);
        INDENT_SLIDER(n);
    }
    motionPane->pack();

#   undef INDENT_SLIDER
    /////////////////////////////////////////////////////////////////////////////////////////

    GuiPane* manualPane = tabPane->addTab(GuiText("Spline", defaultFont, (float)tabCaptionSize));
    manualPane->moveBy(-3, 2);

    manualPane->addCheckBox("Manual Control", &m_manualOperation)->moveBy(-4, 3);

    manualPane->beginRow();
    {
        m_trackList = manualPane->addDropDownList("Path", m_trackFileArray, &m_trackFileIndex);
        m_trackList->setRect(Rect2D::xywh(Vector2(2, m_trackList->rect().y1() - 25), Vector2(180, m_trackList->rect().height())));
        m_trackList->setCaptionWidth(34);

        m_visibleCheckBox = manualPane->addCheckBox("Visible", 
            Pointer<bool>(trackManipulator, 
                          &UprightSplineManipulator::showPath, 
                          &UprightSplineManipulator::setShowPath));

        m_visibleCheckBox->moveBy(6, 0);
    }
    manualPane->endRow();
    
    manualPane->beginRow();
    {
        Vector2 buttonSize = Vector2(20, 20);
        m_recordButton = manualPane->addRadioButton
            (GuiText::Symbol::record(), 
             UprightSplineManipulator::RECORD_KEY_MODE, 
             trackManipulator.get(),
             &UprightSplineManipulator::mode,
             &UprightSplineManipulator::setMode,
             GuiTheme::TOOL_RADIO_BUTTON_STYLE);
        m_recordButton->moveBy(32, 2);
        m_recordButton->setSize(buttonSize);
    
        m_playButton = manualPane->addRadioButton
            (GuiText::Symbol::play(), 
             UprightSplineManipulator::PLAY_MODE, 
             trackManipulator.get(),
             &UprightSplineManipulator::mode,
             &UprightSplineManipulator::setMode,
             GuiTheme::TOOL_RADIO_BUTTON_STYLE);
        m_playButton->setSize(buttonSize);

        m_stopButton = manualPane->addRadioButton
            (GuiText::Symbol::stop(), 
             UprightSplineManipulator::INACTIVE_MODE, 
             trackManipulator.get(),
             &UprightSplineManipulator::mode,
             &UprightSplineManipulator::setMode,
             GuiTheme::TOOL_RADIO_BUTTON_STYLE);
        m_stopButton->setSize(buttonSize);

        m_saveButton = manualPane->addButton("Save...");
        m_saveButton->setSize(m_saveButton->rect().wh() - Vector2(20, 1));
        m_saveButton->moveBy(20, -3);
        m_saveButton->setEnabled(false);
    }
    manualPane->endRow();


    manualPane->beginRow();
    {
        m_linearSplineButton = manualPane->addRadioButton
            ("Linear", 
             SplineExtrapolationMode::LINEAR, 
             trackManipulator.get(),
             &UprightSplineManipulator::extrapolationMode,
             &UprightSplineManipulator::setExtrapolationMode,
             GuiTheme::TOOL_RADIO_BUTTON_STYLE);
        

        m_clampedSplineButton = manualPane->addRadioButton
            ("Clamp", 
             SplineExtrapolationMode::CLAMP, 
             trackManipulator.get(),
             &UprightSplineManipulator::extrapolationMode,
             &UprightSplineManipulator::setExtrapolationMode,
             GuiTheme::TOOL_RADIO_BUTTON_STYLE);
        //m_clampedSplineButton->moveRightOf(m_linearSplineButton);

        m_cyclicSplineButton = manualPane->addRadioButton
            ("Cyclic", 
             SplineExtrapolationMode::CYCLIC, 
             trackManipulator.get(),
             &UprightSplineManipulator::extrapolationMode,
             &UprightSplineManipulator::setExtrapolationMode,
             GuiTheme::TOOL_RADIO_BUTTON_STYLE);
        //m_cyclicSplineButton->moveRightOf(m_clampedSplineButton);
    } manualPane->endRow();
    manualPane->pack();


    /////////////////////////////////////////////////////////
    GuiPane* projectionPane = tabPane->addTab(GuiText("Projection", defaultFont, (float)tabCaptionSize));
    projectionPane->moveBy(-3, 2);
    {
        // Near and far planes
        GuiNumberBox<float>* b = projectionPane->addNumberBox("Near Plane Z", 
            Pointer<float>(m_camera, &Camera::nearPlaneZ, &Camera::setNearPlaneZ),
            "m", GuiTheme::LOG_SLIDER, -20.0f, -0.001f);
        b->setWidth(290);  b->setCaptionWidth(105);

        GuiRadioButton* radioButton;
        m_farPlaneZSlider = b = projectionPane->addNumberBox("Far Plane Z", Pointer<float>(m_camera, &Camera::farPlaneZ, &Camera::setFarPlaneZ), "m", GuiTheme::LOG_SLIDER, -1000.0f, -0.10f, 0.0f, GuiTheme::NORMAL_TEXT_BOX_STYLE, true, false);
        b->setWidth(290);  b->setCaptionWidth(105);

        // Field of view
        b = projectionPane->addNumberBox("Field of View", Pointer<float>(m_camera, &Camera::fieldOfViewAngleDegrees, &Camera::setFieldOfViewAngleDegrees), GuiText("\xB0", greekFont, 15), GuiTheme::LINEAR_SLIDER, 10.0f, 120.0f, 0.5f);
        b->setWidth(290);  b->setCaptionWidth(105);

        projectionPane->beginRow();
        Pointer<FOVDirection> directionPtr(m_camera, &Camera::fieldOfViewDirection, &Camera::setFieldOfViewDirection);
        GuiRadioButton* radioButton2 = projectionPane->addRadioButton("Horizontal", FOVDirection::HORIZONTAL, directionPtr, GuiTheme::TOOL_RADIO_BUTTON_STYLE);
        radioButton2->moveBy(106, 0);
        radioButton2->setWidth(91);
        radioButton = projectionPane->addRadioButton("Vertical", FOVDirection::VERTICAL, directionPtr, GuiTheme::TOOL_RADIO_BUTTON_STYLE);
        radioButton->setWidth(radioButton2->rect().width());
        projectionPane->endRow();

        projectionPane->pack();
    }

    tabPane->pack();

    m_manualHelpCaption = GuiText("W,A,S,D,Z,C keys and right mouse (or ctrl+left mouse) to move; SHIFT = slow", shared_ptr<GFont>(), 10);

    m_autoHelpCaption = "";
    m_playHelpCaption = "";

    m_recordHelpCaption = GuiText("Spacebar to place a control point.", shared_ptr<GFont>(), 10);

    m_helpLabel = manualPane->addLabel(m_manualHelpCaption);
    m_helpLabel->setSize(280, 64);

    manualPane->pack();
    filmPane->setWidth(manualPane->rect().width() + 16);
    tabPane->setWidth(306);
    pack();

    // Set width to max expanded size so client size is correct when adding drawer items below
    setRect(Rect2D::xywh(rect().x0y0(), sExpandedWindowSize));

    // Make the pane width match the window width
    manualPane->setPosition(0, manualPane->rect().y0());
    manualPane->setSize(clientRect().width(), manualPane->rect().height());

    // Have to create the m_drawerButton last, otherwise the setRect
    // code for moving it to the bottom of the window will cause
    // layout to become broken.
    m_drawerCollapseCaption = GuiText("5", iconFont);
    m_drawerExpandCaption = GuiText("6", iconFont);
    m_drawerButtonPane = pane->addPane("", GuiTheme::NO_PANE_STYLE);
    m_drawerButton = m_drawerButtonPane->addButton(m_drawerExpandCaption, GuiTheme::TOOL_BUTTON_STYLE);
    m_drawerButton->setRect(Rect2D::xywh(0, 0, 12, 10));
    m_drawerButtonPane->setSize(12, 10);
    
    // Resize the pane to include the drawer button so that it is not clipped
    pane->setSize(clientRect().wh());

    // Collapse the window back down to default size
    setRect(Rect2D::xywh(rect().x0y0(), sDefaultWindowSize));
    sync();
}


shared_ptr<CameraControlWindow> CameraControlWindow::create
(const shared_ptr<FirstPersonManipulator>&      manualManipulator,
 const UprightSplineManipulatorRef&             trackManipulator,
 const Pointer<shared_ptr<Manipulator> >&       cameraManipulator,
 const shared_ptr<Camera>&                      camera,
 const shared_ptr<Film>&                        film,
 const shared_ptr<GuiTheme>&                    theme) {
    
    return shared_ptr<CameraControlWindow>(new CameraControlWindow(manualManipulator, trackManipulator, cameraManipulator, camera, film, theme));
}


void CameraControlWindow::setManager(WidgetManager* manager) {
    GuiWindow::setManager(manager);
    if (manager) {
        // Move to the upper right
        float osWindowWidth = (float)manager->window()->width();
        setRect(Rect2D::xywh(osWindowWidth - rect().width(), 40, rect().width(), rect().height()));
    }
}


void CameraControlWindow::copyToClipboard() {
    OSWindow::setClipboardText(cameraFrame().toXYZYPRDegreesString());
}


void CameraControlWindow::setRect(const Rect2D& r) {
    GuiWindow::setRect(r);
    if (m_drawerButtonPane) {
        const Rect2D& r = clientRect();
        m_drawerButtonPane->setPosition((r.width() - m_drawerButtonPane->rect().width()) / 2.0f, r.height() - m_drawerButtonPane->rect().height());
    }
}


void CameraControlWindow::updateTrackFiles() {
    m_trackFileArray.fastClear();
    m_trackFileArray.append(noSpline);
    FileSystem::getFiles("*.us.any", m_trackFileArray);

    // Element 0 is <unsaved>, so skip it
    for (int i = 1; i < m_trackFileArray.size(); ++i) {
        m_trackFileArray[i] = FilePath::base(FilePath::base(m_trackFileArray[i]));
    }
    m_trackFileIndex = iMin(m_trackFileArray.size() - 1, m_trackFileIndex);
}


void CameraControlWindow::onUserInput(UserInput* ui) {
    GuiWindow::onUserInput(ui);

    if (m_manualOperation && (m_trackManipulator->mode() == UprightSplineManipulator::PLAY_MODE)) {
        // Keep the FPS controller in sync with the spline controller
        CoordinateFrame cframe;
        m_trackManipulator->getFrame(cframe);
        m_manualManipulator->setFrame(cframe);
        m_trackManipulator->camera()->setFrame(cframe);
    }
}


bool CameraControlWindow::onEvent(const GEvent& event) {

    // Allow super class to process the event
    if (GuiWindow::onEvent(event)) {
        return true;
    }

    if (! visible()) {
        return false;
    }

    // Special buttons
    if (event.type == GEventType::GUI_ACTION) {
        GuiControl* control = event.gui.control;

        if (control == m_drawerButton) {

            // Change the window size
            m_expanded = ! m_expanded;
            morphTo(Rect2D::xywh(rect().x0y0(), m_expanded ? sExpandedWindowSize : sDefaultWindowSize));
            m_drawerButton->setCaption(m_expanded ? m_drawerCollapseCaption : m_drawerExpandCaption);

        } else if (control == m_trackList) {
            
            if (m_trackFileArray[m_trackFileIndex] != untitled) {
                // Load the new spline
                loadSpline(m_trackFileArray[m_trackFileIndex] + ".UprightSpline.any");

                // When we load, we lose our temporarily recorded spline,
                // so remove that display from the menu.
                if (m_trackFileArray.last() == untitled) {
                    m_trackFileArray.remove(m_trackFileArray.size() - 1);
                }
            }

        } else if (control == m_playButton) {

            // Take over manual operation
            m_manualOperation = true;
            // Restart at the beginning of the path
            m_trackManipulator->setTime(0);

        } else if (control == m_recordButton) {

            // Take over manual operation and reset the recording
            m_manualOperation = true;
            m_trackManipulator->clear();
            m_trackManipulator->setTime(0);

            // Select the untitled path
            if ((m_trackFileArray.size() == 0) || (m_trackFileArray.last() != untitled)) {
                m_trackFileArray.append(untitled);
            }
            m_trackFileIndex = m_trackFileArray.size() - 1;

            m_saveButton->setEnabled(true);

        } else if (control == m_saveButton) {

            // Save
            String saveName;

            if (FileDialog::getFilename(saveName)) {
                saveName = filenameBaseExt(trimWhitespace(saveName));

                if (saveName != "") {
                    saveName = saveName.substr(0, saveName.length() - filenameExt(saveName).length());
                    saveSpline(saveName);
                }
            }
        }
        sync();

    } else if (m_trackManipulator->mode() == UprightSplineManipulator::RECORD_KEY_MODE) {
        // Check if the user has added a point yet
        sync();
    } 
    return false;
}


void CameraControlWindow::saveSpline(const String& trackName) {
    Any any(m_trackManipulator->spline());
    any.save(trackName + ".UprightSpline.any");

    updateTrackFiles();

    // Select the one we just saved
    m_trackFileIndex = iMax(0, m_trackFileArray.findIndex(trackName));
                    
    m_saveButton->setEnabled(false);
}


void CameraControlWindow::loadSpline(const String& filename) {
    m_saveButton->setEnabled(false);
    m_trackManipulator->setMode(UprightSplineManipulator::INACTIVE_MODE);

    if (filename == noSpline) {
        m_trackManipulator->clear();
        return;
    }

    if (! FileSystem::exists(filename)) {
        m_trackManipulator->clear();
        return;
    }

    Any any;
    any.load(filename);

    UprightSpline spline(any);

    m_trackManipulator->setSpline(spline);
    m_manualOperation = true;
}


void CameraControlWindow::sync() {

    if (m_expanded) {
        bool hasTracks = m_trackFileArray.size() > 0;
        m_trackList->setEnabled(hasTracks);

        bool hasSpline = m_trackManipulator->splineSize() > 0;
        m_visibleCheckBox->setEnabled(hasSpline);

        m_linearSplineButton->setEnabled(hasSpline);
        m_clampedSplineButton->setEnabled(hasSpline);
        m_cyclicSplineButton->setEnabled(hasSpline);

        m_playButton->setEnabled(hasSpline);

        if (m_manualOperation) {
            switch (m_trackManipulator->mode()) {
            case UprightSplineManipulator::RECORD_KEY_MODE:
            case UprightSplineManipulator::RECORD_INTERVAL_MODE:
                m_helpLabel->setCaption(m_recordHelpCaption);
                break;

            case UprightSplineManipulator::PLAY_MODE:
                m_helpLabel->setCaption(m_playHelpCaption);
                break;

            case UprightSplineManipulator::INACTIVE_MODE:
                m_helpLabel->setCaption(m_manualHelpCaption);
            }
        } else {
            m_helpLabel->setCaption(m_autoHelpCaption);
        }
    }

    if (m_manualOperation) {
        // User has control
        bool playing = m_trackManipulator->mode() == UprightSplineManipulator::PLAY_MODE;
        m_manualManipulator->setEnabled(! playing);
        if (playing) {
            *m_cameraManipulator = m_trackManipulator;
        } else {
            *m_cameraManipulator = m_manualManipulator;
        }
    } else {
        // Program has control
        m_manualManipulator->setEnabled(false);
        *m_cameraManipulator = shared_ptr<Manipulator>();
        m_trackManipulator->setMode(UprightSplineManipulator::INACTIVE_MODE);
    }
}

}
