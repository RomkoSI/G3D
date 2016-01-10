#if 0
#include "G3D/platform.h"

#if (defined(G3D_LINUX) || defined(G3D_FREEBSD)) && ! defined(SDL_TABLESIZE)

// For SDL_Event
#include <SDL/SDL_events.h>

#if (defined(main) && ! defined(G3D_WINDOWS))
#   undef main
#endif

#ifdef G3D_WINDOWS

//////////////////////////////////////////////////////////
// SDL_types.h

/* The number of elements in a table */
#define SDL_TABLESIZE(table)    (sizeof(table)/sizeof(table[0]))

/* Basic data types */
typedef enum {
    SDL_FALSE = 0,
    SDL_TRUE  = 1
} SDL_bool;

typedef unsigned char    Uint8;
typedef signed char      Sint8;
typedef unsigned short   Uint16;
typedef signed short     Sint16;
typedef unsigned int     Uint32;
typedef signed int       Sint32;

#define SDL_HAS_64BIT_TYPE    __int64

typedef enum {
    DUMMY_ENUM_VALUE
} SDL_DUMMY_ENUM;

/* General keyboard/mouse state definitions */
enum { GButtonState::PRESSED = 0x01, GButtonState::RELEASED = 0x00 };

//////////////////////////////////////////////////////////

// SDL_joystick.h

/*
 * Get the current state of a POV hat on a joystick
 * The return value is one of the following positions:
 */
#define SDL_HAT_CENTERED    0x00
#define SDL_HAT_UP        0x01
#define SDL_HAT_RIGHT        0x02
#define SDL_HAT_DOWN        0x04
#define SDL_HAT_LEFT        0x08
#define SDL_HAT_RIGHTUP        (SDL_HAT_RIGHT|SDL_HAT_UP)
#define SDL_HAT_RIGHTDOWN    (SDL_HAT_RIGHT|SDL_HAT_DOWN)
#define SDL_HAT_LEFTUP        (SDL_HAT_LEFT|SDL_HAT_UP)
#define SDL_HAT_LEFTDOWN    (SDL_HAT_LEFT|SDL_HAT_DOWN)

/* The joystick structure used to identify an SDL joystick */
struct _SDL_Joystick;
typedef struct _SDL_Joystick SDL_Joystick;
///////////////////////////////////////////////////////////
// SDL_keysym.h

/* What we really want is a mapping of every raw key on the keyboard.
   To support international keyboards, we use the range 0xA1 - 0xFF
   as international virtual keycodes.  We'll follow in the footsteps of X11...
   The names of the keys
 */
 
