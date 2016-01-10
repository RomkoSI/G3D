/**
 \file GLG3D/Widget.h

 \maintainer Morgan McGuire, morgan3d@users.sourceforge.net

 \created 2006-04-22
 \edited  2014-07-30
 
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/

#ifndef GLG3D_Widget_h
#define GLG3D_Widget_h

#include "G3D/platform.h"
#include "G3D/Array.h"
#include "G3D/Projection.h"
#include "G3D/ReferenceCount.h"
#include "GLG3D/Surface.h"
#include "GLG3D/OSWindow.h"

namespace G3D {

class RenderDevice;
class UserInput;
class WidgetManager;

/** Used by Widget%s for mapping between 2D events and 3D rendering. */
class EventCoordinateMapper {
protected:
    Projection              m_lastProjection;
    CFrame                  m_lastCameraToWorldMatrix;
    Rect2D                  m_lastViewport;
    Vector2                 m_lastGuardBandOffset;

public:

    EventCoordinateMapper() : m_lastGuardBandOffset(Vector2::nan()) {}

    void update(RenderDevice* rd);

	/** True if update has been called, so eventPixelToCameraSpaceRay may be used*/
	bool ready() const {
		return ! m_lastGuardBandOffset.isNaN();
	}

    Ray eventPixelToCameraSpaceRay(const Point2& pixel) const;
};

/**
 Interface for 2D or 3D objects that experience standard virtual world
 events and are rendered.

 Widget is an interface for GUI-like objects.  You could think of it
 as a bare-bones scene graph.

 Widgets are objects like the G3D::FirstPersonController,
 G3D::GConsole, and debug text overlay that need to receive almost the
 same set of events (onXXX methods) as G3D::GApp and that you would
 like to be called from the corresponding methods of a G3D::GApp.
 They are a way to break large pieces of functionality for UI and
 debugging off so that they can be mixed and matched.

 Widget inherits G3D::Surface2D because it is often convenient to
 implement a 2D widget whose onPose method adds itself to the
 rendering array rather than using a proxy object.

 \sa Entity, GuiControl
 */
class Widget : public Surface2D {
protected:

    /** The manager, set by setManager().  This cannot be a reference
        counted pointer because that would create a cycle between the
        Widget and its manager. */
    WidgetManager* m_manager;

    float          m_depth;

    Widget() : m_manager(NULL), m_depth(0.5f) {}

public:

    typedef shared_ptr<class Widget> Ref;

