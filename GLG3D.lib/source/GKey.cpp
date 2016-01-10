/**
  \file GLG3D/source/GKey.cpp

  \created       2007-01-27
  \edited        2013-01-07
    
  Copyright 2000-2015, Morgan McGuire.
  All rights reserved.
*/
#include "GLG3D/GKey.h"
#include "G3D/stringutils.h"

namespace G3D {

GKey GKey::fromString(const String& _s) {
    String s = trimWhitespace(toLower(_s));    

    for (int i = 0; i < LAST; ++i) {
        String t = GKey(i).toString();
        if ((t.size() == s.size()) &&
            (toLower(t) == s)) {
            return GKey(i);
        }
    }

    return UNKNOWN;
}


String GKey::toString() const {
    // Cast to int to avoid warning about char constants in the switch
    switch (int(value)) {
    case LEFT_MOUSE:
        return "L Mouse";

    case MIDDLE_MOUSE:
        return "Mid Mouse";

    case RIGHT_MOUSE:
        return "R Mouse";
    
    case MOUSE_WHEEL_UP:
        return "MWheel Up";

    case MOUSE_WHEEL_DOWN:
        return "MWheel Dn";

    case BACKSPACE:
        return "Bksp";
        
    case TAB:
        return "Tab";

    case CLEAR:
        return "Clear";
        
    case RETURN:
        return "Enter";

    case PAUSE:
        return "Pause";

    case ESCAPE:
        return "Esc";

    case SPACE:
        return "Spc";

    case EXCLAIM:
    case QUOTEDBL:
    case HASH:
    case DOLLAR:
    case AMPERSAND:
    case QUOTE:
    case LEFTPAREN:
    case RIGHTPAREN:
    case ASTERISK:
    case PLUS:
    case COMMA:
    case MINUS:
    case PERIOD:
    case SLASH:
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case COLON:
    case SEMICOLON:
    case LESS:
    case EQUALS:
    case GREATER:
    case QUESTION:
    case AT:
    case LEFTBRACKET:
    case BACKSLASH:
    case RIGHTBRACKET:
    case CARET:
    case UNDERSCORE:
    case BACKQUOTE:
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
        return String("") + (char)toupper(value);

    case DELETE:
        return "Del";

    case KP0:
    case KP1:
    case KP2:
    case KP3:
    case KP4:
    case KP5:
    case KP6:
    case KP7:
    case KP8:
    case KP9:
        return String("Keypad ") + (char)('0' + (int)(value - KP0)); 

    case KP_PERIOD:
        return "Keypad .";

    case KP_DIVIDE:
        return "Keypad \\";

    case KP_MULTIPLY:
        return "Keypad *";

    case KP_MINUS:
        return "Keypad -";

    case KP_PLUS:
        return "Keypad +";

    case KP_ENTER:
        return "Keypad Enter";

    case KP_EQUALS:
        return "Keypad =";

    case UP:
        return "Up";

    case DOWN:
        return "Down";

    case RIGHT:
        return "Right";

    case LEFT:
        return "Left";

    case INSERT:
        return "Ins";

    case HOME:
        return "Home";

    case END:
        return "End";

    case PAGEUP:
        return "Pg Up";

    case PAGEDOWN:
        return "Pg Dn";

    case F1:
    case F2:
    case F3:
    case F4:
    case F5:
    case F6:
    case F7:
    case F8:
    case F9:
        return String("F") + (char)('1' + (int)(value - F1));

    case F10:
    case F11:
    case F12:
    case F13:
    case F14:
    case F15:
        // Key codes for F10...F15 require two characters
        return String("F1") + (char)('0' + (int)(value - F10));

    case NUMLOCK:
        return "Num Lock";

    case CAPSLOCK:
        return "Caps Lock";

    case SCROLLOCK:
        return "Scrl Lock";

    case RSHIFT:
        return "R Shift";
        
    case LSHIFT:
        return "L Shift";

    case RCTRL:
        return "R Ctrl";

    case LCTRL:
        return "L Ctrl";

    case RALT:
        return "R Alt";

    case LALT:
        return "L Alt";

    case RMETA:
        return "R Meta";

    case LMETA:
        return "L Meta";

    case LSUPER:
        return "L Win";

    case RSUPER:
        return "R Win";

    case MODE:
        return "Alt Gr";

    case HELP:
        return "Help";

    case PRINT:
        return "Print";

    case SYSREQ:
        return "Sys Req";

    case BREAK:
        return "Break";

    case CONTROLLER_A:
        return "A Button";

    case CONTROLLER_B:
        return "B Button";

    case CONTROLLER_X:
        return "X Button";

    case CONTROLLER_Y:
        return "Y Button";

    case CONTROLLER_LEFT_BUMPER:
        return "Lt Bumper";

    case CONTROLLER_RIGHT_BUMPER:
        return "Rt Bumper";

    case CONTROLLER_LEFT_CLICK:
        return "Lt Stick Press";

    case CONTROLLER_RIGHT_CLICK:
        return "Rt Stick Press";

    case CONTROLLER_DPAD_RIGHT:
        return "DPad Rt";

    case CONTROLLER_DPAD_UP:
        return "DPad Up";

    case CONTROLLER_DPAD_LEFT:
        return "DPad Lt";

    case CONTROLLER_DPAD_DOWN:
        return "DPad Dn";

    case CONTROLLER_BACK:
        return "Back Button";

    case CONTROLLER_GUIDE:
        return "Guide Button";

    case CONTROLLER_START:
        return "Start Button";

    default:
        return format("Key 0x%x", value);
    }

    return "";
}

}