typedef enum {
    /* The keyboard syms have been cleverly chosen to map to ASCII */
    GKEY_UNKNOWN        = 0,
    GKEY_FIRST        = 0,
    GKEY_BACKSPACE        = 8,
    GKEY_TAB        = 9,
    GKEY_CLEAR        = 12,
    GKEY_RETURN        = 13,
    GKEY_PAUSE        = 19,
    GKEY_ESCAPE        = 27,
    GKEY_SPACE        = 32,
    GKEY_EXCLAIM        = 33,
    GKEY_QUOTEDBL        = 34,
    GKEY_HASH        = 35,
    GKEY_DOLLAR        = 36,
    GKEY_AMPERSAND        = 38,
    GKEY_QUOTE        = 39,
    GKEY_LEFTPAREN        = 40,
    GKEY_RIGHTPAREN        = 41,
    GKEY_ASTERISK        = 42,
    GKEY_PLUS        = 43,
    GKEY_COMMA        = 44,
    GKEY_MINUS        = 45,
    GKEY_PERIOD        = 46,
    GKEY_SLASH        = 47,
    GKEY_0            = 48,
    GKEY_1            = 49,
    GKEY_2            = 50,
    GKEY_3            = 51,
    GKEY_4            = 52,
    GKEY_5            = 53,
    GKEY_6            = 54,
    GKEY_7            = 55,
    GKEY_8            = 56,
    GKEY_9            = 57,
    GKEY_COLON        = 58,
    GKEY_SEMICOLON        = 59,
    GKEY_LESS        = 60,
    GKEY_EQUALS        = 61,
    GKEY_GREATER        = 62,
    GKEY_QUESTION        = 63,
    GKEY_AT            = 64,
    /* 
       Skip uppercase letters
     */
    GKEY_LEFTBRACKET    = 91,
    GKEY_BACKSLASH        = 92,
    GKEY_RIGHTBRACKET    = 93,
    GKEY_CARET        = 94,
    GKEY_UNDERSCORE        = 95,
    GKEY_BACKQUOTE        = 96,
    GKEY_a            = 97,
    GKEY_b            = 98,
    GKEY_c            = 99,
    GKEY_d            = 100,
    GKEY_e            = 101,
    GKEY_f            = 102,
    GKEY_g            = 103,
    GKEY_h            = 104,
    GKEY_i            = 105,
    GKEY_j            = 106,
    GKEY_k            = 107,
    GKEY_l            = 108,
    GKEY_m            = 109,
    GKEY_n            = 110,
    GKEY_o            = 111,
    GKEY_p            = 112,
    GKEY_q            = 113,
    GKEY_r            = 114,
    GKEY_s            = 115,
    GKEY_t            = 116,
    GKEY_u            = 117,
    GKEY_v            = 118,
    GKEY_w            = 119,
    GKEY_x            = 120,
    GKEY_y            = 121,
    GKEY_z            = 122,
    GKEY_DELETE        = 127,
    /* End of ASCII mapped keysyms */

    /* International keyboard syms */
    GKEY_WORLD_0        = 160,        /* 0xA0 */
    GKEY_WORLD_1        = 161,
    GKEY_WORLD_2        = 162,
    GKEY_WORLD_3        = 163,
    GKEY_WORLD_4        = 164,
    GKEY_WORLD_5        = 165,
    GKEY_WORLD_6        = 166,
    GKEY_WORLD_7        = 167,
    GKEY_WORLD_8        = 168,
    GKEY_WORLD_9        = 169,
    GKEY_WORLD_10        = 170,
    GKEY_WORLD_11        = 171,
    GKEY_WORLD_12        = 172,
    GKEY_WORLD_13        = 173,
    GKEY_WORLD_14        = 174,
    GKEY_WORLD_15        = 175,
    GKEY_WORLD_16        = 176,
    GKEY_WORLD_17        = 177,
    GKEY_WORLD_18        = 178,
    GKEY_WORLD_19        = 179,
    GKEY_WORLD_20        = 180,
    GKEY_WORLD_21        = 181,
    GKEY_WORLD_22        = 182,
    GKEY_WORLD_23        = 183,
    GKEY_WORLD_24        = 184,
    GKEY_WORLD_25        = 185,
    GKEY_WORLD_26        = 186,
    GKEY_WORLD_27        = 187,
    GKEY_WORLD_28        = 188,
    GKEY_WORLD_29        = 189,
    GKEY_WORLD_30        = 190,
    GKEY_WORLD_31        = 191,
    GKEY_WORLD_32        = 192,
    GKEY_WORLD_33        = 193,
    GKEY_WORLD_34        = 194,
    GKEY_WORLD_35        = 195,
    GKEY_WORLD_36        = 196,
    GKEY_WORLD_37        = 197,
    GKEY_WORLD_38        = 198,
    GKEY_WORLD_39        = 199,
    GKEY_WORLD_40        = 200,
    GKEY_WORLD_41        = 201,
    GKEY_WORLD_42        = 202,
    GKEY_WORLD_43        = 203,
    GKEY_WORLD_44        = 204,
    GKEY_WORLD_45        = 205,
    GKEY_WORLD_46        = 206,
    GKEY_WORLD_47        = 207,
    GKEY_WORLD_48        = 208,
    GKEY_WORLD_49        = 209,
    GKEY_WORLD_50        = 210,
    GKEY_WORLD_51        = 211,
    GKEY_WORLD_52        = 212,
    GKEY_WORLD_53        = 213,
    GKEY_WORLD_54        = 214,
    GKEY_WORLD_55        = 215,
    GKEY_WORLD_56        = 216,
    GKEY_WORLD_57        = 217,
    GKEY_WORLD_58        = 218,
    GKEY_WORLD_59        = 219,
    GKEY_WORLD_60        = 220,
    GKEY_WORLD_61        = 221,
    GKEY_WORLD_62        = 222,
    GKEY_WORLD_63        = 223,
    GKEY_WORLD_64        = 224,
    GKEY_WORLD_65        = 225,
    GKEY_WORLD_66        = 226,
    GKEY_WORLD_67        = 227,
    GKEY_WORLD_68        = 228,
    GKEY_WORLD_69        = 229,
    GKEY_WORLD_70        = 230,
    GKEY_WORLD_71        = 231,
    GKEY_WORLD_72        = 232,
    GKEY_WORLD_73        = 233,
    GKEY_WORLD_74        = 234,
    GKEY_WORLD_75        = 235,
    GKEY_WORLD_76        = 236,
    GKEY_WORLD_77        = 237,
    GKEY_WORLD_78        = 238,
    GKEY_WORLD_79        = 239,
    GKEY_WORLD_80        = 240,
    GKEY_WORLD_81        = 241,
    GKEY_WORLD_82        = 242,
    GKEY_WORLD_83        = 243,
    GKEY_WORLD_84        = 244,
    GKEY_WORLD_85        = 245,
    GKEY_WORLD_86        = 246,
    GKEY_WORLD_87        = 247,
    GKEY_WORLD_88        = 248,
    GKEY_WORLD_89        = 249,
    GKEY_WORLD_90        = 250,
    GKEY_WORLD_91        = 251,
    GKEY_WORLD_92        = 252,
    GKEY_WORLD_93        = 253,
    GKEY_WORLD_94        = 254,
    GKEY_WORLD_95        = 255,        /* 0xFF */

    /* Numeric keypad */
    GKEY_KP0        = 256,
    GKEY_KP1        = 257,
    GKEY_KP2        = 258,
    GKEY_KP3        = 259,
    GKEY_KP4        = 260,
    GKEY_KP5        = 261,
    GKEY_KP6        = 262,
    GKEY_KP7        = 263,
    GKEY_KP8        = 264,
    GKEY_KP9        = 265,
    GKEY_KP_PERIOD        = 266,
    GKEY_KP_DIVIDE        = 267,
    GKEY_KP_MULTIPLY    = 268,
    GKEY_KP_MINUS        = 269,
    GKEY_KP_PLUS        = 270,
    GKEY_KP_ENTER        = 271,
    GKEY_KP_EQUALS        = 272,

    /* Arrows + Home/End pad */
    GKEY_UP            = 273,
    GKEY_DOWN        = 274,
    GKEY_RIGHT        = 275,
    GKEY_LEFT        = 276,
    GKEY_INSERT        = 277,
    GKEY_HOME        = 278,
    GKEY_END        = 279,
    GKEY_PAGEUP        = 280,
    GKEY_PAGEDOWN        = 281,

    /* Function keys */
    GKEY_F1            = 282,
    GKEY_F2            = 283,
    GKEY_F3            = 284,
    GKEY_F4            = 285,
    GKEY_F5            = 286,
    GKEY_F6            = 287,
    GKEY_F7            = 288,
    GKEY_F8            = 289,
    GKEY_F9            = 290,
    GKEY_F10        = 291,
    GKEY_F11        = 292,
    GKEY_F12        = 293,
    GKEY_F13        = 294,
    GKEY_F14        = 295,
    GKEY_F15        = 296,

    /* Key state modifier keys */
    GKEY_NUMLOCK        = 300,
    GKEY_CAPSLOCK        = 301,
    GKEY_SCROLLOCK        = 302,
    GKEY_RSHIFT        = 303,
    GKEY_LSHIFT        = 304,
    GKEY_RCTRL        = 305,
    GKEY_LCTRL        = 306,
    GKEY_RALT        = 307,
    GKEY_LALT        = 308,
    GKEY_RMETA        = 309,
    GKEY_LMETA        = 310,
    GKEY_LSUPER        = 311,        /* Left "Windows" key */
    GKEY_RSUPER        = 312,        /* Right "Windows" key */
    GKEY_MODE        = 313,        /* "Alt Gr" key */
    GKEY_COMPOSE        = 314,        /* Multi-key compose key */

    /* Miscellaneous function keys */
    GKEY_HELP        = 315,
    GKEY_PRINT        = 316,
    GKEY_SYSREQ        = 317,
    GKEY_BREAK        = 318,
    GKEY_MENU        = 319,
    GKEY_POWER        = 320,        /* Power Macintosh power key */
    GKEY_EURO        = 321,        /* Some european keyboards */
    GKEY_UNDO        = 322,        /* Atari keyboard has Undo */

    /* Add any other keys here */

    GKEY_LAST
} SDLKey;

