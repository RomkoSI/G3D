/**
  \file DeveloperWindow.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2007-06-01
  \edited  2013-04-08

 Copyright 2000-2017, Morgan McGuire morgan@casual-effects.com
 All rights reserved.
*/

#include "G3D/platform.h"
#include "GLG3D/DeveloperWindow.h"
#include "GLG3D/CameraControlWindow.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/GApp.h"
#include "GLG3D/IconSet.h"
#include "GLG3D/SceneEditorWindow.h"
#include "GLG3D/Scene.h"
#include "GLG3D/CameraControlWindow.h"
#include "GLG3D/VideoRecordDialog.h"
#include "GLG3D/UprightSplineManipulator.h"
#include "GLG3D/FirstPersonManipulator.h"
#include "GLG3D/GConsole.h"
#include "GLG3D/Camera.h"
#include "GLG3D/Film.h"
#include "GLG3D/GConsole.h"
#include "GLG3D/GuiTabPane.h"

#include "GLG3D/ProfilerWindow.h"

namespace G3D {

shared_ptr<DeveloperWindow> DeveloperWindow::create
    (GApp*                                      app,
     const shared_ptr<FirstPersonManipulator>&  manualManipulator,
     const shared_ptr<UprightSplineManipulator>&trackManipulator,
     const Pointer<shared_ptr<Manipulator> >&   cameraManipulator,
     const shared_ptr<Camera>&                  camera,
     const shared_ptr<Scene>&                   scene,
     const shared_ptr<Film>&                    film,
     const shared_ptr<GuiTheme>&                theme,
     const shared_ptr<GConsole>&                console,
     const Pointer<bool>&                       debugVisible,
     bool*                                      showStats,
     bool*                                      showText,
     const String&                              screenshotPrefix) {

    return createShared<DeveloperWindow>(app, manualManipulator, trackManipulator, cameraManipulator,
               camera, scene, film, theme, console, debugVisible, showStats, showText, screenshotPrefix);
}


DeveloperWindow::DeveloperWindow
    (GApp*                                      app,
     const shared_ptr<FirstPersonManipulator>&  manualManipulator,
     const shared_ptr<UprightSplineManipulator>&trackManipulator,
     const Pointer< shared_ptr<Manipulator> >&  cameraManipulator,
     const shared_ptr<Camera>&                  debugCamera,
     const shared_ptr<Scene>&                   scene,
     const shared_ptr<Film>&                    film,
     const shared_ptr<GuiTheme>&                theme,
     const shared_ptr<GConsole>&                console,
     const Pointer<bool>&                       debugVisible,
     bool*                                      showStats,
     bool*                                      showText,
     const String&                              screenshotPrefix) :
    GuiWindow("Developer (F11)", theme, Rect2D::xywh(0, 80, 0, 0), GuiTheme::TOOL_WINDOW_STYLE, HIDE_ON_CLOSE),
    m_textureBrowserButton(),
    m_textureBrowserWindow(),
    m_texturePopUpWindow(),
    consoleWindow(console) {

    alwaysAssertM(notNull(this), "Memory corruption");
    alwaysAssertM(notNull(debugCamera), "NULL camera");

    cameraControlWindow = CameraControlWindow::create(manualManipulator, trackManipulator, cameraManipulator,
                                                      debugCamera, film, theme);

    cameraControlWindow->moveTo(Point2(0, 140));

    videoRecordDialog = VideoRecordDialog::create(theme, screenshotPrefix, app);

    profilerWindow = ProfilerWindow::create(theme);
    app->addWidget(profilerWindow);
    profilerWindow->setVisible(false);

    if (notNull(scene)) {
        sceneEditorWindow = SceneEditorWindow::create(app, scene, theme);
        sceneEditorWindow->moveTo(cameraControlWindow->rect().x0y1() + Vector2(0, 15));
    }

    // For texture windows
    m_app      = app;
    m_theme    = theme;
    m_menu     = nullptr;

    GuiPane* root = pane();

    const float iconSize = 32;
    Vector2 buttonSize(iconSize, iconSize);

    const shared_ptr<IconSet>& iconSet = IconSet::fromFile(System::findDataFile("icon/tango.icn"));
    const GuiText cameraIcon(iconSet->get("22x22/devices/camera-photo.png"));
    const GuiText movieIcon(iconSet->get("22x22/categories/applications-multimedia.png"));
    const GuiText consoleIcon(iconSet->get("22x22/apps/utilities-terminal.png"));
    const GuiText statsIcon(iconSet->get("22x22/apps/utilities-system-monitor.png"));
    const GuiText debugIcon(iconSet->get("22x22/categories/preferences-desktop.png"));
    const GuiText sceneIcon(iconSet->get("22x22/categories/preferences-system.png"));
    const GuiText textIcon(iconSet->get("22x22/mimetypes/text-x-generic.png"));
    const GuiText textureBrowserIcon(iconSet->get("22x22/actions/window-new.png"));
    const GuiText profilerIcon(iconSet->get("22x22/actions/appointment-new.png"));

    const Pointer<bool>& ptr = Pointer<bool>((shared_ptr<GuiWindow>)cameraControlWindow, &GuiWindow::visible, &GuiWindow::setVisible);
    GuiControl* cameraButton = root->addCheckBox(cameraIcon, ptr, GuiTheme::TOOL_CHECK_BOX_STYLE);
    cameraButton->setSize(buttonSize);
    cameraButton->setPosition(0, 0);

    const Pointer<bool>& ptr2 = Pointer<bool>((shared_ptr<GuiWindow>)videoRecordDialog, &GuiWindow::visible, &GuiWindow::setVisible);
    GuiControl* movieButton = root->addCheckBox(movieIcon, ptr2, GuiTheme::TOOL_CHECK_BOX_STYLE);
    movieButton->setSize(buttonSize);

    static bool ignore = false;
    if (notNull(scene)) {
        Pointer<bool> ptr3 = Pointer<bool>((shared_ptr<GuiWindow>)sceneEditorWindow, &GuiWindow::visible, &GuiWindow::setVisible);
        GuiControl* sceneButton = root->addCheckBox(sceneIcon, ptr3, GuiTheme::TOOL_CHECK_BOX_STYLE);
        sceneButton->setSize(buttonSize);
    } else {
        GuiControl* sceneButton = root->addCheckBox(sceneIcon, &ignore, GuiTheme::TOOL_CHECK_BOX_STYLE);
        sceneButton->setSize(buttonSize);
        sceneButton->setEnabled(false);
    }

    Pointer<bool> profilePtr = Pointer<bool>((shared_ptr<GuiWindow>)profilerWindow, &GuiWindow::visible, &GuiWindow::setVisible);
    GuiControl* profilerButton = root->addCheckBox(profilerIcon, profilePtr, GuiTheme::TOOL_CHECK_BOX_STYLE);
    profilerButton->setSize(buttonSize);

    m_textureBrowserButton = root->addButton(textureBrowserIcon, this, &DeveloperWindow::texturePopUp, GuiTheme::TOOL_BUTTON_STYLE);
    m_textureBrowserButton->setSize(buttonSize);

    GuiControl* consoleButton = root->addCheckBox(consoleIcon, Pointer<bool>(consoleWindow, &GConsole::active, &GConsole::setActive), GuiTheme::TOOL_CHECK_BOX_STYLE);
    consoleButton->setSize(buttonSize);

    GuiControl* debugButton = root->addCheckBox(debugIcon, debugVisible, GuiTheme::TOOL_CHECK_BOX_STYLE);
    debugButton->setSize(buttonSize);

    GuiControl* statsButton = root->addCheckBox(statsIcon, showStats, GuiTheme::TOOL_CHECK_BOX_STYLE);
    statsButton->setSize(buttonSize);

    GuiControl* printButton = root->addCheckBox(textIcon, showText, GuiTheme::TOOL_CHECK_BOX_STYLE);
    printButton->setSize(buttonSize);

    cameraControlWindow->setVisible(true);
    videoRecordDialog->setVisible(false);
    pack();
}


void DeveloperWindow::setManager(WidgetManager* manager) {
    if (m_manager) {
        // Remove from old
        m_manager->remove(cameraControlWindow);
        m_manager->remove(videoRecordDialog);
        m_manager->remove(sceneEditorWindow);
    }

    if (manager) {
        // Add to new
        manager->add(cameraControlWindow);
        manager->add(videoRecordDialog);
        if (notNull(sceneEditorWindow)) {
            manager->add(sceneEditorWindow);
        }
    }

    GuiWindow::setManager(manager);

    // Move to the lower left
    Vector2 osWindowSize = manager->window()->clientRect().wh();
    setRect(Rect2D::xywh(Point2(0, osWindowSize.y - rect().height()), rect().wh()));
}


bool DeveloperWindow::onEvent(const GEvent& event) {
    if (! m_enabled) {
        return false;
    }

    if (GuiWindow::onEvent(event)) {
        return true;
    }

    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::F11)) {
        // Toggle visibility
        setVisible(! visible());
        return true;
    }

    return false;
}


