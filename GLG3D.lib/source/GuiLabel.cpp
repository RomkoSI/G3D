/**
 @file GuiLabel.cpp
 
 @maintainer Morgan McGuire, http://graphics.cs.williams.edu

 @created 2007-06-02
 @edited  2007-06-10

 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */
#include "G3D/platform.h"
#include "GLG3D/GuiLabel.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiPane.h"

namespace G3D {

GuiLabel::GuiLabel(GuiContainer* parent, const GuiText& text, GFont::XAlign x, GFont::YAlign y) 
    : GuiControl(parent, text), m_xalign(x), m_yalign(y) {
}


void GuiLabel::render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const {
    (void)rd;
    if (m_visible) {
        theme->renderLabel(m_rect, m_caption, m_xalign, m_yalign, m_enabled && ancestorsEnabled, true);
    }
}


}