/* Enumeration of valid key mods (possibly OR'd together) */
typedef enum {
    KMOD_NONE  = 0x0000,
    KMOD_LSHIFT= 0x0001,
    KMOD_RSHIFT= 0x0002,
    KMOD_LCTRL = 0x0040,
    KMOD_RCTRL = 0x0080,
    KMOD_LALT  = 0x0100,
    KMOD_RALT  = 0x0200,
    KMOD_LMETA = 0x0400,
    KMOD_RMETA = 0x0800,
    KMOD_NUM   = 0x1000,
    KMOD_CAPS  = 0x2000,
    KMOD_MODE  = 0x4000,
    KMOD_RESERVED = 0x8000
} SDLMod;

#define KMOD_CTRL    (KMOD_LCTRL|KMOD_RCTRL)
#define KMOD_SHIFT    (KMOD_LSHIFT|KMOD_RSHIFT)
#define KMOD_ALT    (KMOD_LALT|KMOD_RALT)
#define KMOD_META    (KMOD_LMETA|KMOD_RMETA)

///////////////////////////////////////////////////////////
// SDL_keyboard.h

/* Keysym structure
   - The scancode is hardware dependent, and should not be used by general
     applications.  If no hardware scancode is available, it will be 0.

   - The 'unicode' translated character is only available when character
     translation is enabled by the SDL_EnableUNICODE() API.  If non-zero,
     this is a UNICODE character corresponding to the keypress.  If the
     high 9 bits of the character are 0, then this maps to the equivalent
     ASCII character:
    char ch;
    if ( (keysym.unicode & 0xFF80) == 0 ) {
        ch = keysym.unicode & 0x7F;
    } else {
        An international character..
    }
 */
