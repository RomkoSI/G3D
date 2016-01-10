#include "GLG3D/Xbox360Controller.h"
#include "GLG3D/OSWindow.h"

namespace G3D {

Xbox360Controller::Button& Xbox360Controller::button(GKey k) {
    alwaysAssertM(k >= GKey::CONTROLLER_A && k <= GKey::CONTROLLER_GUIDE, 
        "Not a controller button");

    return m_buttonArray[k - GKey::CONTROLLER_A];
}


const Xbox360Controller::Stick& Xbox360Controller::stick(StickIndex s) const {
    alwaysAssertM(s >= LEFT && s <= TRIGGER, 
        "Not an analog stick");

    return m_stickArray[s];
}


// Since Widget::onAfterEvents is always called from a single
// thread, we can share these arrays over all instances of Xbox360Controller
static Array<float> axis;
static Array<bool>  buttonArray;


// Different operating system drivers seem to map the Xbox360 controller differently
#ifdef G3D_WINDOWS
    enum { RIGHT_X_AXIS = 4, RIGHT_Y_AXIS = 3, LEFT_X_AXIS = 0, LEFT_Y_AXIS = 1, TRIGGER_X_AXIS = 2, TRIGGER_Y_AXIS = 2, NUM_AXES = 5};
    static const int buttonRemap[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, -1};
# else
    // Currently set based on OSX driver, which has inverted axes
    enum { RIGHT_X_AXIS = 2, RIGHT_Y_AXIS = 3, LEFT_X_AXIS = 0, LEFT_Y_AXIS = 1, TRIGGER_X_AXIS = 4, TRIGGER_Y_AXIS = 5, NUM_AXES = 6};
    static const int buttonRemap[] = {11, 12, 13, 14, 8, 9, 5, 4, 6, 7, 0, 3, 1, 2, 10};
#endif

void Xbox360Controller::onAfterEvents() {
    axis.fastClear();
    buttonArray.fastClear();

    for (int s = 0; s < NUM_STICKS; ++s) {
        m_stickArray[s].previousValue = m_stickArray[s].currentValue;
    }

    OSWindow* window = m_manager->window();

    m_present = (window->numJoysticks() > int(m_joystickNumber));
    if (! m_present) { return; }

    window->getJoystickState(m_joystickNumber, axis, buttonArray);
#   ifdef G3D_WINDOWS
        // Guide button is never reported on windows
        m_present = (axis.size() >= NUM_AXES) && (buttonArray.size() >= NUM_BUTTONS - 1);
#   else
        m_present = (axis.size() >= NUM_AXES) && (buttonArray.size() >= NUM_BUTTONS);
#   endif
    if (! m_present) { return; }

    m_stickArray[LEFT].currentValue.x    = axis[LEFT_X_AXIS];
    m_stickArray[LEFT].currentValue.y    = axis[LEFT_Y_AXIS];

    m_stickArray[RIGHT].currentValue.x   = axis[RIGHT_X_AXIS];
    m_stickArray[RIGHT].currentValue.y   = axis[RIGHT_Y_AXIS];

    m_stickArray[TRIGGER].currentValue.x = axis[TRIGGER_X_AXIS];
    m_stickArray[TRIGGER].currentValue.y = axis[TRIGGER_Y_AXIS];

    for (int b = 0; b < NUM_BUTTONS; ++b) {
        const int index = buttonRemap[b];
        // For the guide button, which doesn't work on Windows
        const bool newValue = (index >= 0) ? buttonArray[index] : false;
        Button& button = m_buttonArray[buttonRemap[b]];
        button.changed = (newValue != button.currentValue);
        button.currentValue = newValue;
    }
}


void Xbox360Controller::setManager(WidgetManager* m) {
    Widget::setManager(m);
    // Update state for the first time, notably, the m_present flag.
    onAfterEvents();
}


float Xbox360Controller::angleDelta(StickIndex s) const {
    const static float threshold = 0.2f;
    const Stick& st = stick(s);

    if ((st.previousValue.length() < threshold) || (st.currentValue.length() < threshold)) {
        // The stick was too close to the center to measure angles
        return 0.0f;
    } else {
        const float oldAngle = atan2(st.previousValue.y, st.previousValue.x);
        const float newAngle = atan2(st.currentValue.y,  st.currentValue.x);
        float delta = newAngle - oldAngle;

        // Make sure we go the short way

        if (delta > pif()) {
            delta -= 2.0f * pif();
        } else if (delta < -pif()) {
            delta += 2.0f * pif();
        }

        return delta;
    }
}

} // namespace G3D

