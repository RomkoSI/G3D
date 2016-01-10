/**
 \file GuiDropDownList.cpp
 
 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2007-06-02
 \edited  2014-08-16
 */
#include "G3D/platform.h"
#include "GLG3D/GuiDropDownList.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiPane.h"

namespace G3D {


GuiDropDownList::GuiDropDownList
(GuiContainer*                         parent, 
 const GuiText&                        caption, 
 const Pointer<int>&                   indexValue, 
 const Array<GuiText>&                 listValue,
 const Pointer<Array<String> >&        listValuePtr,
 const GuiControl::Callback&           actionCallback) :
 GuiControl(parent, caption), 
    m_indexValue(indexValue.isNull() ? Pointer<int>(&m_myInt) : indexValue),
    m_myInt(0),
    m_listValuePtr(listValuePtr),
    m_listValue(listValue),
    m_selecting(false),
    m_actionCallback(actionCallback) {

}


shared_ptr<GuiMenu> GuiDropDownList::menu() { 
    if (isNull(m_menu)) {
        m_menu = GuiMenu::create(theme(), &m_listValue, m_indexValue);

    }
    return m_menu;
}


bool GuiDropDownList::containsValue(const String& s) const {
    for (int i = 0; i < m_listValue.size(); ++i) {
        if (m_listValue[i].text() == s) {
            return true;
        }
    }
    return false;
}


void GuiDropDownList::render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const {
    (void)rd;
    if (m_visible) {
        theme->renderDropDownList(m_rect, m_enabled && ancestorsEnabled, focused() || mouseOver(), 
                                 m_selecting, selectedValue(), m_caption, m_captionWidth);
    }
}


void GuiDropDownList::setSelectedValue(const String& s) {
    for (int i = 0; i < m_listValue.size(); ++i) {
        if (m_listValue[i].text() == s) {
            setSelectedIndex(i);
            return;
        }
    }
}


void GuiDropDownList::showMenu() {
    // Show the menu
    Rect2D clickRect = theme()->dropDownListToClickBounds(rect(), m_captionWidth);
    Vector2 clickOffset = clickRect.x0y0() - rect().x0y0();
    Vector2 menuOffset(10, clickRect.height() + 10);
    //Vector2 menuOffset(GuiContainer::CONTROL_PADDING, clickRect.height() + 10);

    menu()->show(m_gui->manager(), window(), this, toOSWindowCoords(clickOffset + menuOffset), false, m_actionCallback);
}


bool GuiDropDownList::onEvent(const GEvent& event) {
    if (! m_visible) {
        return false;
    }

    if (event.type == GEventType::MOUSE_BUTTON_DOWN) {

        if (! m_gui->manager()->contains(menu())) {
            showMenu();
        }
        return true;

    } else if (event.type == GEventType::KEY_DOWN) {
        switch (event.key.keysym.sym) {
        case GKey::DOWN:
            *m_indexValue = selectedIndex();
            if (*m_indexValue < m_listValue.size() - 1) {
                *m_indexValue = *m_indexValue + 1;
            } else {
                *m_indexValue = 0;
            }
            m_actionCallback.execute();
            fireEvent(GEventType::GUI_ACTION);
            return true;

        case GKey::UP:
            *m_indexValue = selectedIndex();
            if (*m_indexValue > 0) {
                *m_indexValue = *m_indexValue - 1;
            } else {
                *m_indexValue = m_listValue.size() - 1;
            }
            m_actionCallback.execute();
            fireEvent(GEventType::GUI_ACTION);
            return true;
        default:;
        }
    }

   return false;
}


void GuiDropDownList::setRect(const Rect2D& rect) {
     m_rect = rect;
     m_clickRect = theme()->dropDownListToClickBounds(rect, m_captionWidth);
}


const GuiText& GuiDropDownList::selectedValue() const {
    if (m_listValue.size() > 0) {
        return m_listValue[selectedIndex()];
    } else {
        const static GuiText empty;
        return empty;
    }
}


void GuiDropDownList::setList(const Array<GuiText>& c) {
    m_listValue = c;
    *m_indexValue = iClamp(*m_indexValue, 0, m_listValue.size() - 1);
    m_menu.reset();
}


void GuiDropDownList::setList(const Array<String>& c) {
    m_listValue.resize(c.size());
    for (int i = 0; i < c.size(); ++i) {
        m_listValue[i] = c[i];
    }
    *m_indexValue = selectedIndex();
    m_menu.reset();
}


void GuiDropDownList::clear() {
    m_listValue.clear();
    *m_indexValue = 0;
    m_menu.reset();
}


void GuiDropDownList::append(const GuiText& c) {
    m_listValue.append(c);
    m_menu.reset();
}

}