typedef struct SDL_keysym {
    Uint8 scancode;            /* hardware specific scancode */
    SDLKey sym;            /* SDL virtual keysym */
    SDLMod mod;            /* current key modifiers */
    Uint16 unicode;            /* translated character */
} SDL_keysym;

/* This is the mask which refers to all hotkey bindings */
#define SDL_ALL_HOTKEYS        0xFFFFFFFF

///////////////////////////////////////////////////////////

/* Event enumerations */
enum { SDL_NOEVENT = 0,            /* Unused (do not remove) */
       SDL_ACTIVEEVENT,            /* Application loses/gains visibility */
       SDL_KEYDOWN,            /* Keys pressed */
       SDL_KEYUP,            /* Keys released */
       SDL_MOUSEMOTION,            /* Mouse moved */
       SDL_MOUSEBUTTONDOWN,        /* Mouse button pressed */
       SDL_MOUSEBUTTONUP,        /* Mouse button released */
       SDL_JOYAXISMOTION,        /* Joystick axis motion */
       SDL_JOYBALLMOTION,        /* Joystick trackball motion */
       SDL_JOYHATMOTION,        /* Joystick hat position change */
       SDL_JOYBUTTONDOWN,        /* Joystick button pressed */
       SDL_JOYBUTTONUP,            /* Joystick button released */
       SDL_QUIT,            /* User-requested quit */
       SDL_SYSWMEVENT,            /* System specific event */
       SDL_EVENT_RESERVEDA,        /* Reserved for future use.. */
       SDL_EVENT_RESERVEDB,        /* Reserved for future use.. */
       SDL_VIDEORESIZE,            /* User resized video mode */
       SDL_VIDEOEXPOSE,            /* Screen needs to be redrawn */
       SDL_EVENT_RESERVED2,        /* Reserved for future use.. */
       SDL_EVENT_RESERVED3,        /* Reserved for future use.. */
       SDL_EVENT_RESERVED4,        /* Reserved for future use.. */
       SDL_EVENT_RESERVED5,        /* Reserved for future use.. */
       SDL_EVENT_RESERVED6,        /* Reserved for future use.. */
       SDL_EVENT_RESERVED7,        /* Reserved for future use.. */
       /* Events SDL_USEREVENT through SDL_MAXEVENTS-1 are for your use */
       SDL_USEREVENT = 24,
       /* This last event is only for bounding internal arrays
      It is the number of bits in the event mask datatype -- Uint32
        */
       SDL_NUMEVENTS = 32
};

