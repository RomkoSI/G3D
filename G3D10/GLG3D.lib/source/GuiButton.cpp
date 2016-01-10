/**
 @file GuiButton.cpp
 
 @maintainer Morgan McGuire, http://graphics.cs.williams.edu

 @created 2007-06-02
 @edited  2010-01-13

 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */
#include "G3D/platform.h"
#include "GLG3D/GuiButton.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiPane.h"

namespace G3D {

GuiButton::GuiButton(GuiContainer* parent, const GuiButton::Callback& callback, const GuiText& text, GuiTheme::ButtonStyle style) : 
    GuiControl(parent, text), 
    m_down(false), m_callback(callback), m_style(style) {}


void GuiButton::render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const {
    (void)rd;
    if (m_visible) {
        theme->renderButton(m_rect, m_enabled && ancestorsEnabled, focused() || mouseOver(), 
                           m_down && mouseOver(), m_caption, (GuiTheme::ButtonStyle)m_style);
    }
}


bool GuiButton::onEvent(const GEvent& event) {
    switch (event.type) {
    case GEventType::MOUSE_BUTTON_DOWN:
        m_down = true;
        // invoke the pre-event handler
        m_callback.execute();
        fireEvent(GEventType::GUI_DOWN);
        return true;
    
    case GEventType::MOUSE_BUTTON_UP:
        fireEvent(GEventType::GUI_UP);

        // Only trigger an action if the mouse was still over the control
        if (m_down && m_rect.contains(event.mousePosition())) {
            fireEvent(GEventType::GUI_ACTION);
        }

        m_down = false;
        return true;
    }

    return false;
}

void GuiButton::setDown() {
    m_down = true;
}

void GuiButton::setUp() {
    m_down = false;
}

bool GuiButton::isDown() {
    return m_down;
}

}
