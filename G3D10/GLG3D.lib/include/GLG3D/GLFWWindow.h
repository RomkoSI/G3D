/**
  \file GLFWWindow.h
  
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
  \author Michael Mara, mikemx7f@gmail.com
  \created 2012-08-16
  \edited  2015-08-17

  G3D Innovation Engine
  Copyright 2000-2015, Morgan McGuire.
  All rights reserved
 */

#ifndef G3D_GLFWWindow_h
#define G3D_GLFWWindow_h

#include "GLG3D/OSWindow.h"
#include <memory>
#include "GLG3D/GEvent.h"

  struct GLFWwindow;
namespace G3D {



class GLFWWindow : public OSWindow {
  private:
    /** GLFWwindow is itself a pointer type, but we want to hold off on the #include until our cpp file */
    GLFWwindow* m_glfwWindow;

    /** The current key modifications (alts, ctrls, shifts) */
    GKeyMod::Value  m_currentKeyMod;

    bool m_inputCapture;

    /** The cursor is inside the window (and thus we should emit mouse motion events */
    bool m_cursorInside;

    bool m_mouseVisible;

    bool m_iconified;

    G3D::Set< intptr_t > m_usedIcons;

    Array<String> m_fileList;

    /** glfw doesn't necessarily report valid joysticks sequentially. 
        For example, if three joysticks are active , they could be {0, 2, 3}
        according to glfw. This maps those to sequential indices
        */
    Array<int> m_joystickMapping;

    GLFWWindow(const OSWindow::Settings& settings);

    /** Updates m_settings with data queried using glfw. TODO: finish implementation */
    void updateSettings();

    /** Registers out event callback functions with glfw (for key events, mouse events, etc.) */
    void setEventCallbacks();

    /** Checks the state of the modifier keys to set m_currentKeyMod properly */
    void initializeCurrentKeyMod();

    void updateJoystickMapping();

    static void setCurrentWindowPtr(GLFWWindow* w);

    /** Only implemented on Windows, returns (0,0) on other platforms */
    static Vector2int32 getDecoration(int width, int height, OSWindow::Settings s = OSWindow::Settings());

  public:
    void appendFileToFileList(String file) {
        m_fileList.append(file);
    }

    void clearDroppedFileList() {
        m_fileList.fastClear();
    }

    bool cursorActive(){
      return m_cursorInside;
    }

    void handleCursorEnter(int action);

    GKeyMod::Value getCurrentKeyMod(){
      return m_currentKeyMod;
    }

    void modifyCurrentKeyMod(GKeyMod::Value button, GButtonState state);

    void handleResizeFromCallback(int width, int height){
      //there is a minimum size that doesn't break G3D
      handleResize(max(8, width), max(8, height));
    }

    static GLFWWindow* getCurrentWindowPtr();

    virtual void getSettings(OSWindow::Settings& settings) const override;
    
    virtual int width() const override;
    virtual int height() const override;
    
    virtual Rect2D clientRect() const override;

    virtual void setClientRect(const Rect2D &dims) override;

    virtual Rect2D fullRect() const override {
        alwaysAssertM(false, "unimplemented");
        return Rect2D();
    }

    virtual void setFullRect(const Rect2D &dims) override {
        alwaysAssertM(false, "unimplemented");
    }


    virtual void getDroppedFilenames(Array<String>& files) override;
    
    virtual void setClientPosition(int x, int y) override;

    /** Only differs from setClientPosition properly on Windows */
    virtual void setFullPosition(int x, int y) override;
    
    virtual bool hasFocus() const override;
    
    String getAPIVersion() const override;

    String getAPIName() const override;

    virtual String className() const override { return "GLFWWindow"; }

    virtual void setGammaRamp(const Array<uint16>& gammaRamp) override;
    
    virtual void setCaption(const String& title) override;

    virtual String caption() override;
    
    virtual int numJoysticks() const override;
    
    virtual String joystickName(unsigned int stickNum) const override;
    
    virtual void setIcon(const shared_ptr<Image>& src) override;
    virtual void setIcon(const String& imageFilename) override;

    virtual void setRelativeMousePosition(double x, double y) override;
    virtual void setRelativeMousePosition(const Vector2& p) override;
    virtual void getRelativeMouseState(Vector2 &position, uint8 &mouseButtons) const override;

    virtual void getRelativeMouseState(int &x, int &y, uint8 &mouseButtons) const override;

    virtual void getRelativeMouseState(double &x, double &y, uint8 &mouseButtons) const override;

    virtual void getJoystickState(unsigned int stickNum, Array<float> &axis, Array<bool> &buttons) const override;
    
    virtual void setInputCapture(bool c) override;

    virtual bool inputCapture() const;
    
    virtual void setMouseVisible(bool b) override;

    virtual bool mouseVisible() const;
    
    virtual bool requiresMainLoop() const override{
        return false;
    }
    
    virtual void swapGLBuffers() override;

    /** \copydoc OSWindow::primaryDisplaySize */
    static Vector2 primaryDisplaySize();

    /** \copydoc OSWindow::virtualDisplaySize */
    static Vector2 virtualDisplaySize();

    /** \copydoc OSWindow::primaryDisplayWindowSize */
    static Vector2int32 primaryDisplayWindowSize();

    /** \copydoc OSWindow::numDisplays */
    static int numDisplays();

    void setSize(int width, int height);

    void getMousePosition(int& x, int& y) const;

    void getMouseButtonState(uint8& mouseButtons) const;

    int visibleCursorMode();

    virtual bool isIconified() const override;
    virtual void setIconified(bool b) override;

    static GLFWWindow* create(const OSWindow::Settings& settings = OSWindow::Settings());
    
    virtual void reallyMakeCurrent() const override;

    virtual ~GLFWWindow();

    virtual String _clipboardText() const override;

    virtual void _setClipboardText(const String& text) const override;


protected:
 
    virtual void getOSEvents(Queue<GEvent>& events) override;
    

}; // GLFWWindow

} // namespace G3D


#endif // G3D_GLFWWINDOW_H
