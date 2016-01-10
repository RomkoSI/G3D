/**
 \file GuiScrollBar.cpp
 
 \maintainer Daniel Evangelakos

 \created 2013-01-24
 \edited  2013-01-24

 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */
#include "G3D/platform.h"
#include "GLG3D/GuiScrollBar.h"
#include "GLG3D/GuiWindow.h"

namespace G3D {

static const float BUTTON_HEIGHT_AND_WIDTH = 15;
static const float BUTTON_SCROLL_SPEED = .03;
static const float BUTTON_PRESS_SCROLL = .01;

GuiScrollBar::GuiScrollBar(GuiContainer* parent, 
                            const Pointer<float>& value,
                            float maxValue, 
                            float extent, 
                            bool vertical, 
                            GuiControl* eventSource) :
        GuiControl(parent),
        m_extent(extent),
        m_maxValue(maxValue), 
        m_vertical(vertical) {
               
    if (eventSource != NULL) {
        m_eventSource = eventSource;
    }
    m_value = value;
    m_state = NONE;
}

void GuiScrollBar::updateScroll() {
    if (m_state == ARROW_UP_SCROLLING) {
        *m_value = max<float>( 0.0f, *m_value - m_extent * BUTTON_SCROLL_SPEED);
    } else if (m_state == ARROW_DOWN_SCROLLING) {
        *m_value = min( m_maxValue - m_extent, *m_value + m_extent * BUTTON_SCROLL_SPEED);
    }
}


void GuiScrollBar::render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const {
    (void)rd;
    if (m_visible) {            
        const_cast<GuiScrollBar*>(this)->updateScroll();
        if (m_vertical) {
            theme->renderVerticalScrollBar(m_rect, floatValue(), maxValue(), focused());
        } else {
            theme->renderHorizontalScrollBar(m_rect, floatValue(), maxValue(), focused());
        }
    }
}

void GuiScrollBar::getAllBounds(Rect2D& top, Rect2D& bottom, Rect2D& barBounds, Rect2D& thumbBounds) const {
    if(m_vertical) {
        top = Rect2D::xywh(m_rect.x0y0(), 
            Vector2(BUTTON_HEIGHT_AND_WIDTH, BUTTON_HEIGHT_AND_WIDTH));
        bottom = Rect2D::xywh(m_rect.x0y1() - Vector2(0, BUTTON_HEIGHT_AND_WIDTH), 
               Vector2(BUTTON_HEIGHT_AND_WIDTH, BUTTON_HEIGHT_AND_WIDTH));
        barBounds = theme()->verticalScrollBarToBarBounds(m_rect);
        thumbBounds = theme()->verticalScrollBarToThumbBounds(barBounds, floatValue(), maxValue());
    } else {
        top = Rect2D::xywh(m_rect.x0y0(), 
            Vector2(BUTTON_HEIGHT_AND_WIDTH, BUTTON_HEIGHT_AND_WIDTH));
        bottom = Rect2D::xywh(m_rect.x1y0() - Vector2(BUTTON_HEIGHT_AND_WIDTH, 0), 
               Vector2(BUTTON_HEIGHT_AND_WIDTH, BUTTON_HEIGHT_AND_WIDTH));
        barBounds = theme()->horizontalScrollBarToBarBounds(m_rect);
        thumbBounds = theme()->horizontalScrollBarToThumbBounds(barBounds, floatValue(), maxValue());
    }
}

bool GuiScrollBar::onEvent(const GEvent& event) {
    if (! m_visible) {
        return false;
    }
    float m = maxValue();
        Rect2D topB;
        Rect2D bottomB; 
        Rect2D barBounds;
        Rect2D thumbBounds;

        getAllBounds(topB, bottomB, barBounds, thumbBounds);
    if (event.type == GEventType::MOUSE_BUTTON_DOWN) {
       
        const Vector2& mouse = Vector2(event.button.x, event.button.y);
      
        if(bottomB.contains(mouse)) {
            *m_value = min<float>( m * m_extent, *m_value + m_extent * BUTTON_PRESS_SCROLL);
            m_state = ARROW_DOWN_SCROLLING;
            GEvent response;

            response.gui.type = GEventType::GUI_CHANGE;
            response.gui.control = m_eventSource;
            m_gui->fireEvent(response);

            fireEvent(GEventType::GUI_ACTION);
            return true;

        } else if (topB.contains(mouse)) {

            *m_value = max<float>( 0.0f, *m_value - m_extent*BUTTON_PRESS_SCROLL);

            m_state = ARROW_UP_SCROLLING;

            GEvent response;
            response.gui.type = GEventType::GUI_CHANGE;
            response.gui.control = m_eventSource;
            m_gui->fireEvent(response);

            fireEvent(GEventType::GUI_ACTION);
            return true;

        } else if (thumbBounds.contains(mouse)) {
            m_state = IN_DRAG;
            m_dragStart = mouse;
            m_dragStartValue = floatValue();
            
            GEvent response;
            response.gui.type = GEventType::GUI_DOWN;
            response.gui.control = m_eventSource;
            m_gui->fireEvent(response);

            response.gui.type = GEventType::GUI_CHANGE;
            response.gui.control = m_eventSource;
            m_gui->fireEvent(response);

            return true;

        } else if (barBounds.contains(mouse)) {
            // Jump to this position
            float p;
            if(m_vertical) {
                p =  ( mouse.y - (float)barBounds.y0() ) / ((float)barBounds.height()/(m + 1)) - .5f;
            } else {
                p = ( mouse.x - (float)barBounds.x0() ) / ((float)barBounds.width()/(m + 1)) - .5f;
            }
            p = max<float>(0, p);
            p = min<float>(p, m);
            setFloatValue(p);

            m_state = NONE;

            GEvent response;
            response.gui.type = GEventType::GUI_CHANGE;
            response.gui.control = m_eventSource;
            m_gui->fireEvent(response);

            fireEvent(GEventType::GUI_ACTION);
            return true;
        }

        return false;

    } else if (event.type == GEventType::MOUSE_BUTTON_UP) {

        m_state = NONE;

        fireEvent(GEventType::GUI_ACTION);
        if (m_state == IN_DRAG) {
            // End the drag

            fireEvent(GEventType::GUI_DOWN);
            fireEvent(GEventType::GUI_ACTION);

            return true;
        }

    } else if (event.type == GEventType::MOUSE_MOTION) {

        // We'll only receive these events if we have the keyFocus, but we can't
        // help receiving the key focus if the user clicked on the control!

        const Vector2& mouse = Vector2(event.button.x, event.button.y);

        if (m_state == IN_DRAG) {    
            float delta;
            if(m_vertical) {
                delta = (mouse.y - m_dragStart.y) / (barBounds.height()/(m + 1));
            } else {
                delta = (mouse.x - m_dragStart.x) / (barBounds.width()/(m + 1));
            }
            float p = m_dragStartValue + delta;
            p = max<float>(0, p);
            p = min<float>(p, m);
            setFloatValue(p);
    
            GEvent response;
            response.gui.type = GEventType::GUI_CHANGE;
            response.gui.control = m_eventSource;
            m_gui->fireEvent(response);

             return true;
        } else if  (m_state == ARROW_UP_SCROLLING || m_state == ARROW_UP) {
            if(topB.contains(mouse)) {
                m_state = ARROW_UP_SCROLLING;
            } else {
                m_state = ARROW_UP;
            }
            return true;
        } else if (m_state == ARROW_DOWN_SCROLLING || m_state == ARROW_DOWN)  {       
            if(bottomB.contains(mouse)) {
                m_state = ARROW_DOWN_SCROLLING;
            } else {
                m_state = ARROW_DOWN;
            }
            return true;
        }
    } 
    return false;
    }
}
