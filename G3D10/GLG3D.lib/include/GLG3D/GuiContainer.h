/**
 @file GLG3D/GuiContainer.h

 @created 2008-07-15
 @edited  2014-07-15

 G3D Library http://g3d.codeplex.com
 Copyright 2001-2014, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
*/
#ifndef G3D_GuiContainer_h
#define G3D_GuiContainer_h

#include "G3D/Rect2D.h"
#include "GLG3D/GuiControl.h"

namespace G3D {


/**
 \brief Base class for controls that contain other controls.  

 This class contains helper routines for processing internal controls and 
 is treated specially during layout and rendering by GuiPane.

 See GuiTextureBox's source code for an example of how to build a GuiControl subclass.

 All coordinates of objects inside a pane are relative to the container's
 clientRect().  
 */
class GuiContainer : public GuiControl {
public:
    enum {CONTROL_HEIGHT    =  25};
    enum {CONTROL_WIDTH     = 215};
    enum {BUTTON_WIDTH      =  80};
    enum {TOOL_BUTTON_WIDTH =  50};
    enum {CONTROL_PADDING   =   4};

protected:

    /** Position to which all child controls are relative.*/
    Rect2D              m_clientRect;

    GuiContainer(class GuiWindow* gui, const class GuiText& text);
    GuiContainer(class GuiContainer* parent, const class GuiText& text);

public: 

    /** Client rect bounds, relative to the parent (or window if
        there is no parent). */
    const Rect2D& clientRect() const {
        return m_clientRect;
    }

    virtual void setRect(const Rect2D& rect);

    /** Updates this container to ensure that its client rect is least as wide and
        high as the specified extent, then recursively calls
        increaseBounds on its parent.  Used during automatic layout sizing.  */
    virtual void increaseBounds(const Vector2& extent);

    /** Invoked immediately (i.e., outside of the queue sequence) when a child fires an event 
    through Widget::fireEvent. 
      If this method returns true, the event is never submitted to the event queue. 
      The default implementation passes the event to the GUI parent of this GuiContainer. 
      
      This enables creation of new custom controls by embedding other controls inside
      a GuiContainer; the container can suppress or watch the child control events
      in order to present its own behavior to its parent and the GuiWindow.
      */
    virtual bool onChildControlEvent(const GEvent& event);
};

}

#endif
