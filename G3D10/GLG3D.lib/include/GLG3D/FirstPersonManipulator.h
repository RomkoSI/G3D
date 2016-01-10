/**
  \file GLG3D/FirstPersonManipulator.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2002-07-28
  \edited  2011-09-03
*/

#ifndef G3D_FirstPersonManipulator_h
#define G3D_FirstPersonManipulator_h

#include "G3D/platform.h"
#include "G3D/Vector3.h"
#include "G3D/CoordinateFrame.h"
#include "GLG3D/Widget.h"

namespace G3D {

class CoordinateFrame;

/**
 Uses a First Person (Quake- or World of Warcraft style) mapping to translate keyboard and mouse input
 into a flying camera position.  The result is an Euler-angle 
 camera controller suitable for games and fly-throughs.  

  To use without G3D::GApp:
  <OL>
    <LI> Create a G3D::RenderDevice
    <LI> Create a UserInput object (set the keyboard controls when creating it)
    <LI> Create a ManualCameraController
    <LI> Call ManualCameraController::setActive(true)
    <LI> Invoke ManualCameraController::doSimulation every time simulation is invoked (e.g. once per rendering iteration)
    <LI> Use ManualCameraController::getCoordinateFrame() to set the camera's position
  </OL>

 */
class FirstPersonManipulator : public Manipulator {
public:
    enum MouseMode {
        /** Shooter/Quake style (default), mouse cursor is hidden and mouse controls yaw/pitch */
        MOUSE_DIRECT,

        /** RPG/World of Warcraft style, on right mouse button cursor
            is hidden and mouse controls yaw/pitch.  To support single-button systems
            the combination ctrl+left button is also treated as if it were the right
            mouse button.*/
        MOUSE_DIRECT_RIGHT_BUTTON,

        /** Leaves mouse cursor visible and rotates
            while mouse is near the window edge. */
        MOUSE_SCROLL_AT_EDGE,

        /** RTS style. Leaves mouse cursor visible and rotates when
            the mouse is actively pushing against the window edge */
        MOUSE_PUSH_AT_EDGE
    };

private:

    GKey                        m_flyUpKey;
    GKey                        m_flyDownKey;

    GKey                        m_yawLeftKey;
    GKey                        m_yawRightKey;

    /** m/s */
    float                       m_maxMoveRate;
    
    /** rad/s */
    float                       m_maxTurnRate;

    /** \copydoc shiftSpeedMultipler */
    float                       m_shiftSpeedMultiplier;

    /** \copydoc altSpeedMultipler */
    float                       m_altSpeedMultiplier;
    
    float                       m_yaw;
    float                       m_pitch;
    Vector3                     m_translation;

    bool                        m_enabled;

    class UserInput*            m_userInput;

    MouseMode                   m_mouseMode;

    /** Is the right mouse button currently held down? */
    bool                        m_rightDown;


    /** Returns true if the right mouse button is down, or on OSX, the
        user is using the left mouse and ctrl */
    bool rightDown(UserInput*) const;

    FirstPersonManipulator(UserInput* ui);

public:
    
    /** If the UserInput is not provided here, it is automatically recorded 
    after the first onUserInput method invocation */
    static shared_ptr<FirstPersonManipulator> create(UserInput* ui = NULL);
        
    /** Deactivates the controller */
    virtual ~FirstPersonManipulator();

    /** When enabled, the FirstPersonManipulator responds to events and updates its frame.

        When deactivated, the mouse cursor is restored and the mouse is located
        where it was when the camera controller was activated.

        In some modes (e.g., MOUSE_DIRECT_RIGHT_BUTTON), the FirstPersonManipulator does not
        hide the mouse until some other event has occured.

        In release mode, the cursor movement is restricted to the window
        while the controller is active.  This does not occur in debug mode because
        you might hit a breakpoint while the controller is active and it
        would be annoying to not be able to move the mouse.*/
    void setEnabled(bool a) override;

    MouseMode mouseMode() const;

    void setMouseMode(MouseMode m);

    bool enabled() const override;

    /** Initial value is 10 */
    void setMoveRate(double metersPerSecond);

    /** Initial value is PI / 2 */
    void setTurnRate(double radiansPerSecond);

    /** Invoke immediately before entering the main game loop. */
    void reset();

    /** When the shift key is held down, multiply m_maxMoveRate by this. Default is 0.1 */
    void setShiftSpeedMultipler(float m) {
        m_shiftSpeedMultiplier = m;
    }

    float shiftSpeedMultipler() const {
        return m_shiftSpeedMultiplier;
    }

    /** When the alt key is held down, multiply m_maxMoveRate by this. Default is 0.1 */
    void setAltSpeedMultipler(float m) {
        m_altSpeedMultiplier = m;
    }

    float altSpeedMultipler() const {
        return m_altSpeedMultiplier;
    }

    /** Key for translating +Y */
    GKey flyUpKey() const {
        return m_flyUpKey;
    }

    /** Key for translating -Y */
    GKey flyDownKey() const {
        return m_flyDownKey;
    }

    /** Default is 'z' */
    void setFlyUpKey(GKey k) {
        m_flyUpKey = k;
    }

    /** Default is 'c' */
    void setFlyDownKey(GKey k) {
        m_flyDownKey = k;
    }

    void setPosition(const Vector3& t) {
        m_translation = t;
    }

    void lookAt(const Vector3& position);

    float yaw() const {
        return m_yaw;
    }

    float pitch() const {
        return m_pitch;
    }

    void setYaw(float y) {
        m_yaw = y;
    }

    void setPitch(float p) {
        m_pitch = p;
    }

    const Vector3& translation() const {
        return m_translation;
    }

    /**
      Sets to the closest legal controller orientation to the coordinate frame.
     */
    virtual void setFrame(const CoordinateFrame& c) override;

    // Inherited from Manipulator
    virtual void getFrame(CoordinateFrame& c) const override;

    virtual CoordinateFrame frame() const override;

    // Inherited from Widget
    virtual void onPose(Array<shared_ptr<Surface>>& p3d, Array<shared_ptr<Surface2D>>& p2d) override;

    virtual void onNetwork() override;
    virtual void onAI() override;
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override;
    virtual void onUserInput(UserInput* ui) override;
    virtual bool onEvent(const GEvent& event) override;

    Vector3 lookVector() const {
        return frame().lookVector();
    }
};

}
#endif
