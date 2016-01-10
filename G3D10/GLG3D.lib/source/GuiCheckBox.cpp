#include "GLG3D/GuiCheckBox.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiPane.h"

namespace G3D {

GuiCheckBox::GuiCheckBox(GuiContainer* parent, const GuiText& text, 
                         const Pointer<bool>& value, GuiTheme::CheckBoxStyle style) 
    : GuiControl(parent, text), m_value(value), m_style(style) {}


void GuiCheckBox::render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const {
    (void)rd;
    if (m_visible) {
        switch (m_style) {
        case GuiTheme::NORMAL_CHECK_BOX_STYLE:
            theme->renderCheckBox(m_rect, ancestorsEnabled && m_enabled, focused() || mouseOver(), *m_value, m_caption);
            break;

        default:
            theme->renderButton(m_rect, ancestorsEnabled && m_enabled, focused() || mouseOver(), *m_value, m_caption, 
                               (m_style == GuiTheme::BUTTON_CHECK_BOX_STYLE) ? 
                               GuiTheme::NORMAL_BUTTON_STYLE : GuiTheme::TOOL_BUTTON_STYLE);
        }
    }
}


void GuiCheckBox::setRect(const Rect2D& rect) {
     if (m_style == GuiTheme::NORMAL_CHECK_BOX_STYLE) {
         // Prevent the checkbox from stealing clicks very far away
         m_rect = rect;
         m_clickRect = Rect2D::xywh(rect.x0y0(), theme()->bounds("AA"));
     } else {
         GuiControl::setRect(rect);
     }
}


bool GuiCheckBox::onEvent(const GEvent& event) {
    if ((event.type == GEventType::MOUSE_BUTTON_DOWN) && m_visible) {
        *m_value = ! *m_value;
        fireEvent(GEventType::GUI_ACTION);
        return true;
    } else {
        return false;
    }
}


}
