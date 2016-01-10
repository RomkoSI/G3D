/**
  \file GLG3D.lib/source/FirstPersonManipulator.cpp

  \maintainer Morgan McGuire, morgan@cs.brown.edu

  \created 2002-07-28
  \edited  2013-10-20
*/

#include "G3D/platform.h"
#include <cmath>
#include "G3D/Rect2D.h"
#include "GLG3D/FirstPersonManipulator.h"
#include "GLG3D/OSWindow.h"
#include "GLG3D/UserInput.h"

namespace G3D {

shared_ptr<FirstPersonManipulator> FirstPersonManipulator::create(UserInput* ui) {
    return shared_ptr<FirstPersonManipulator>(new FirstPersonManipulator(ui));
}


FirstPersonManipulator::FirstPersonManipulator(UserInput* ui) : 
    m_flyUpKey('z'),
    m_flyDownKey('c'),
    m_yawLeftKey('q'),
    m_yawRightKey('e'),
    m_maxMoveRate(10),
    m_maxTurnRate(20),
    m_shiftSpeedMultiplier(0.1f),
    m_altSpeedMultiplier(6.0f),
    m_yaw(0),
    m_pitch(0),
    m_enabled(false),
    m_userInput(ui),
    m_mouseMode(MOUSE_DIRECT),
    m_rightDown(false) {
}


void FirstPersonManipulator::getFrame(CoordinateFrame& c) const {
    c.translation = m_translation;
    c.rotation = Matrix3::fromEulerAnglesZYX(0, -m_yaw, -m_pitch);

    debugAssert(isFinite(c.rotation[0][0]));

    debugAssertM(c.rotation[1][1] >= 0, 
        "y-axis tipped under the equator due to an internal "
        "inconsistency in FirstPersonManipulator");

    debugAssertM(fuzzyEq(c.rotation[1][0], 0.0f),
        "x-axis is not in the plane of the equator due to an internal "
        "inconsistency in FirstPersonManipulator");
}


CoordinateFrame FirstPersonManipulator::frame() const {
    CoordinateFrame c;
    getFrame(c);
    return c;
}


FirstPersonManipulator::~FirstPersonManipulator() {
}


FirstPersonManipulator::MouseMode FirstPersonManipulator::mouseMode() const {
    return m_mouseMode;
}


void FirstPersonManipulator::setMouseMode(FirstPersonManipulator::MouseMode m) {
    if (m_mouseMode != m) {
        bool wasEnabled = enabled();

        if (wasEnabled) {
            // Toggle activity to let the cursor and 
            // state variables reset.
            setEnabled(false);
        }

        m_mouseMode = m;

        if (wasEnabled) {
            setEnabled(true);
        }
    }
}


bool FirstPersonManipulator::enabled() const {
    return m_enabled;
}


void FirstPersonManipulator::reset() {
    m_enabled     = false;
    m_rightDown   = false;
    m_yaw         = -(float)halfPi();
    m_pitch       = 0;
    m_translation = Vector3::zero();
    setMoveRate(10);

#   ifdef G3D_OSX
        // OS X has a really slow mouse by default
        setTurnRate(pi() * 12);
#   else
        setTurnRate(pi() * 5);
#   endif
}


bool FirstPersonManipulator::rightDown(UserInput* ui) const {
    return 
        ui->keyDown(GKey::RIGHT_MOUSE) || 
        (ui->keyDown(GKey::LEFT_MOUSE) && 
        (ui->keyDown(GKey::LCTRL) ||
            ui->keyDown(GKey::RCTRL)));
}


void FirstPersonManipulator::setEnabled(bool a) {
    if (m_enabled == a) {
        return;
    }
    m_enabled = a;

    debugAssertM(m_userInput != NULL, 
                 "Cannot call FirstPersonManipulator::setEnabled() before the WidgetManager"
                 " has called onUserInput (i.e., cannot call setEnabled on the first frame)");

    switch (m_mouseMode) {
    case MOUSE_DIRECT:
        m_userInput->setPureDeltaMouse(m_enabled);
        break;

    case MOUSE_DIRECT_RIGHT_BUTTON:
        // Only turn on when enabled and the right mouse button is down before
        // activation.
        m_userInput->setPureDeltaMouse(m_enabled && rightDown(m_userInput));
        break;

    case MOUSE_SCROLL_AT_EDGE:
    case MOUSE_PUSH_AT_EDGE:        
        m_userInput->setPureDeltaMouse(false);
        if (m_enabled) {
            m_userInput->window()->incInputCaptureCount();
        } else {
            m_userInput->window()->decInputCaptureCount();
        }
        break;

    default:
        debugAssert(false);
    }
}


void FirstPersonManipulator::setMoveRate(double metersPerSecond) {
    m_maxMoveRate = (float)metersPerSecond;
}


void FirstPersonManipulator::setTurnRate(double radiansPerSecond) {
    m_maxTurnRate = (float)radiansPerSecond;
}


void FirstPersonManipulator::lookAt(const Point3& position) {

    const Vector3 look = (position - m_translation);

    m_yaw   =  float(aTan2(look.x, -look.z));
    m_pitch = -float(aTan2(look.y, distance(look.x, look.z)));
}


void FirstPersonManipulator::setFrame(const CoordinateFrame& c) {
    const Vector3& look = c.lookVector();

    setPosition(c.translation);
    lookAt(c.translation + look);
}


void FirstPersonManipulator::onPose(Array<shared_ptr<Surface> >& p3d, Array<Surface2DRef>& p2d) {
}


void FirstPersonManipulator::onNetwork() {
}


void FirstPersonManipulator::onAI() {
}


void FirstPersonManipulator::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    if (! m_enabled) {
        return;
    }

