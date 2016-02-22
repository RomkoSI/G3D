/**
  \file G3D/GEvent.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
  \created 2006-10-20
  \edited  2011-06-25
 
 G3D Library http://g3d.cs.williams.edu
 Copyright 2000-2015, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
 Use permitted under the BSD license
 */
#ifndef G3D_GEvent_h
#define G3D_GEvent_h

#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"
#include "G3D/platform.h"
#include "G3D/g3dmath.h"
#include "GLG3D/GKey.h"

namespace G3D {

/** General keyboard/mouse state definitions used by GEvent */
class GButtonState {
public:

    enum Value {
        RELEASED = 0,
        PRESSED = 1
    };

private:
    static const char* toString(int i, Value& v) {
        static const char* str[] = {"RELEASED", "PRESSED", NULL};
        static const Value val[] = {RELEASED, PRESSED};
        const char* s = str[i];
        if (s) {
            v = val[i];
        }
        return s;
    }

    Value value;

public:

    G3D_DECLARE_ENUM_CLASS_METHODS(GButtonState);

};
} // namespace G3D

G3D_DECLARE_ENUM_CLASS_HASHCODE(G3D::GButtonState);


#ifndef _SDL_joystick_h
/** The joystick structure used to identify an joystick */
struct _SDL_Joystick;
typedef struct _SDL_Joystick SDL_Joystick;
#endif

namespace G3D {

/** General keyboard/mouse state definitions. 
    @deprecated  Use GButtonState*/
enum { SDL_PRESSED = 0x01, SDL_RELEASED = 0x00 };
    
//////////////////////////////////////////////////////////

/*
 * Get the current state of a POV hat on a joystick
 * The return value is one of the following positions:
 */
#ifndef SDL_HAT_CENTERED
#define SDL_HAT_CENTERED    0x00
#define SDL_HAT_UP          0x01
#define SDL_HAT_RIGHT        0x02
#define SDL_HAT_DOWN        0x04
#define SDL_HAT_LEFT        0x08
#define SDL_HAT_RIGHTUP        (SDL_HAT_RIGHT|SDL_HAT_UP)
#define SDL_HAT_RIGHTDOWN    (SDL_HAT_RIGHT|SDL_HAT_DOWN)
#define SDL_HAT_LEFTUP        (SDL_HAT_LEFT|SDL_HAT_UP)
#define SDL_HAT_LEFTDOWN    (SDL_HAT_LEFT|SDL_HAT_DOWN)
#endif

/** Enumeration of valid key modifier codes (often used  ORed together) */
class GKeyMod {
public:
    enum Value {
        NONE  = 0x0000,
        LSHIFT= 0x0001,
        RSHIFT= 0x0002,
        LCTRL = 0x0040,
        RCTRL = 0x0080,
        LALT  = 0x0100,
        RALT  = 0x0200,
        LMETA = 0x0400,
        RMETA = 0x0800,
        NUM   = 0x1000,
        CAPS  = 0x2000,
        MODE  = 0x4000,
        RESERVED = 0x8000,  
        CTRL  = LCTRL  | RCTRL,
        SHIFT = LSHIFT | RSHIFT,
        ALT   = LALT   | RALT,
        META = LMETA  | RMETA
    };

private:
    
    Value value;

    static const char* toString(int i, Value& v) {
        static const char* str[] = {"NONE", "LSHIFT", "RSHIFT", "LCTRL", "RCTRL", "LALT", "RALT", "LMETA", "RMETA", "NUM", "CAPS", "MODE", "RESERVED", "CTRL", "SHIFT", "ALT", "META", NULL};
        static const Value val[] = {NONE, LSHIFT, RSHIFT, LCTRL, RCTRL, LALT, RALT, LMETA, RMETA, NUM, CAPS, MODE, RESERVED, CTRL, SHIFT, ALT, META};
        const char* s = str[i];
        if (s) {
            v = val[i];
        }
        return s;
    }

public:

    G3D_DECLARE_ENUM_CLASS_METHODS(GKeyMod);

};
} // namespace G3D