/* Predefined event masks */
#define SDL_EVENTMASK(X)    (1<<(X))
enum {
    SDL_ACTIVEEVENTMASK    = SDL_EVENTMASK(SDL_ACTIVEEVENT),
    SDL_KEYDOWNMASK        = SDL_EVENTMASK(SDL_KEYDOWN),
    SDL_KEYUPMASK        = SDL_EVENTMASK(SDL_KEYUP),
    SDL_MOUSEMOTIONMASK    = SDL_EVENTMASK(SDL_MOUSEMOTION),
    SDL_MOUSEBUTTONDOWNMASK    = SDL_EVENTMASK(SDL_MOUSEBUTTONDOWN),
    SDL_MOUSEBUTTONUPMASK    = SDL_EVENTMASK(SDL_MOUSEBUTTONUP),
    SDL_MOUSEEVENTMASK    = SDL_EVENTMASK(SDL_MOUSEMOTION)|
                              SDL_EVENTMASK(SDL_MOUSEBUTTONDOWN)|
                              SDL_EVENTMASK(SDL_MOUSEBUTTONUP),
    SDL_JOYAXISMOTIONMASK    = SDL_EVENTMASK(SDL_JOYAXISMOTION),
    SDL_JOYBALLMOTIONMASK    = SDL_EVENTMASK(SDL_JOYBALLMOTION),
    SDL_JOYHATMOTIONMASK    = SDL_EVENTMASK(SDL_JOYHATMOTION),
    SDL_JOYBUTTONDOWNMASK    = SDL_EVENTMASK(SDL_JOYBUTTONDOWN),
    SDL_JOYBUTTONUPMASK    = SDL_EVENTMASK(SDL_JOYBUTTONUP),
    SDL_JOYEVENTMASK    = SDL_EVENTMASK(SDL_JOYAXISMOTION)|
                              SDL_EVENTMASK(SDL_JOYBALLMOTION)|
                              SDL_EVENTMASK(SDL_JOYHATMOTION)|
                              SDL_EVENTMASK(SDL_JOYBUTTONDOWN)|
                              SDL_EVENTMASK(SDL_JOYBUTTONUP),
    SDL_VIDEORESIZEMASK    = SDL_EVENTMASK(SDL_VIDEORESIZE),
    SDL_VIDEOEXPOSEMASK    = SDL_EVENTMASK(SDL_VIDEOEXPOSE),
    SDL_QUITMASK        = SDL_EVENTMASK(SDL_QUIT),
    SDL_SYSWMEVENTMASK    = SDL_EVENTMASK(SDL_SYSWMEVENT)
};
#define SDL_ALLEVENTS        0xFFFFFFFF

/* Application visibility event structure */
typedef struct SDL_ActiveEvent {
    Uint8 type;    /* SDL_ACTIVEEVENT */
    Uint8 gain;    /* Whether given states were gained or lost (1/0) */
    Uint8 state;    /* A mask of the focus states */
} SDL_ActiveEvent;

/* Keyboard event structure */
typedef struct SDL_KeyboardEvent {
    Uint8 type;    /* SDL_KEYDOWN or SDL_KEYUP */
    Uint8 which;    /* The keyboard device index */
    Uint8 state;    /* GButtonState::PRESSED or GButtonState::RELEASED */
    SDL_keysym keysym;
} SDL_KeyboardEvent;

/* Mouse motion event structure */
typedef struct SDL_MouseMotionEvent {
    Uint8 type;    /* SDL_MOUSEMOTION */
    Uint8 which;    /* The mouse device index */
    Uint8 state;    /* The current button state */
    Uint16 x, y;    /* The X/Y coordinates of the mouse */
    Sint16 xrel;    /* The relative motion in the X direction */
    Sint16 yrel;    /* The relative motion in the Y direction */
} SDL_MouseMotionEvent;

