/**
  \file GLG3D/DeveloperWindow.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2007-06-10
  \edited  2012-12-27
*/
#ifndef G3D_DeveloperWindow_h
#define G3D_DeveloperWindow_h

#include "G3D/platform.h"
#include "G3D/Pointer.h"
#include "GLG3D/Widget.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/TextureBrowserWindow.h"

namespace G3D {

class SceneEditorWindow;
class Scene;
class VideoRecordDialog;
class GConsole;
class Camera;
class Film;
class FirstPersonManipulator;
class UprightSplineManipulator;
class Manipulator;
class GApp;
class CameraControlWindow;
class GuiTabPane;
class ProfilerWindow;


/**
 Developer HUD controls instantiated by GApp::createDeveloperHUD for debugging.

 \sa G3D::GApp, G3D::CameraControlWindow, G3D::GConsole
 */
class DeveloperWindow : public GuiWindow {
protected:

    DeveloperWindow
    (GApp*                              app,
     const shared_ptr<FirstPersonManipulator>&   manualManipulator,
     const shared_ptr<UprightSplineManipulator>& trackManipulator,
     const Pointer< shared_ptr<Manipulator> >&   cameraManipulator,
     const shared_ptr<Camera>&          debugCamera,
     const shared_ptr<Scene>&           scene,
     const shared_ptr<Film>&            film,
     const shared_ptr<GuiTheme>&        theme,
     const shared_ptr<GConsole>&        console,
     const Pointer<bool>&               debugVisible,
     bool*                              showStats,
     bool*                              showText,
     const String&                      screenshotPrefix);

    // For the Texture Browsers, which are not created on init
    GApp*                               m_app;
    shared_ptr<GuiTheme>                m_theme;
    GuiButton*                          m_textureBrowserButton;
    shared_ptr<TextureBrowserWindow>    m_textureBrowserWindow;
    shared_ptr<GuiWindow>               m_texturePopUpWindow; 
    shared_ptr<GuiMenu>                 m_menu;
    // For storing the input to a popup menu for a TextureBrowser
    int                                 m_textureIndex;

    void makeNewTexturePane();

public:

    shared_ptr<VideoRecordDialog>       videoRecordDialog;
    shared_ptr<CameraControlWindow>     cameraControlWindow;
    shared_ptr<GConsole>                consoleWindow;
    shared_ptr<SceneEditorWindow>       sceneEditorWindow;
    shared_ptr<ProfilerWindow>          profilerWindow;

    /** 
      \sa GApp::createDeveloperHUD
      \param scene May be NULL
     */
    static shared_ptr<DeveloperWindow> create
    (GApp*                              app,
     const shared_ptr<FirstPersonManipulator>&   manualManipulator,
     const shared_ptr<UprightSplineManipulator>& trackManipulator,
     const Pointer< shared_ptr<Manipulator> >&   cameraManipulator,
     const shared_ptr<Camera>&          debugCamera,
     const shared_ptr<Scene>&           scene,
     const shared_ptr<Film>&            film,
     const shared_ptr<GuiTheme>&        theme,
     const shared_ptr<GConsole>&        console,
     const Pointer<bool>&               debugVisible,
     bool*                              showStats,
     bool*                              showText,
     const String&                 screenshotPrefix);
    
    virtual void setManager(WidgetManager* manager);

    virtual bool onEvent(const GEvent& event);

    void texturePopUp();
};

}

#endif
