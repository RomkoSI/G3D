/**
 \file GLG3D/GuiMenu.cpp
 
 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2008-07-14
 \edited  2013-08-14
 */
#include "G3D/platform.h"
#include "GLG3D/GuiMenu.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/RenderDevice.h"

namespace G3D {

shared_ptr<GuiMenu> GuiMenu::create(const shared_ptr<GuiTheme>& theme, Array<String>* listPtr, const Pointer<int>& indexValue) {
    return shared_ptr<GuiMenu>(new GuiMenu(theme, Rect2D::xywh(0, 0, 120, 0), listPtr, indexValue));
}


shared_ptr<GuiMenu> GuiMenu::create(const shared_ptr<GuiTheme>& theme, Array<GuiText>* listPtr, const Pointer<int>& indexValue) {
    return shared_ptr<GuiMenu>(new GuiMenu(theme, Rect2D::xywh(0, 0, 120, 0), listPtr, indexValue));
}


static int menuHeight(int numLabels) {
    return (numLabels * GuiPane::CONTROL_HEIGHT) + GuiPane::CONTROL_PADDING;
}


GuiMenu::GuiMenu(const shared_ptr<GuiTheme>& skin, const Rect2D& rect, Array<String>* listPtr, const Pointer<int>& indexValue) : 
    GuiWindow("", skin, rect, GuiTheme::MENU_WINDOW_STYLE, NO_CLOSE), m_eventSource(NULL), m_stringListValue(listPtr), 
    m_captionListValue(NULL), m_indexValue(indexValue), m_useStringList(true), m_superior(NULL), m_innerScrollPane(NULL)  {

    Array<GuiText> guitextList;
    guitextList.resize(listPtr->size());
    for (int i = 0; i < guitextList.size(); ++i) {
        guitextList[i] = (*listPtr)[i];
    }

    init(skin, rect, guitextList, indexValue);
}


GuiMenu::GuiMenu(const shared_ptr<GuiTheme>& skin, const Rect2D& rect, Array<GuiText>* listPtr, 
                 const Pointer<int>& indexValue) : 
    GuiWindow("", skin, rect, GuiTheme::MENU_WINDOW_STYLE, NO_CLOSE), m_stringListValue(NULL), 
    m_captionListValue(listPtr), m_indexValue(indexValue), m_useStringList(false), m_superior(NULL), m_innerScrollPane(NULL) {

    init(skin, rect, *listPtr, indexValue);
}


void GuiMenu::init(const shared_ptr<GuiTheme>& skin, const Rect2D& rect, const Array<GuiText>& listPtr, const Pointer<int>& indexValue) {
    GuiPane* innerPane;
    const int windowHeight = RenderDevice::current->window()->height();

    m_innerScrollPane = pane()->addScrollPane(true, false, GuiTheme::BORDERLESS_SCROLL_PANE_STYLE);
    m_innerScrollPane->setPosition(0,0);
    m_innerScrollPane->setHeight(float(min(windowHeight, menuHeight(listPtr.size())) + GuiPane::CONTROL_PADDING));
    innerPane = m_innerScrollPane->viewPane();
    
    innerPane->setHeight(0);
    m_labelArray.resize(listPtr.size());

    for (int i = 0; i < listPtr.size(); ++i) {
        m_labelArray[i] = innerPane->addLabel(listPtr[i]);
    }

    if (notNull(m_innerScrollPane)) {
        m_innerScrollPane->pack();
    }

    pack();
    m_highlightIndex = *m_indexValue;
}


bool GuiMenu::onEvent(const GEvent& event) {
    if (! m_visible) {
        return false;
    }

    // Hide on escape
    if ((event.type == GEventType::KEY_DOWN) && 
        (event.key.keysym.sym == GKey::ESCAPE)) {
        fireMyEvent(GEventType::GUI_CANCEL);
        hide();
        return true;
    }

    if ((event.type == GEventType::KEY_DOWN) &&
        (event.key.keysym.sym == GKey::UP)) {
            if (m_highlightIndex > 0) {
                --m_highlightIndex;
            } else {
                m_highlightIndex = m_labelArray.size() - 1;
            }
            *m_indexValue = m_highlightIndex;
            return true;
    }

    if ((event.type == GEventType::KEY_DOWN) &&
        (event.key.keysym.sym == GKey::DOWN)) {
            if (m_highlightIndex < m_labelArray.size() - 1) {
                ++m_highlightIndex;
            } else {
                m_highlightIndex = 0;
            }
            *m_indexValue = m_highlightIndex;
            return true;
    }

    if ((event.type == GEventType::KEY_DOWN) &&
        (event.key.keysym.sym == GKey::RETURN)) {
            *m_indexValue = m_highlightIndex;
            m_actionCallback.execute();
            fireMyEvent(GEventType::GUI_ACTION);
            hide();
            return true;
    }
    
    if (event.type == GEventType::MOUSE_BUTTON_DOWN) {
        // See what was clicked on
        Vector2 click(event.button.x, event.button.y);
        if (m_clientRect.contains(click)) {
            int i = labelIndexUnderMouse(click);
            if (i >= 0) {
                *m_indexValue = i;
                m_actionCallback.execute();
                fireMyEvent(GEventType::GUI_ACTION);
                hide();
                return true;
            } 
        } else { 
            fireMyEvent(GEventType::GUI_CANCEL);

            // Clicked off the menu
            hide();
            return false;
        }
    } else if (event.type == GEventType::MOUSE_MOTION) {
        // Change the highlight
        Vector2 click(event.button.x, event.button.y);
        if (m_clientRect.contains(click)) {
            m_highlightIndex = labelIndexUnderMouse(click);
        }            
    }
    

    bool handled = GuiWindow::onEvent(event);

    if (! (focused() || (notNull(m_innerScrollPane) && m_innerScrollPane->enabled()))) {
        hide();
    }

    return handled;
}


int GuiMenu::labelIndexUnderMouse(Vector2 click) const {
        
    click += pane()->clientRect().x0y0() - m_clientRect.x0y0();
    
    if (notNull(m_innerScrollPane)) {
        click += m_innerScrollPane->paneOffset();
    }

    for (int i = 0; i < m_labelArray.size(); ++i) {
        if (m_labelArray[i]->rect().contains(click)) {
            return i;
        }
    }

    return -1;    
}


void GuiMenu::fireMyEvent(GEventType type) {
    GEvent e;
    e.gui.type = type;
    e.gui.control = m_eventSource;
    Widget::fireEvent(e);
}


void GuiMenu::show(WidgetManager* manager, GuiWindow* superior, GuiControl* eventSource, const Vector2& position, bool modal, const GuiControl::Callback& actionCallback) {
    m_actionCallback = actionCallback;
    m_superior = superior;
    debugAssertM(notNull(eventSource), "Event source may not be NULL");
    m_eventSource = eventSource;
    manager->add(dynamic_pointer_cast<Widget>(shared_from_this()));

    // Clamp position to screen bounds
    OSWindow* osWindow = (superior != NULL) ? superior->window() : RenderDevice::current->window();
    const Vector2 high(osWindow->width() - m_rect.width(), osWindow->height() - m_rect.height());
    Vector2 actualPos = position.min(high).max(Vector2(0, 0));

    moveTo(actualPos);
    manager->setFocusedWidget(dynamic_pointer_cast<Widget>(shared_from_this()));

    if (modal) {
        showModal(dynamic_pointer_cast<GuiWindow>(superior->shared_from_this()));
    } else {
        setVisible(true);
    }
}


void GuiMenu::hide() {
    setVisible(false);
    m_manager->remove(dynamic_pointer_cast<Widget>(shared_from_this()));
    m_manager->setFocusedWidget(dynamic_pointer_cast<GuiWindow>(m_superior->shared_from_this()));
    m_superior = NULL;
}


void GuiMenu::render(RenderDevice* rd) const {
    if (m_morph.active) {
        GuiMenu* me = const_cast<GuiMenu*>(this);
        me->m_morph.update(me);
    }
    
    m_theme->beginRendering(rd);
    {
        m_theme->renderWindow(m_rect, focused(), false, false, false, m_text, GuiTheme::WindowStyle(m_style));
        m_theme->pushClientRect(m_clientRect);
            // Draw the hilight (the root pane is invisible, so it will not overwrite)
            int i = m_highlightIndex;
            if ((i >= 0) && (i < m_labelArray.size())) {
                const Rect2D& r = m_labelArray[i]->rect();
                const float verticalOffset = notNull(m_innerScrollPane) ? -m_innerScrollPane->verticalOffset() : 0.0f;
                m_theme->renderSelection(Rect2D::xywh(0, r.y0() + verticalOffset, m_clientRect.width(), r.height()));
            }
            
            m_rootPane->render(rd, m_theme, m_enabled);
        m_theme->popClientRect();
    }
    m_theme->endRendering();
}

}