void DeveloperWindow::makeNewTexturePane() {
    m_textureBrowserWindow->setTextureIndex(m_textureIndex);
    m_app->addWidget(m_textureBrowserWindow);
    m_textureBrowserWindow->setVisible(true);
    m_texturePopUpWindow->setVisible(false);
    m_texturePopUpWindow->setEnabled(false);
}


void DeveloperWindow::texturePopUp() {
    Array<String> textureNames;
    m_textureBrowserWindow = TextureBrowserWindow::create(m_theme, m_app);
    m_textureBrowserWindow->setVisible(false);
    m_textureBrowserWindow->getTextureList(textureNames);

    m_texturePopUpWindow = GuiWindow::create("", m_theme, Rect2D::xywh(rect().x0y0()  - Vector2(0, 250), Vector2(0, 250)), GuiTheme::NO_WINDOW_STYLE);

    // We purposefully recreate m_menu every time, because textureNames can change between calls
    m_menu = GuiMenu::create(m_theme, &textureNames, &m_textureIndex, true);
    m_menu->setManager(m_manager);

    m_menu->pack();
    m_menu->show(m_manager, this, m_textureBrowserButton, Vector2(window()->width() - m_menu->rect().width(), 0.0), false, GuiControl::Callback(this, &DeveloperWindow::makeNewTexturePane));
}

}
