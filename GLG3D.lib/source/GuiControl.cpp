/**
 \file GuiControl.cpp
 
 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2007-06-01
 \edited  2011-05-10
 */

#include "G3D/platform.h"
#include "GLG3D/GuiControl.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiContainer.h"

namespace G3D {

GuiControl::GuiControl(GuiWindow* gui, const GuiText& caption) : m_enabled(true), m_gui(gui), m_parent(NULL), m_rect(Rect2D::xywh(0,0,0,0)), m_clickRect(Rect2D::empty()), m_visible(true) {
    m_eventSource = this;
    setCaption(caption);
}


GuiControl::GuiControl(GuiContainer* parent, const GuiText& caption) : m_enabled(true), m_gui(parent->m_gui), m_parent(parent), m_rect(Rect2D::xywh(0,0,0,0)), m_clickRect(Rect2D::empty()), m_visible(true) {
    m_eventSource = this;
    setCaption(caption);
}


GuiWindow* GuiControl::window() const {
    return m_gui;
}


Point2 GuiControl::fromOSWindowCoords(const Point2& v) const {
    Vector2 xform = toOSWindowCoords(Point2(0, 0));
    return v - xform;
}


Point2 GuiControl::toOSWindowCoords(const Point2& v) const {

    Point2 result = v + m_rect.x0y0();

    const GuiContainer* current = m_parent;

    while (current != NULL) {
        result += current->m_rect.x0y0();
        current = current->m_parent;
    }

    // result is now relative to a GuiWindow   
    result += m_gui->rect().x0y0();

    // result is now relative to the OSWindow
    return result;
}


void GuiControl::setFocused(bool b) {
    if (! b) {
        if (m_gui->keyFocusGuiControl == this) {
            m_gui->setKeyFocusOnNextControl();
        }
    } else {
        m_gui->setKeyFocusControl(this);
    }
}


shared_ptr<GuiTheme> GuiControl::theme() const {
    return m_gui->theme();
}


float GuiControl::captionWidth() const {
    return m_captionWidth;
}


float GuiControl::captionHeight() const {
    return m_captionHeight;
}


void GuiControl::setCaptionWidth(float c) {
    m_captionWidth = c;
    // Update the click rect bounds
    setRect(rect());
}


void GuiControl::setCaptionHeight(float c) {
    m_captionHeight = c;
    // Update the click rect bounds
    setRect(rect());
}


void GuiControl::setPosition(float x, float y) {
    setPosition(Vector2(x, y));
}


void GuiControl::setPosition(const Vector2& v) {
    setRect(Rect2D::xywh(v, rect().wh()));
}


void GuiControl::moveBy(float x, float y) {
    moveBy(Vector2(x, y));
}


void GuiControl::moveBy(const Vector2& v) {
    setRect(Rect2D::xywh(rect().x0y0() + v, rect().wh()));
}


void GuiControl::setSize(float x, float y) {
    setSize(Vector2(x, y));
}


void GuiControl::setSize(const Vector2& v) {
    setRect(Rect2D::xywh(rect().x0y0(), v));
}


void GuiControl::setHeight(float h) {
    setRect(Rect2D::xywh(rect().x0y0(), Vector2(rect().width(), h)));
}


void GuiControl::setWidth(float w) {
    setRect(Rect2D::xywh(rect().x0y0(), Vector2(w, rect().height())));
}


void GuiControl::moveRightOf(const GuiControl* control, const Vector2& offset) {
    setRect(Rect2D::xywh(control->rect().x1y0() + offset, rect().wh()));
}


bool GuiControl::mouseOver() const {
    return m_gui->mouseOverGuiControl == this;
}
        

bool GuiControl::focused() const {
    // In focus if this control has the key focus and this window is in focus
    return (m_gui->keyFocusGuiControl == this) && m_gui->focused();
}


bool GuiControl::visible() const {
    return m_visible;
}


void GuiControl::setVisible(bool v) {
    m_visible = v;
}


bool GuiControl::enabled() const {
    return m_enabled;
}


void GuiControl::setEnabled(bool e) {
    m_enabled = e;
}


const GuiText& GuiControl::caption() const {
    return m_caption;
}


void GuiControl::setCaption(const GuiText& text) {
    m_caption = text;
    if (m_caption.text() == "") {
        setCaptionWidth(0);
        setCaptionHeight(0);
    } else {
        setCaptionWidth(defaultCaptionWidth());
        setCaptionHeight(defaultCaptionHeight());
    }
}


const Rect2D& GuiControl::rect() const {
    return m_rect;
}


void GuiControl::setRect(const Rect2D& rect) {
    m_clickRect = m_rect = rect;
}


void GuiControl::fireEvent(GEventType type) {
    GEvent e;
    e.gui.type = type;
    e.gui.control = m_eventSource;
    m_gui->fireEvent(e);
}

}