G3D_DECLARE_ENUM_CLASS_HASHCODE(G3D::GKeyMod);

namespace G3D {

/** @deprecated use GKeyMod */
enum {
    GKEYMOD_NONE  = 0x0000,
    GKEYMOD_LSHIFT= 0x0001,
    GKEYMOD_RSHIFT= 0x0002,
    GKEYMOD_LCTRL = 0x0040,
    GKEYMOD_RCTRL = 0x0080,
    GKEYMOD_LALT  = 0x0100,
    GKEYMOD_RALT  = 0x0200,
    GKEYMOD_LMETA = 0x0400,
    GKEYMOD_RMETA = 0x0800,
    GKEYMOD_NUM   = 0x1000,
    GKEYMOD_CAPS  = 0x2000,
    GKEYMOD_MODE  = 0x4000,
    GKEYMOD_RESERVED = 0x8000
};

#define GKEYMOD_CTRL    (GKEYMOD_LCTRL  | GKEYMOD_RCTRL)
#define GKEYMOD_SHIFT    (GKEYMOD_LSHIFT | GKEYMOD_RSHIFT)
#define GKEYMOD_ALT     (GKEYMOD_LALT   | GKEYMOD_RALT)
#define GKEYMOD_META    (GKEYMOD_LMETA  | GKEYMOD_RMETA)


/** Symbolic key structure */
class GKeySym {
public:
    /** Hardware specific scancode.
        Should not be used by general
        applications.  If no hardware scancode is available, it will be 0.
        */
    uint8           scancode;            

    /** G3D virtual raw key code */
    GKey::Value     sym;        

    /** Current key modifiers */
    GKeyMod::Value  mod;    

    /** Translated character. If non-zero,
     this is a UNICODE character corresponding to the keypress.  If the
     high 9 bits of the character are 0, then this maps to the equivalent
     ASCII character:
        <pre>
        char ch;
        if ( (keysym.unicode & 0xFF80) == 0 ) {
            ch = keysym.unicode & 0x7F;
        } else {
            An international character..
        }
        </pre>
     */
    uint16          unicode;

    void serialize(G3D::BinaryOutput& b) const {
        b.writeUInt8(scancode);
        b.writeInt32(sym);
        b.writeInt32(mod);
        b.writeUInt16(unicode);
    }

    void deserialize(G3D::BinaryInput& b) {
        scancode = b.readUInt8();
        sym = static_cast<GKey::Value>(b.readInt32());
        mod = static_cast<GKeyMod::Value>(b.readInt32());
        unicode = b.readUInt16();
    }
};

///////////////////////////////////////////////////////////

/** Event code enumerations */
class GEventType {
public:
    enum Value { 
       NONE = 0,            /** Unused (do not remove) */
       ACTIVE,                /** Application loses/gains visibility \deprecated.  Use FOCUS */
       KEY_DOWN,            /** Keys pressed */
       KEY_UP,                    /** Keys released */
       MOUSE_MOTION,        /** Mouse moved */
       MOUSE_BUTTON_DOWN,    /** Mouse button pressed */
       MOUSE_BUTTON_UP,        /** Mouse button released */
       JOY_AXIS_MOTION,        /** Joystick axis motion */
       JOY_BALL_MOTION,        /** Joystick trackball motion */
       JOY_HAT_MOTION,        /** Joystick hat position change */
       JOY_BUTTON_DOWN,        /** Joystick button pressed */
       JOY_BUTTON_UP,        /** Joystick button released */
       QUIT,                /** User-requested quit */
       SYSWMEVENT,            /** System specific event */
       EVENT_RESERVEDA,        /** Reserved for future use. */
       EVENT_RESERVEDB,        /** Reserved for future use. */
       VIDEO_RESIZE,        /** User resized video mode */
       VIDEO_EXPOSE,        /** Screen needs to be redrawn */
       EVENT_RESERVED2,        /** Reserved for future use. */
       EVENT_RESERVED3,        /** Reserved for future use. */
       EVENT_RESERVED4,        /** Reserved for future use. */
       EVENT_RESERVED5,        /** Reserved for future use. */
       EVENT_RESERVED6,        /** Reserved for future use. */
       CHAR_INPUT,          /** Character input (usually simultaneous with a KEY_DOWN) */
       GUI_DOWN,            /** GuiControl button, etc. pressed. */
       GUI_UP,              /** GuiControl button, etc. released. */
       GUI_ACTION,          /** Commit action: Button fire, enter pressed in a text box, slider released, menu selecion. */
       GUI_CHANGE,          /** Continuous changing (e.g., typing in text box, slider dragged.) */
       GUI_CANCEL,          /** Esc pressed in a text box or menu */
       GUI_CLOSE,           /** GuiWindow close button pressed. */
       GUI_KEY_FOCUS,       /** GuiControl gains or loses key focus. */
       FILE_DROP,           /** Signifies that files have been dropped onto the program. Call 
                               OSWindow.getDroppedFilenames to receive the actual data.*/
       MOUSE_SCROLL_2D,     /** A 2D scroll event has occured */
       MOUSE_BUTTON_CLICK,  /** A 2D button click (in addition to mouse released event).  Uses MouseButtonEvent. */
       KEY_REPEAT,          /** Operating system virtual key press from the key being held down. This is not fired on the physical key press.*/