    /** 
     Appends a posed model for this object to the array, if it has a
     graphic representation.  The posed model appended is allowed to
     reference the agent and is allowed to mutate if the agent is
     mutated.
     */
    virtual void onPose(
        Array< shared_ptr<Surface> >& surfaceArray,
        Array< shared_ptr<Surface2D> >& surface2DArray) {
        (void)surfaceArray;
        (void)surface2DArray;
    }

    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
        (void)rdt;
        (void)sdt;
        (void)idt;
    }

    /** Called by the WidgetManager when this module is added to it.  The argument may be NULL */
    virtual void setManager(WidgetManager* m) {
        m_manager = m;
    }

    WidgetManager* manager() const {
        return m_manager;
    }


    /** The camera-space z position that this Widget considers this pixel to be at.
        Used for positional event (e.g., mouse click) delivery. Higher means closer.

        Large negative values are far away. 0 is the closest 3D object. Values greater 
        than zero are used for 2D objects. Returning fnan() [the default] requests
        the WidgetManager to set hte positionalEventZ to the object's normal event
        zorder.

        See the implementation of ControlPointEditor::positionalEventZ for an example
        of how to use this with a 3D object.
    */
    virtual float positionalEventZ(const Point2& pixel) const {
        return fnan();
    }

    /** Fire an event on the containing window */
    virtual void fireEvent(const GEvent& event);

    /** Returning true consumes the event and prevents other GModules
        from seeing it.  Motion events (GEventType::MOUSEMOTION,
        GEventType::JOYHATMOTION, JGEventType::OYBALLMOTION, and
        GEventType::JOYAXISMOTION) cannot be cancelled.
     */
    virtual bool onEvent(const GEvent& event) { (void)event; return false;}

    /** Invoked after all onEvent handlers for the current frame and before onUserInput for
        any other Widget. */
    virtual void onAfterEvents() {}

    virtual void onUserInput(UserInput* ui) { (void)ui;}

    virtual void onNetwork() {}

    virtual void onAI() {}

    /** Returns the operating system window that is currently
        rendering this Widget. */
    virtual OSWindow* window() const;

    /** Inherited from Surface2D */
    virtual void render(RenderDevice* rd) const override { (void)rd; }

    /** Inherited from Surface2D */
    virtual Rect2D bounds() const override {
        return AABox2D(-Vector2::inf(), Vector2::inf());
    }

    /** Inherited from Surface2D.  Controls the depth of objects when
     rendering.  Subclasses may override this but it can interfere
     with the normal handling of rendering and event delivery. depth =
     0 is usually the "top" widget and depth = 1 is usually the
     "bottom" widget. */
    virtual float depth() const override { 
        return m_depth; 
    }

    /** Called by the WidgetManager.  This is the depth that the Widget is expected to use when posed as a Surface2D.
      Subclasses may override or ignore this but it can interfere with the normal handling of rendering. */
    virtual void setDepth(float d) {
        m_depth = d;
    }
};

/** \brief A widget that renders below everything else and fills the OSWindow. */
class FullScreenWidget : public Widget {
protected:

    FullScreenWidget() {}

public:

    virtual float depth() const override {
        return finf();
    }


    virtual Rect2D bounds() const override {
        return window()->clientRect() - window()->clientRect().x0y0();
    }

    virtual void onPose(Array< shared_ptr< Surface > > &surfaceArray, Array< shared_ptr< Surface2D > > &surface2DArray) override {
        surface2DArray.append(dynamic_pointer_cast<Widget>(shared_from_this()));
    }
};

/**
 Manages a group of G3D::Widget.  This is used internally by G3D::GApp
 to process its modules.  It also enables use of Widgets without the
 G3D::GApp infrastructure.  Most users do not need to use this class.

 You can use G3D::Widget without this class.
 */
class WidgetManager : public Widget {
public:

    typedef shared_ptr<class WidgetManager> Ref;

private:
    /** Manages events that have been delayed by a lock.  Not related
        to GEvent in any way. */
    class DelayedEvent {
    public:
        enum Type {REMOVE_ALL, REMOVE, ADD, SET_FOCUS, SET_FOCUS_AND_MOVE_TO_FRONT, SET_DEFOCUS, MOVE_TO_BACK};

        Type        type;
        shared_ptr<Widget> module;

        DelayedEvent(Type type = ADD, const shared_ptr<Widget>& module = shared_ptr<Widget>()) : type(type), module(module) {}
    };
    
    /** Events are delivered in DECREASING index order.  */
    Array<shared_ptr<Widget> >  m_moduleArray;

    bool                        m_locked;

    /** The widget that will receive events first. This is usually but
        not always the top Widget in m_moduleArray. */
    shared_ptr<Widget>          m_focusedModule;

 
    /** To be processed in endLock */
    Array<DelayedEvent>         m_delayedEvent;

    /** Operating system window */
    OSWindow*                   m_window;

    WidgetManager();

    /** Assigns a depth to each widget based on its current position in m_moduleArray and then
        sorts by depth.  The sorting pass is needed because some widgets will not accept
        their assigned position and will move up or down the array anyway. */
    void updateWidgetDepths();

public:

    /** \param window The window that generates events for this manager.*/
    static shared_ptr<WidgetManager> create(OSWindow* window);