    if (m_userInput == NULL) {
        return;
    }

    RealTime elapsedTime = rdt;
    const float maxTurn = m_maxTurnRate * (float)elapsedTime;
    Vector2 joyRotate;

    bool wantUp   = m_userInput->keyDown(m_flyUpKey);
    bool wantDown = m_userInput->keyDown(m_flyDownKey);
    bool wantSlow = m_userInput->keyDown(GKey::LSHIFT) || m_userInput->keyDown(GKey::RSHIFT);
    bool wantFast = m_userInput->keyDown(GKey::LALT) || m_userInput->keyDown(GKey::RALT);

#   ifdef G3D_WINDOWS
       enum { YAW_AXIS = 4, PITCH_AXIS = 3, X_AXIS = 0, Y_AXIS = 1, UP_BUTTON = 10, DOWN_BUTTON = 12, SLOW_BUTTON = 4, FAST_BUTTON = 5};
       static const float PITCH_SIGN = 1.0f;
#   else
       // Currently set based on OSX driver, which has inverted axes
       enum { YAW_AXIS = 2, PITCH_AXIS = 3, X_AXIS = 0, Y_AXIS = 1, UP_BUTTON = 0, DOWN_BUTTON = 1, SLOW_BUTTON = 8, FAST_BUTTON = 9};
       static const float PITCH_SIGN = -1.0f;
#   endif

    // Joystick processing 
    if (window()->numJoysticks() > 0) {
        static Array<float> axis;
        static Array<bool> button;
        window()->getJoystickState(0, axis, button);
        if ((axis.size() >= 4) && (button.size() >= 12)) {
            wantUp   = wantUp   || button[UP_BUTTON];
            wantDown = wantDown || button[DOWN_BUTTON];
            wantSlow = wantSlow || button[SLOW_BUTTON];
            wantFast = wantFast || button[FAST_BUTTON];

            joyRotate = Vector2(axis[YAW_AXIS], axis[PITCH_AXIS] * PITCH_SIGN);

            // Create a dead zone
            static const float threshold = 0.1f;

            // Scale non-linearly
            static const float exp = 2.5f;

            
            for (int i = 0; i < 2; ++i) {
                // The sign() function and implicit abs() compile
                // incorrectly on OS X under clang++ here so we
                // explicitly spell it them out.
                const float s = (joyRotate[i] > 0.0f) ? 1.0f : -1.0f;
                joyRotate[i] = maxTurn * 0.22f * s *
                    std::pow(std::max(0.0f, std::abs(joyRotate[i]) - threshold) /
                             (1.0f - threshold), exp);
            }
        }
    } // if joystick

    {
        // Translation direction
        float dy = 0;

        if (wantUp && ! wantDown) {
            dy = 1;
        } else if (! wantUp && wantDown) {
            dy = -1;
        }

        float modifier = 1.0f;

        if (wantSlow) {
            modifier *= m_shiftSpeedMultiplier;
        }
        
        if (wantFast) {
            modifier *= m_altSpeedMultiplier;
        }

        const Vector3& direction =
            Vector3(m_userInput->getX(), dy, m_userInput->getY()).directionOrZero();

        // Translate forward
        m_translation += (frame().rightVector() * direction.x + 
                          frame().upVector()    * direction.y +
                          frame().lookVector()  * direction.z) *
            float(elapsedTime) * m_maxMoveRate * modifier;
    }
    
    // Desired change in yaw and pitch
    Vector2 mouseRotate;

