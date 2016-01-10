/**
 \file GLG3D/Xbox360Controller.h

 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2013-01-15
 \edited  2013-01-16

 G3D Innovation Engine
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/
#ifndef GLG3D_Xbox360Controller_h
#define GLG3D_Xbox360Controller_h

#include "G3D/platform.h"
#include "G3D/Vector2.h"
#include "GLG3D/Widget.h"
#include "GLG3D/GKey.h"

namespace G3D {

class OSWindow;

/**
Platform-independent tracking of input from an Xbox360 controller.

The Xbox360 controller has become the de facto standard PC controller and
merits special support in G3D.  Unforuntately, the controller's axes and buttons are mapped
differently on Windows and OS X by the underlying drivers.  This class 
provides a uniform interface.

A reliable open source OS X drivers for the Xbox360 controller and Wireless Gaming Receiver 
for Windows is available from http://tattiebogle.net/index.php/ProjectRoot/Xbox360Controller/OsxDriver

There is no hardware difference between the "Xbox360 controller for Windows" by Microsoft and the 
"Xbox360 controller" that ships for the console itself.  However, third party controllers
may not work with the Microsoft driver for Windows. A driver that the G3D team has used 
successfully with these controllers is available (with source) at:
http://vba-m.com/forum/Thread-xbcd-0-2-7-release-info-updates-will-be-posted-here (follow the installation instructions carefully).

On Windows, the left and right trigger buttons are mapped to the same axis due to
a strange underlying API choice by Microsoft in their own driver and DirectInput 8.  
The newer XInput API supports the axes correctly, and force feedback.  Since G3D 9.0, 
G3D uses the 
GLFW library for access to the joystick. G3D
will provide independent access to the triggers when the GLFW project adds support
for XInput.

\sa UserInput, UserInput::virtualStick1, OSWindow::getJoystickState
\beta
*/
class Xbox360Controller : public Widget {
public:

    enum StickIndex {
        LEFT,
        RIGHT,
        /** Both triggers, where the left is the x axis
            and the right is the y axis.  Note that this is 
            currently mapped incorrectly on Windows. */
        TRIGGER
    };

protected:

    class Button {
    public:
        bool                    currentValue;

        /** Changed since the previous onAfterEvents */
        bool                    changed;

        Button() : currentValue(false), changed(false) {}
    };

    class Stick {
    public:
        Vector2                 currentValue;
        Vector2                 previousValue;
    };

    bool                        m_present;
    unsigned int                m_joystickNumber;

    enum {NUM_STICKS = 3, NUM_BUTTONS = GKey::CONTROLLER_GUIDE - GKey::CONTROLLER_A + 1};

    Stick                       m_stickArray[NUM_STICKS];

    /** State of the buttons, where <code>index = k - GKey::CONTROLLER_A</code>*/
    Button                      m_buttonArray[NUM_BUTTONS];

    /** Performs range checking */
    const Stick& stick(StickIndex) const;

    /** Performs range checking */
    Button& button(GKey k);

    const Button& button(GKey k) const {
        return const_cast<Xbox360Controller*>(this)->button(k);
    }

    Xbox360Controller(unsigned int n) : m_present(false), m_joystickNumber(n) {}

public:

    /** True if this controller is connected and appears to actually be an Xbox360 controller. */
    bool present() const {
        return m_present;
    }

    static shared_ptr<Xbox360Controller> create(unsigned int joystickNumber) {
        return shared_ptr<Xbox360Controller>(new Xbox360Controller(joystickNumber));
    }

    virtual void setManager(WidgetManager* m) override;

    /** Latches the state of the controller. */
    virtual void onAfterEvents() override;
 
    /** Returns true if this controller button was pressed between the last two calls of onAfterEvents.  Supports
        GKey::CONTROLLER_A through GKey::CONTROLLER_GUIDE. */
    bool justPressed(GKey k) const {
        return button(k).currentValue && button(k).changed;
    }

    /** Returns true if this controller button was held down as of the last onAfterEvents call.  Supports
        GKey::CONTROLLER_A through GKey::CONTROLLER_GUIDE. */
    bool currentlyDown(GKey k) const {
        return button(k).currentValue;
    }

    /** Returns true if this controller button was released between the last two calls of onAfterEvents.  Supports
        GKey::CONTROLLER_A through GKey::CONTROLLER_GUIDE. */
    bool justReleased(GKey k) const {
        return ! button(k).currentValue && button(k).changed;
    }

    /** Position of an analog stick as of onAfterEvents. */
    Vector2 position(StickIndex s) const {
        return stick(s).currentValue;
    }

    /** Change in position of an analog stick between the previous two calls to onAfterEvents. */
    Vector2 delta(StickIndex s) const { 
        const Stick& st = stick(s);
        return st.currentValue - st.previousValue;
    }

    /** 
      Returns the counter-clockwise angle in radians that the stick has rotated through between the last
      two calls to onAfterEvents.  This is zero if the stick position had magnitude less than 0.20 during
      the frame.
      Useful for gesture-based input, such as the spray-painting swipes in Jet Grind Radio.
    */
    float angleDelta(StickIndex s) const;
};

} // namespace

#endif