    /** 
      Between beginLock and endLock, add and remove operations are
      delayed so that iteration is safe.  Locks may not be executed
      recursively; only one level of locking is allowed.  If using with
      G3D::GApp, allow the G3D::GApp to perform the locking for you.
      */
    void beginLock();

    void endLock();

    /** Widgets currently executing. Note that some widgets may have already been added
        but if the WidgetManager is locked they will not appear in this array yet.*/
    inline const Array<shared_ptr<Widget> >& widgetArray() const {
        return m_moduleArray;
    }

    /** Pushes this widget to the back of the z order.  This window will render first and receive events last.
        This is the opposite of focussing a window.
     */
    void moveWidgetToBack(const shared_ptr<Widget>& widget);

    /** At most one Widget has focus at a time.  May be NULL. 
 
        Note that a G3D::GuiWindow is a single Widget that then delegates key focus to one GuiControl
        within itself.
      */
    shared_ptr<Widget> focusedWidget() const;

    /** The Widget must have already been added.  This widget will be moved to
        the top of the priority list (i.e., it will receive events first).
        You can set the focussed widget to NULL.

        If you change the focus during a lock, the actual focus change
        will not take effect until the lock is released.

        Setting the focus automatically brings a module to the front of the 
        event processing list unless \a bringToFront is false
        */
    void setFocusedWidget(const shared_ptr<Widget>& m, bool bringToFront = true);

    /** Removes focus from this module if it had focus, otherwise does
        nothing.  The widget will move down one z-order level in focus.  See
        moveWidgetToBack() to push it all of the way to the back. */
    void defocusWidget(const shared_ptr<Widget>& m);

    /** If a lock is in effect, the add may be delayed until the
        unlock.

        Priorities should generally not be used; they are largely for
        supporting debugging components at HIGH_PRIORITY that
        intercept events before they can hit the regular
        infrastructure.
      */
    void add(const shared_ptr<Widget>& m);

    /**
       If a lock is in effect, then the remove will be delayed until the unlock.
     */
    void remove(const shared_ptr<Widget>& m);

    /** If this widget has been added to the WidgetManager, or has a pending add (but not a subsequent pending remove) while in a lock.*/
    bool contains(const shared_ptr<Widget>& m) const;

    /**
     Removes all widgets on this manager.
     */
    void clear();

    /** Number of installed widgets */
    int size() const;

    /** Queues an event on the window associated with this manager. */
    void fireEvent(const GEvent& event);

    /** @deprecated
        Runs the event handles of each manager interlaced, as if all
        the modules from b were in a.*/
    static bool onEvent(const GEvent& event, 
                        WidgetManager::Ref& a, 
                        WidgetManager::Ref& b);

    static bool onEvent(const GEvent& event,
                        WidgetManager::Ref& a);

    /** Returns a module by index number.  The highest index is the one that receives events first.*/
    const shared_ptr<Widget>& operator[](int i) const;

    /** Calls onPose on all children.*/
    virtual void onPose(
        Array<shared_ptr<Surface> >& posedArray, 
        Array<Surface2DRef>& posed2DArray);

    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);

    /** \copydoc Widget::onEvent */
    virtual bool onEvent(const GEvent& event);

    /** \copydoc Widget::onAfterEvents */
    virtual void onAfterEvents();

    virtual void onUserInput(UserInput* ui);

    virtual void onNetwork();

    virtual void onAI();

    virtual OSWindow* window() const;
};


/**
 Exports a coordinate frame, typically in response to user input.
 Examples:
 G3D::ThirdPersonManipulator,
 G3D::FirstPersonManipulator
 */
class Manipulator : public Widget {
public:
    /** By default, does nothing */
    virtual void setFrame(const CFrame& c) {
        (void)c;
    }

    virtual void setEnabled(bool b) {}
    virtual bool enabled() const { return false; }

    virtual void getFrame(CoordinateFrame& c) const = 0;

    virtual CoordinateFrame frame() const = 0;
};


} // G3D

#endif