       FOCUS,               /** Application gains or loses keyboard focus */

       /** This last event is only for bounding internal arrays
           It is the number of bits in the event mask datatype -- uint32 */
       NUMEVENTS
    };
private:
    static const char* toString(int i, Value& v) {
        static const char* str[] = {
           "NONE",            /* Unused (do not remove) */
           "ACTIVE",            /* Application loses/gains visibility \deprecated.  Use FOCUS */
           "KEY_DOWN",            /* Keys pressed */
           "KEY_UP",                /* Keys released */
           "MOUSE_MOTION",        /* Mouse moved */
           "MOUSE_BUTTON_DOWN",    /* Mouse button pressed */
           "MOUSE_BUTTON_UP",        /* Mouse button released */
           "JOY_AXIS_MOTION",        /* Joystick axis motion */
           "JOY_BALL_MOTION",        /* Joystick trackball motion */
           "JOY_HAT_MOTION",        /* Joystick hat position change */
           "JOY_BUTTON_DOWN",        /* Joystick button pressed */
           "JOY_BUTTON_UP",        /* Joystick button released */
           "QUIT",                /* User-requested quit */
           "SYSWMEVENT",            /* System specific event */
           "EVENT_RESERVEDA",        /* Reserved for future use.. */
           "EVENT_RESERVEDB",        /* Reserved for future use.. */
           "VIDEO_RESIZE",        /* User resized video mode */
           "VIDEO_EXPOSE",        /* Screen needs to be redrawn */
           "EVENT_RESERVED2",        /* Reserved for future use.. */
           "EVENT_RESERVED3",        /* Reserved for future use.. */
           "EVENT_RESERVED4",        /* Reserved for future use.. */
           "EVENT_RESERVED5",        /* Reserved for future use.. */
           "EVENT_RESERVED6",        /* Reserved for future use.. */
           "CHAR_INPUT",          /* Character input (usually simultaneous with a KEY_DOWN) */
           "GUI_DOWN",            /* GuiControl button, etc. pressed. */
           "GUI_UP",              /* GuiControl button, etc. released. */
           "GUI_ACTION",          /* Commit action: Button fire, enter pressed in a text box, slider released, menu selecion. */
           "GUI_CHANGE",          /* Continuous changing (e.g., typing in text box, slider dragged.) */
           "GUI_CANCEL",          /* Esc pressed in a text box or menu */
           "GUI_CLOSE",           /* GuiWindow close button pressed. */
           "GUI_KEY_FOCUS",       /** GuiControl gains or loses key focus. */
           "FILE_DROP",           /* Signifies that files have been dropped onto the program. Call 
                                   OSWindow.getDroppedFilenames to receive the actual data.*/
           "MOUSE_SCROLL_2D",     /* A 2D scroll event has occured */
           "MOUSE_BUTTON_CLICK",  /* A 2D button click (in addition to mouse released event).  Uses MouseButtonEvent. */
           "KEY_REPEAT",/** Operating system virtual key press from the key being held down. This is not fired on the physical key press.*/
           "FOCUS",

           "NUMEVENTS", NULL};
       static const Value val[] = {NONE,            /* Unused (do not remove) */
       ACTIVE,                /* Application loses/gains visibility \deprecated.  Use FOCUS */
       KEY_DOWN,            /* Keys pressed */
       KEY_UP,                /* Keys released */
       MOUSE_MOTION,        /* Mouse moved */
       MOUSE_BUTTON_DOWN,    /* Mouse button pressed */
       MOUSE_BUTTON_UP,        /* Mouse button released */
       JOY_AXIS_MOTION,        /* Joystick axis motion */
       JOY_BALL_MOTION,        /* Joystick trackball motion */
       JOY_HAT_MOTION,        /* Joystick hat position change */
       JOY_BUTTON_DOWN,        /* Joystick button pressed */
       JOY_BUTTON_UP,        /* Joystick button released */
       QUIT,                /* User-requested quit */
       SYSWMEVENT,            /* System specific event */
       EVENT_RESERVEDA,        /* Reserved for future use.. */
       EVENT_RESERVEDB,        /* Reserved for future use.. */
       VIDEO_RESIZE,        /* User resized video mode */
       VIDEO_EXPOSE,        /* Screen needs to be redrawn */
       EVENT_RESERVED2,        /* Reserved for future use.. */
       EVENT_RESERVED3,        /* Reserved for future use.. */
       EVENT_RESERVED4,        /* Reserved for future use.. */
       EVENT_RESERVED5,        /* Reserved for future use.. */
       EVENT_RESERVED6,        /* Reserved for future use.. */
       CHAR_INPUT,          /* Character input (usually simultaneous with a KEY_DOWN) */
       GUI_DOWN,            /* GuiControl button, etc. pressed. */
       GUI_UP,              /* GuiControl button, etc. released. */
       GUI_ACTION,          /* Commit action: Button fire, enter pressed in a text box, slider released, menu selecion. */
       GUI_CHANGE,          /* Continuous changing (e.g., typing in text box, slider dragged.) */
       GUI_CANCEL,          /* Esc pressed in a text box or menu */
       GUI_CLOSE,           /* GuiWindow close button pressed. */
       GUI_KEY_FOCUS,       /** GuiControl gains or loses key focus within its window.  This is distinct from a GuiWindow gaining or losing focus relative to other windows, and relative to the app as a whole losing focus. */
       FILE_DROP,           /* Signifies that files have been dropped onto the program. Call 
                               OSWindow.getDroppedFilenames to receive the actual data.*/
       MOUSE_SCROLL_2D,     /* A 2D scroll event has occured */
       MOUSE_BUTTON_CLICK,  /* A 2D button click (in addition to mouse released event).  Uses MouseButtonEvent. */
       KEY_REPEAT,
       FOCUS,

       NUMEVENTS};

        const char* s = str[i];
        if (s) {
            v = val[i];
        }
        return s;
    }
    Value value;

public:

