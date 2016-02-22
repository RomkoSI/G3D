/**
 \file GLG3D/GuiControl.h

 \created 2006-05-01
 \edited  2011-08-28

 G3D Library http://g3d.cs.williams.edu
 Copyright 2000-2015, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
 Use permitted under the BSD license
*/
#ifndef G3D_GuiControl_h
#define G3D_GuiControl_h

#include "G3D/G3DString.h"
#include "GLG3D/GuiTheme.h"
#include "GLG3D/glcalls.h"
#include "GLG3D/Widget.h"
#include <functional>

namespace G3D {

// forward declare heavily dependent classes
class RenderDevice;
class GuiWindow;
class GuiContainer;


/** Base class for all controls. */
class GuiControl {
    friend class GuiWindow;
    friend class GuiPane;
    friend class GuiContainer;
    friend class WidgetManager;

protected:

    enum {
        LEFT_CAPTION_WIDTH = 80,
        TOP_CAPTION_HEIGHT = 20
    };

public:

    /** Base class for GuiButton pre-event handlers. You may subclass this and override execute or
        simply use one of the provided constructors. */
    class Callback {
    private:
        
        std::function<void(void)> m_internal;
        
    public:
        
        inline Callback() : m_internal(std::function<void(void)>()) {}
        
        /** Create a callback from a function, e.g., <code>GuiControl::Callback(&printWarning)</code> */
        inline Callback(std::function<void(void)> function) : m_internal(function) {}
        
        /** Create a callback from a class and method of no arguments, e.g.,
            <CODE> App* app = ...; Callback( app, &App::endProgram ); </CODE>.
            If the method is defined on a base class and not
            overriden in the derived class, you must cast the pointer:
            <CODE> Callback(static_cast<Base*>(ptr), &Base::method); </CODE>
         */
        template<class Class>
        inline Callback(
            Class* const object,
            void (Class::*method)()) : m_internal(std::bind(method, object)) {}
        
        /** Create a callback from a reference counted class and method of no arguments, 
            e.g., 
            
            \code 
            shared_ptr<Player> player = ...; 
            pane->addButton("Jump", GuiControl::Callback(player, &Player::jump));
            \endcode */
        template<class Class>
        inline Callback(
            const shared_ptr<Class>& object,
            void (Class::*method)()) : m_internal(std::bind(method, object)) {}
        
        /** Execute the callback.*/
        virtual void execute() {
            if (m_internal) {
                m_internal();
            }
        }
        
        /** Assignment */
        inline Callback& operator=(const Callback& c) {
            m_internal = c.m_internal;
            return this[0];
        }
        
        /** Copy constructor */
        Callback(const Callback& c) : m_internal(std::function<void(void)>()) {
            this[0] = c;
        }
    };
    
protected:
    
    /** Sent events should appear to be from this object, which is
       usually "this".  Other controls can set the event source
       to create compound controls that seem atomic from the outside. */
    GuiControl*       m_eventSource;

    bool              m_enabled;

    /** The window that ultimately contains this control */
    GuiWindow*        m_gui;

    /** Parent pane */
    GuiContainer*     m_parent;

    /** Rect bounds used for rendering and layout.
        Relative to the enclosing pane's clientRect. */
    Rect2D            m_rect;

    /** Rect bounds used for mouse actions.  Updated by setRect.*/
    Rect2D            m_clickRect;

    GuiText           m_caption;

    float             m_captionWidth;
    float             m_captionHeight;

    bool              m_visible;

    GuiControl(GuiWindow* gui, const GuiText& text = "");
    GuiControl(GuiContainer* parent, const GuiText& text = "");

    /** Fires an event. */
    void fireEvent(GEventType type);    

public:

    void setEventSource(GuiControl* c) {
        m_eventSource = c;
    }
    virtual ~GuiControl() {}

    bool enabled() const;
    bool mouseOver() const;
    bool visible() const;
    void setVisible(bool b);
    bool focused() const;
    virtual void setCaption(const GuiText& caption);

    /** Grab or release keyboard focus */
    void setFocused(bool b);
    virtual void setEnabled(bool e);
    
    /** For controls that have a caption outside the bounds of the control on the left,
        this is the size reserved for the caption. */
    float captionWidth() const;

    /** For controls that have a caption outside the bounds of the control on the top or bottom,
        this is the size reserved for the caption. */
    float captionHeight() const;

    virtual void setCaptionWidth(float c);
    virtual void setCaptionHeight(float c);

    const GuiText& caption() const;
    const Rect2D& rect() const;

    /** Get the window containing this control. */
    GuiWindow* window() const;

    /** If you explicitly change the rectangle of a control, the
        containing pane may clip its borders.  Call pack() on the
        containing pane (or window) to resize that container
        appropriately.*/
    virtual void setRect(const Rect2D& rect);
    void setSize(const Vector2& v);
    void setSize(float x, float y);
    void setPosition(const Vector2& v);
    void setPosition(float x, float y);
    void setWidth(float w);
    void setHeight(float h);

    /** If these two controls have the same parent, move this one 
        immediately to the right of the argument.
        
        \param offset May be negative */
    void moveRightOf(const GuiControl* control, const Vector2& offset);
    void moveRightOf(const GuiControl* control, float offsetX = 0.0f) {
        moveRightOf(control, Vector2(offsetX, 0.0f));
    }
    void moveBy(const Vector2& delta);
    void moveBy(float dx, float dy);    

    /** \brief Return the enabled, visible control containing the mouse.
    
    The default implementation returns itself if the mouse is within its bounds.  GuiContainer%s should override this to iterate through children. 
    Since only one (non-overlapping) child will write to \a control, it is sufficient to call this on all children without testing 
    to see if one already wrote to \a control. */
    virtual void findControlUnderMouse(Vector2 mouse, GuiControl*& control) {
        if (m_rect.contains(mouse) && m_visible && m_enabled) {
            control = this;
        }
    }


    shared_ptr<GuiTheme> theme() const;

    /** Return true if this is in tool button style */
    virtual bool toolStyle() const { 
        return false;
    }

    /** Default caption size for this control. */
    virtual float defaultCaptionHeight() const {
        return TOP_CAPTION_HEIGHT;
    }

    virtual float defaultCaptionWidth() const {
        return LEFT_CAPTION_WIDTH;
    }

    /**
     Only methods on @a theme may be called from this method by default.  To make arbitrary 
     RenderDevice calls, wrap them in GuiTheme::pauseRendering ... GuiTheme::resumeRendering.

     \param ancestorsEnabled Draw as disabled if this is false or if enabled() is false.
     */
    virtual void render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const = 0;

    /** Used by GuiContainers */
    const Rect2D& clickRect() const {
        return m_clickRect;
    }

    /** Returns the coordinates of \a v, which is in the coordinate system of this object,
       relative to the OSWindow on which it will be rendered. */
    Vector2 toOSWindowCoords(const Vector2& v) const;

    /** Transforms \a v from OS window coordinates to this control's coordinates */
    Vector2 fromOSWindowCoords(const Vector2& v) const;

    Rect2D toOSWindowCoords(const Rect2D& r) const {
        return Rect2D::xywh(toOSWindowCoords(r.x0y0()), r.wh());
    }
protected:

    /** Events are only delivered (by GuiWindow) to a GuiControl when the control has the key focus.
        If the control does not consume the event, the event is delivered to
        each of its GUI parents in order, back to the window's root pane.
        
        Key focus is transferred during a mouse down event. */
    virtual bool onEvent(const GEvent& event) { (void)event; return false; }

};

}

#endif