/* Mouse button event structure */
typedef struct SDL_MouseButtonEvent {
    Uint8 type;    /* SDL_MOUSEBUTTONDOWN or SDL_MOUSEBUTTONUP */
    Uint8 which;    /* The mouse device index */
    Uint8 button;    /* The mouse button index */
    Uint8 state;    /* GButtonState::PRESSED or GButtonState::RELEASED */
    Uint16 x, y;    /* The X/Y coordinates of the mouse at press time */
} SDL_MouseButtonEvent;

/* Joystick axis motion event structure */
typedef struct SDL_JoyAxisEvent {
    Uint8 type;    /* SDL_JOYAXISMOTION */
    Uint8 which;    /* The joystick device index */
    Uint8 axis;    /* The joystick axis index */
    Sint16 value;    /* The axis value (range: -32768 to 32767) */
} SDL_JoyAxisEvent;

/* Joystick trackball motion event structure */
typedef struct SDL_JoyBallEvent {
    Uint8 type;    /* SDL_JOYBALLMOTION */
    Uint8 which;    /* The joystick device index */
    Uint8 ball;    /* The joystick trackball index */
    Sint16 xrel;    /* The relative motion in the X direction */
    Sint16 yrel;    /* The relative motion in the Y direction */
} SDL_JoyBallEvent;

/* Joystick hat position change event structure */
typedef struct SDL_JoyHatEvent {
    Uint8 type;    /* SDL_JOYHATMOTION */
    Uint8 which;    /* The joystick device index */
    Uint8 hat;    /* The joystick hat index */
    Uint8 value;    /* The hat position value:
                SDL_HAT_LEFTUP   SDL_HAT_UP       SDL_HAT_RIGHTUP
                SDL_HAT_LEFT     SDL_HAT_CENTERED SDL_HAT_RIGHT
                SDL_HAT_LEFTDOWN SDL_HAT_DOWN     SDL_HAT_RIGHTDOWN
               Note that zero means the POV is centered.
            */
} SDL_JoyHatEvent;

/* Joystick button event structure */
typedef struct SDL_JoyButtonEvent {
    Uint8 type;    /* SDL_JOYBUTTONDOWN or SDL_JOYBUTTONUP */
    Uint8 which;    /* The joystick device index */
    Uint8 button;    /* The joystick button index */
    Uint8 state;    /* GButtonState::PRESSED or GButtonState::RELEASED */
} SDL_JoyButtonEvent;

/* The "window resized" event
   When you get this event, you are responsible for setting a new video
   mode with the new width and height.
 */
typedef struct SDL_ResizeEvent {
    Uint8 type;    /* SDL_VIDEORESIZE */
    int w;        /* New width */
    int h;        /* New height */
} SDL_ResizeEvent;

/* The "screen redraw" event */
typedef struct SDL_ExposeEvent {
    Uint8 type;    /* SDL_VIDEOEXPOSE */
} SDL_ExposeEvent;

/* The "quit requested" event */
typedef struct SDL_QuitEvent {
    Uint8 type;    /* SDL_QUIT */
} SDL_QuitEvent;

/* A user-defined event type */
typedef struct SDL_UserEvent {
    Uint8 type;    /* SDL_USEREVENT through SDL_NUMEVENTS-1 */
    int code;    /* User defined event code */
    void *data1;    /* User defined data pointer */
    void *data2;    /* User defined data pointer */
} SDL_UserEvent;

/* If you want to use this event, you should include SDL_syswm.h */
struct SDL_SysWMmsg;
typedef struct SDL_SysWMmsg SDL_SysWMmsg;
typedef struct SDL_SysWMEvent {
    Uint8 type;
    SDL_SysWMmsg *msg;
} SDL_SysWMEvent;

/* General event structure */
typedef union {
    Uint8 type;
    SDL_ActiveEvent active;
    SDL_KeyboardEvent key;
    SDL_MouseMotionEvent motion;
    SDL_MouseButtonEvent button;
    SDL_JoyAxisEvent jaxis;
    SDL_JoyBallEvent jball;
    SDL_JoyHatEvent jhat;
    SDL_JoyButtonEvent jbutton;
    SDL_ResizeEvent resize;
    SDL_ExposeEvent expose;
    SDL_QuitEvent quit;
    SDL_UserEvent user;
    SDL_SysWMEvent syswm;
} SDL_Event;

#endif // platform

#endif