    G3D_DECLARE_ENUM_CLASS_METHODS(GEventType);

    /** False for GUI_CLOSE, which is a GuiCloseEvent */
    bool isGuiEvent() const {
        return value >= GUI_DOWN && value < GUI_CLOSE;
    }
};

}

G3D_DECLARE_ENUM_CLASS_HASHCODE(G3D::GEventType);

namespace G3D {

#ifndef SDL_APPMOUSEFOCUS
/* The available application states */
#define SDL_APPMOUSEFOCUS    0x01        /** \def SDL_APPMOUSEFOCUS The app has mouse coverage */
#define SDL_APPINPUTFOCUS    0x02        /** \def SDL_APPINPUTFOCUS The app has input focus */
#define SDL_APPACTIVE        0x04        /** \def SDL_APPACTIVE  The application is active */
#endif

/** Application visibility event structure \deprecated */
class ActiveEvent {
public:
    /** GEventType::ACTIVE */
    uint8 type;
    
    /** Whether given states were gained or lost (1/0) */
    uint8 gain;

    /** A mask of the focus states */
    uint8 state;
};

class FocusEvent {
public:
    /** GEventType::FOCUS */
    uint8 type;

    /** Does the App now have focus? */
    bool  hasFocus;
};


/** Keyboard event structure */
class KeyboardEvent {
public:
    /** GEventType::KEY_DOWN, GEventType::KEY_UP, or GEVentType::KEY_REPEAT */
    uint8           type;