    switch (m_mouseMode) {
    case MOUSE_DIRECT_RIGHT_BUTTON:
        m_userInput->setPureDeltaMouse(m_rightDown);
        if (! m_rightDown) {
            // Skip bottom case
            break;
        }
        // Intentionally fall through to MOUSE_DIRECT

    case MOUSE_DIRECT:
        // Time is not a factor in rotation because the mouse movement has already been
        // integrated over time (unlike keyboard presses)
        mouseRotate = m_maxTurnRate * m_userInput->mouseDXY() / 2000.0f;
        break;


    case MOUSE_SCROLL_AT_EDGE:
        {
            Rect2D viewport = 
                Rect2D::xywh(0, 0, float(m_userInput->window()->width()), 
                             float(m_userInput->window()->height()));
            Vector2 mouse = m_userInput->mouseXY();

            Vector2 hotExtent(max(50.0f, viewport.width() / 8), 
                              max(50.0f, viewport.height() / 6));

            // The hot region is outside this rect
            Rect2D hotRegion = Rect2D::xyxy(
                viewport.x0() + hotExtent.x, viewport.y0() + hotExtent.y,
                viewport.x1() - hotExtent.y, viewport.y1() - hotExtent.y);

            // See if the mouse is near an edge
            if (mouse.x <= hotRegion.x0()) {
                mouseRotate.x = -square(1.0f - (mouse.x - viewport.x0()) / hotExtent.x);
                // - Yaw
            } else if (mouse.x >= hotRegion.x1()) {
                mouseRotate.x = square(1.0f - (viewport.x1() - mouse.x) / hotExtent.x);
                // + Yaw
            }

            if (mouse.y <= hotRegion.y0()) {
                mouseRotate.y = -square(1.0f - (mouse.y - viewport.y0()) / hotExtent.y) * 0.6f;
                // - pitch
            } else if (mouse.y >= hotRegion.y1()) {
                mouseRotate.y = square(1.0f - (viewport.y1() - mouse.y) / hotExtent.y) * 0.6f;
                // + pitch
            }

            mouseRotate *= maxTurn / 5;
        }
        break;

    case MOUSE_PUSH_AT_EDGE: 
        break;

    default:
        debugAssert(false);
    }

    m_yaw   += joyRotate.x;
    m_pitch += joyRotate.y;

    m_yaw   += mouseRotate.x;
    m_pitch += mouseRotate.y;

    {
        // Yaw change using keyboard
        float dyawKeyboard = 0;
        if (m_userInput->keyDown(m_yawLeftKey) && ! m_userInput->keyDown(m_yawRightKey)) {
            dyawKeyboard = -1;
        } else if (! m_userInput->keyDown(m_yawLeftKey) && m_userInput->keyDown(m_yawRightKey)) {
            dyawKeyboard = 1;
        }
        
        m_yaw +=  dyawKeyboard * m_maxTurnRate * float(elapsedTime) * 0.2f; //way too fast otherwise
    }

    // As a patch for a setCoordinateFrame bug, we prevent 
    // the camera from looking exactly along the y-axis.
    m_pitch = clamp(m_pitch, (float)-halfPi() + 0.001f, (float)halfPi() - 0.001f);

    debugAssert(isFinite(m_yaw));
    debugAssert(isFinite(m_pitch));
}


void FirstPersonManipulator::onUserInput(UserInput* ui) {
    m_userInput = ui;
}


bool FirstPersonManipulator::onEvent(const GEvent& event) {
    if (m_enabled && 
        (m_mouseMode == MOUSE_DIRECT_RIGHT_BUTTON) && 
        ((event.type == GEventType::MOUSE_BUTTON_DOWN) ||
         (event.type == GEventType::MOUSE_BUTTON_UP))) {

        // This may be the "right-click" (OS dependent) that will
        // start camera movement.  If it is, we don't want other
        // Widgets to see the event.

        //debugPrintf("Button = %d\n", event.button.button);

        if (event.button.button == 2) {
            // Right click
            m_rightDown = (event.type == GEventType::MOUSE_BUTTON_DOWN);
            return true;
        }


        if ((m_userInput != NULL) && (event.button.button == 0)) {
            // "Right click"
            if ((event.type == GEventType::MOUSE_BUTTON_DOWN) && 
                (m_userInput->keyDown(GKey::LCTRL)  ||
                 m_userInput->keyDown(GKey::RCTRL))) {
                m_rightDown = true;
                return true;
            }

            if (event.type == GEventType::MOUSE_BUTTON_UP) {
                // Only preserve the "right mouse is down" state if the 
                // physical right mouse button is actually down.
                m_rightDown = m_userInput->keyDown(GKey::RIGHT_MOUSE);
                if (m_rightDown) {
                    return true;
                }
            }
            
        }

    }

    return false;
}


}

