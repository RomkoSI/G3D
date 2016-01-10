/**
  \file G3D/UserInput.h
 
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2002-09-28
  \edited  2013-01-07

  Copyright 2001-2013, Morgan McGuire, http://graphics.cs.williams.edu
 All rights reserved.
 */

#ifndef G3D_UserInput_h
#define G3D_UserInput_h

#include "G3D/platform.h"
#include "G3D/Array.h"
#include "G3D/Table.h"
#include "G3D/Vector2.h"
#include "GLG3D/OSWindow.h"

namespace G3D {

    
/**
 \brief User input class that consolidates joystick, keyboard, and mouse state.

 Four axes are supported directly: joystick/keyboard x and y and 
 mouse x and y.  Mouse buttons, joystick buttons, and keyboard keys
 can all be used as 'keys' in the UserInput class.

 Call beginEvents() immediately before your SDL event handling routine and hand
 events to processEvent() as they become available.  Call endEvents() immediately
 after the loop.

 \sa G3D::GEvent, G3D::GApp::onEvent, G3D::OSWindow
*/
class UserInput {
public:

    enum UIFunction {UP, DOWN, LEFT, RIGHT, NONE};

private:

    OSWindow*                _window;

    /**
     keyState[x] is true if key[x] is depressed.
     */
    Array<bool>             keyState;

    /**
      All keys that were just pressed down since the last call to
      poll().
     */
    // Since relatively few keys are pressed every frame, keeping an array of
    // key codes pressed is much more compact than clearing a large array of bools.
    Array<GKey>             justPressed;
    Array<GKey>             justReleased;

    /**
     Function of key[x]
     */
    Array<UIFunction>       keyFunction;

    bool                    inEventProcessing;

    /** Called from the constructors */
    void init(OSWindow* window, Table<GKey, UIFunction>* keyMapping);

    /** Called from setPureDeltaMouse */
    void grabMouse();

    /** Called from setPureDeltaMouse */
    void releaseMouse();

    /** 
        This position is returned by mouseXY. 
        Also the last position of the mouse before
        setPureDeltaMouse = true.*/
    Vector2                 guiMouse;


    /** Center of the window.  Recomputed in endEvents. */
    Vector2                 windowCenter;

    /** True if appHasFocus was true on the previous call to endEvents.
        Updated during endEvents. */
    bool                    appHadFocus;

    bool                    _pureDeltaMouse;
    Vector2                 deltaMouse;

    /** Whether each direction key is up or down.*/
    bool                    left;
    bool                    right;
    bool                    up;
    bool                    down;
    
    uint8                   mouseButtons;

    /**
     Joystick x, y
     */
    double                  jx;
    double                  jy;

    /**
     In pixels
     */
    Vector2                 mouse;

    Vector2                 leftStick;
    Vector2                 rightStick;
    Vector2                 triggers;

    /**
     Expects SDL_MOUSEBUTTONDOWN, etc. to be translated into key codes.
     */
    void processKey(GKey code, int event);

public:

    bool                    useJoystick;
    
    /**
     */
    UserInput(OSWindow* window);

    /**
     Return the window used internally by the UserInput
     */
    OSWindow* window() const;

    void setKeyMapping(Table<GKey, UIFunction>* keyMapping = NULL);

    /**
     Closes the joystick if necessary.
     */
    virtual ~UserInput();

    /**
     Call from inside the event loop for every event inside
     processEvents() (done for you by App3D.processEvents())
     */
    void processEvent(const GEvent& event);

    /**
     Call after your OSWindow event polling loop.  If you are using
     G3D::GApplet, this is handled for you.
     */
    void endEvents();

    /**
     Call before your OSWindow event polling loop.If you are using
     G3D::GApplet, this is handled for you.
     */
    void beginEvents();

    /**
     Sets the mouse position.  That new position will
     be returned by getMouseXY until the next endEvents()
     statement.
     */
    void setMouseXY(float x, float y);

    inline void setMouseXY(const Vector2& v) {
        setMouseXY(v.x, v.y);
    }

    int getNumJoysticks() const;

    /**
     Returns a number between -1 and 1 indicating the horizontal
     input from the user.  Keyboard overrides joystick.
     @deprecated
     */
    float getX() const;

    /**
     Returns a number between -1 and 1 indicating the vertical
     input from the user.  Up is positive, down is negative. 
     Keyboard overrides joystick.
     @deprecated
     */
    float getY() const;

    /**
     @deprecated
     */
    Vector2 getXY() const;

    inline Vector2 mouseXY() const {
        return guiMouse;
    }

    /**
     Returns true iff the given key is currently held down.
     */
    bool keyDown(GKey code) const;

    /**
     Returns true if this key went down at least once since the last call to
     poll().
     */
    bool keyPressed(GKey code) const;

    /**
     Returns true if this key came up since the last call to
     poll().
     */
    bool keyReleased(GKey code) const;

    /**
     True if any key has been pressed since the last call to poll().
     */
    bool anyKeyPressed() const;

    /** An array of all keys pressed since the last poll() call. */
    void pressedKeys(Array<GKey>& code) const;
    void releasedKeys(Array<GKey>& code) const;

    Vector2 mouseDXY() const;
    float mouseDX() const;
    float mouseDY() const;

    /**
     When set to true, the mouse cursor is invisible and
     the mouse is restricted to the current window.  Regardless
     of how far the user moves the mouse in one direction,
     continous mouseDXY values are returned.  
     
     The mouseXY values are not particularly useful in this mode.

     This setting is commonly used by first-person games.

     When the app loses focus the mouse is automatically freed
     and recaptured when focus returns.  debugBreak also frees
     the mouse.
     */
    void setPureDeltaMouse(bool m);
    bool pureDeltaMouse() const;

    /** Arrow keys, WASD, or the left stick of the joystick mapped to
        [0, 1] along X and Y axes */
    Vector2 virtualStick1() const;

    /** Mouse dx, dy, Q and E, or the right stick of the joystick
        mapped to [0, 1] along the X and Y axes 

        There is no default keyboard rotate up/down mapping.
    */
    Vector2 virtualStick2() const;

    /** The 5th and 6th analog axes on a controller, for trigger buttons.  Left trigger is 'x'
        and right trigger is 'y'.  Note the default state for each is -1, pressing makes them 1.
    */
    Vector2 virtualStick3() const;
};

} // G3D

#endif //G3D_UserInput_h