    /** The keyboard device index */
    uint8           which;

    /** GButtonState::PRESSED or GButtonState::RELEASED.  GEventType::KEY_REPEAT will generate multiple PRESSED values. */
    uint8           state;

    GKeySym         keysym;
};


/** Mouse motion event structure */
class MouseMotionEvent {
public:
    /** GEventType::MOUSEMOTION */
    uint8 type;    

    /** The mouse device index */
    uint8 which;

    /** The current button state */
    uint8 state;

    /** The X/Y coordinates of the mouse relative to the window. */
    int16 x, y;

    /** The relative motion in the X direction.  Not supported on all platforms. */
    int16 xrel;

    /** The relative motion in the Y direction.  Not supported on all platforms. */
    int16 yrel;
};

/* Mouse button event structure */
class MouseButtonEvent {
public:
    /** MOUSE_BUTTON_DOWN, MOUSE_BUTTON_UP, or MOUSE_CLICK */
    uint8 type;

    /** The mouse device index */
    uint8 which;    
    
    /** The mouse button index */
    uint8 button;

    /* For MOUSE_CLICK, this is numClicks.  For MOUSE_BUTTON_DOWN or MOUSE_BUTTON_UP, this is 
       GButtonState::PRESSED or GButtonState::RELEASED */
    union {
        uint8 numClicks;
        uint8 state;
    };

    /** The X/Y coordinates of the mouse at press time */
    int16 x, y;

    /** True if the ctrl key is pressed */
    bool controlKeyIsDown;
};


/** 2D scroll event structure **/
class MouseScroll2DEvent { 
public:
    /** GEventType::MOUSE_SCROLL_2D */
    uint8 type;
    
    /** The mouse device index */
    uint8 which;
    
    /** The change in x and y directions of scroll */
    int16 dx, dy; 
};


/** Joystick axis motion event structure */
class JoyAxisEvent {
public:
    /* JOYAXISMOTION */
    uint8 type;

    /** The joystick device index */
    uint8 which;    

    /** The joystick axis index */
    uint8 axis;

    /** The axis value (range: -32768 to 32767) */
    int16 value;
};

/** Joystick trackball motion event structure */
class JoyBallEvent {
public:
    /** JOYBALLMOTION */
    uint8 type;

    /** The joystick device index */
    uint8 which;

    /** The joystick trackball index */
    uint8 ball;

    /** The relative motion in the X direction */
    int16 xrel;

    /** The relative motion in the Y direction */
    int16 yrel;
};


/** Joystick hat position change event structure */
class JoyHatEvent {
public:
    /** GEventType::JOYHATMOTION */
    uint8 type;

    /** The joystick device index */
    uint8 which;

    /** The joystick hat index */
    uint8 hat;

    /** The hat position value:
        SDL_HAT_LEFTUP   SDL_HAT_UP       SDL_HAT_RIGHTUP
        SDL_HAT_LEFT     SDL_HAT_CENTERED SDL_HAT_RIGHT
        SDL_HAT_LEFTDOWN SDL_HAT_DOWN     SDL_HAT_RIGHTDOWN
        Note that zero means the POV is centered.
    */
    uint8 value;
};

/* Joystick button event structure */
class JoyButtonEvent {
public:
    /** JOY_BUTTON_DOWN or JOY_BUTTON_UP */
    uint8 type;

    /** The joystick device index */
    uint8 which;    

    /** The joystick button index */
    uint8 button;

    /** GButtonState::PRESSED or GButtonState::RELEASED */
    uint8 state;
};

/** The "window resized" event
   When you get this event, you are responsible for setting a new video
   mode with the new width and height.
 */
class ResizeEvent {
public:
    /** GEventType::VIDEORESIZE */
    uint8 type;    

