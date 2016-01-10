/**
 \file GLG3D/GuiScrollBar.h

 \maintainer Daniel Evangelakos

 \created 2013-01-24
 \edited  2014-07-24

 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/
#ifndef G3D_GUISCROLLBAR_H
#define G3D_GUISCROLLBAR_H

#include "GLG3D/GuiControl.h"
#include "G3D/Pointer.h"

namespace G3D {

class GuiWindow;
class GuiPane;

class GuiScrollBar : public GuiControl {
    friend class GuiPane;
    friend class GuiWindow;

protected:

    enum State {
        NONE,
        ARROW_UP_SCROLLING,
        ARROW_DOWN_SCROLLING,
        ARROW_UP,
        ARROW_DOWN,
        IN_DRAG
    };

    Pointer<float>    m_value;
    float             m_extent;
    /* the total size of the document or pane */
    float             m_maxValue;

    State             m_state; 
    bool              m_vertical;
    float             m_dragStartValue;

    /** Position from which the mouse drag started, relative to
        m_gui.m_clientRect.  When dragging the thumb, the cursor may not be
        centered on the thumb the way it is when the mouse clicks
        on the track.
    */
    Vector2           m_dragStart;

    void getAllBounds(Rect2D& top, Rect2D& bottom, Rect2D& barBounds, Rect2D& thumbBounds) const;

    /** returns the value of the thumb from 0 to the maxValue/extent - 1. 
    which is the amount of thumbW it is from the top of the bar.*/
     float floatValue() const {
        return *m_value / m_extent;
    }

    /** returns the max value that thumb pos can be, when everything is called by m_extent.
        N.B. not the same as m_maxValue - this returns the number of bar lengths away from the 
        top where as m_maxValue is the maximum pixel value*/
    float maxValue() const {
        return max<float>(1.0f, m_maxValue / m_extent) - 1.0f;
    }

    /** Set value on the range set the value of thumb pos; used only 
    in render and on event. to set value of thumb pos use setThumbPos*/
    void setFloatValue(float f) {
        *m_value = f * m_extent;
    }

    void updateScroll();

    virtual bool onEvent(const GEvent& event) override;

public:

    virtual void render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const override;
    
    /**
       \param maxValue    -  Is the maximum length of the slider. value will never reach it, the highest 
                              it will give is maxValue - extent.
       \param extent      -  extent is the size of a single screen or single situation. If maxValue is twice 
                              as large as extent then the thumb will be exactly half as long as the track.
       \param eventSource -  If used in any hybrid controls should be set to the pane it is on so that all events
                              fired from the scroll bar come from that pane.
    */
    GuiScrollBar(GuiContainer* parent, 
                const Pointer<float>& value, 
                float maxValue,
                float extent = 1, 
                bool vertical = true, 
                GuiControl* eventSource = NULL);  
    
    void setThumbPos(float pos) {
        alwaysAssertM(pos >= 0 && pos <= m_maxValue - m_extent, "invalid thumb position");
        *m_value = pos;
    }
    
    void setMax(float max) {
        m_maxValue = max;
    }

    void setExtent(float extent) {
        m_extent = extent;
    }

};

}
#endif