    /** New width */
    int w;
    
    /** New height */
    int h;
};

/**
 Triggered when one or more files are dropped onto a window.  The actual filenames dropped can be obtained by calling
 G3D::OSWindow::getDroppedFilenames
*/
class FileDropEvent {
public:
    /** GEventType::FILE_DROP */
    uint8 type;
    /** Mouse position of drop */
    int   x, y;
};

/** The "screen redraw" event */
class ExposeEvent {
public:
    /** GEventType::VIDEOEXPOSE */
    uint8 type;
};


/** The "quit requested" event */
class QuitEvent {
public:
    /** GEventType::QUIT */
    uint8 type;    
};


/** A user-defined event type */
class UserEvent {
public:
    /** GEventType::USEREVENT through GEventType::NUMEVENTS-1 */
    uint8 type;

    /** User defined event code */
    int code;    

    /** User defined data pointer */
    void *data1;

    /** User defined data pointer */
    void *data2;
};

/** An event triggered by character input (as from the keyboard) */
class CharInputEvent {
public:
    /** GEventType::CHAR_INPUT */
    uint8   type;

    /** The character inputted, if the high nine bits are all low, 
        this is ASCII value */
    uint16  unicode;
};


/** Events triggered by the G3D GUI system (see G3D::GuiWindow for discussion) */
class GuiEvent {
public:
    /** GEventType::GUI_UP, GEventType::GUI_DOWN, GEventType::GUI_ACTION, GEventType::GUI_KEY_FOCUS. */
    uint8              type;

    /** The control that produced the event. */
    class GuiControl*  control;
};


/** Triggered by pressing the close button on a G3D::GuiWindow */
class GuiCloseEvent {
public:
    /** GUI_CLOSE */
    uint8              type;

    /** The GuiWindow that was closed. This may be an intentionally
        dangling pointer if the application was not holding its own
        shared_ptr to the window. */
    class GuiWindow*   window;
};

/** 
  General low-level event structure.
 
  <b>Event Dispatch Overview</b>:

  <ol>
    <li> The OSWindow polls the operating system for events like key strokes and mouse movement, and
  recieves events fired by Gui and other classes.  
    <li> GApp::onUserInput polls GEvents from the OSWindow.
    <li> GApp calls WidgetManager::onEvent for its widget manager.
    <li> WidgetManager invokes Widget::onEvent for every installed widget (e.g., GuiWindow, FirstPersonManipulator).
    <li> When a GuiWindow receives an event, it dispatches that event to the control that 
     has focus if that control is inside the window (for mouse clicks, it first changes focus to the control under the mouse).
  </ol>


  Most event processing code looks like:

  <pre>
  switch (event.type) {
  case GEventType::MOUSEBUTTONDOWN:
     ...
     break;
  ...
  }
  </pre>

  @sa G3D::GEventType, G3D::Widget::onEvent, GApp::onEvent, G3D::OSWindow::pollEvent.

  @cite Based on libsdl's SDL_Event, which is based on X11 and Win32 events
 */
union GEvent {
public:
    /** This is a G3D::GEventType, but is given uint8 type so that it
        does not call the constructor because GEvent is a union. */
    uint8                   type;
    
    /** \deprecated */
    ActiveEvent             active;
    KeyboardEvent           key;
    MouseMotionEvent        motion;
    MouseButtonEvent        button;
    JoyAxisEvent            jaxis;
    JoyBallEvent            jball;
    JoyHatEvent             jhat;
    JoyButtonEvent          jbutton;
    ResizeEvent             resize;
    ExposeEvent             expose;
    QuitEvent               quit;
    UserEvent               user;
    CharInputEvent          character;
    GuiEvent                gui;
    GuiCloseEvent           guiClose;
    FileDropEvent           drop;
    MouseScroll2DEvent      scroll2d;
    FocusEvent              focus;

    String toString() const;
    bool isMouseEvent() const;
    Point2 mousePosition() const;
};

}

#endif
