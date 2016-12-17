/**
 \file GLG3D/GuiMenu.cpp
 
 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2008-07-14
 \edited  2013-08-14

 G3D Library http://g3d.cs.williams.edu
 Copyright 2000-2017, Morgan McGuire morgan@casual-effects.com
 All rights reserved.
*/
#include "G3D/platform.h"
#include "G3D/PrefixTree.h"
#include "GLG3D/GuiMenu.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/RenderDevice.h"

namespace G3D {

shared_ptr<GuiMenu> GuiMenu::create(const shared_ptr<GuiTheme>& theme, Array<String>* listPtr, const Pointer<int>& indexValue, bool usePrefixTreeMenus) {
    return createShared<GuiMenu>(theme, Rect2D::xywh(0, 0, 120, 0), listPtr, indexValue, usePrefixTreeMenus);
}


shared_ptr<GuiMenu> GuiMenu::create(const shared_ptr<GuiTheme>& theme, Array<GuiText>* listPtr, const Pointer<int>& indexValue, bool usePrefixTreeMenus) {
    return createShared<GuiMenu>(theme, Rect2D::xywh(0, 0, 120, 0), listPtr, indexValue, usePrefixTreeMenus);
}


static int menuHeight(int numLabels) {
    return (numLabels * GuiPane::CONTROL_HEIGHT) + GuiPane::CONTROL_PADDING;
}

GuiMenu::GuiMenu(const shared_ptr<GuiTheme>& skin, const Rect2D& rect, Array<String>* listPtr, const Pointer<int>& indexValue, bool usePrefixTreeMenus) : 
    GuiWindow("", skin, rect, GuiTheme::MENU_WINDOW_STYLE, NO_CLOSE), m_eventSource(NULL), m_stringListValue(listPtr), 
    m_captionListValue(nullptr), m_indexValue(indexValue), m_useStringList(true), m_superior(nullptr), m_child(nullptr),
    m_innerScrollPane(nullptr), m_usePrefixTreeMenus(usePrefixTreeMenus) {

    Array<GuiText> guitextList;
    guitextList.resize(listPtr->size());
    for (int i = 0; i < guitextList.size(); ++i) {
        guitextList[i] = (*listPtr)[i];
    }

    init(skin, rect, guitextList, indexValue);
}


GuiMenu::GuiMenu(const shared_ptr<GuiTheme>& skin, const Rect2D& rect, Array<GuiText>* listPtr, 
                 const Pointer<int>& indexValue, bool usePrefixTreeMenus) : 
    GuiWindow("", skin, rect, GuiTheme::MENU_WINDOW_STYLE, NO_CLOSE), m_stringListValue(nullptr), 
    m_captionListValue(listPtr), m_indexValue(indexValue), m_useStringList(false), m_superior(nullptr), m_child(nullptr),
    m_innerScrollPane(nullptr), m_usePrefixTreeMenus(usePrefixTreeMenus)  {

    init(skin, rect, *listPtr, indexValue);
}


void GuiMenu::init(const shared_ptr<GuiTheme>& skin, const Rect2D& rect, const Array<GuiText>& listPtr, const Pointer<int>& indexValue) {
    //if (m_usePrefixTreeMenus) {
    //    alwaysAssertM(! m_useStringList, "Cannot create a prefix tree menu from a GuiText array");
    //    m_prefixTree = PrefixTree::create(*m_stringListValue);
    //    // TODO: instead of making my own GUI, create my immediate child, which is the root menu
    //}

    GuiPane* innerPane;
    const int windowHeight = RenderDevice::current->window()->height();

    m_innerScrollPane = pane()->addScrollPane(true, false, GuiTheme::BORDERLESS_SCROLL_PANE_STYLE);
    m_innerScrollPane->setPosition(0, 0);
    m_innerScrollPane->setHeight(float(min(windowHeight, menuHeight(listPtr.size())) + GuiPane::CONTROL_PADDING));
    innerPane = m_innerScrollPane->viewPane();
    
    innerPane->setHeight(0);
    m_labelArray.resize(listPtr.size());

    m_hasChildren.resize(listPtr.size());
    for (int i = 0; i < listPtr.size(); ++i) {
        m_labelArray[i] = innerPane->addLabel(listPtr[i]);
        m_hasChildren[i] = false;
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

    // Hide all menus on escape key
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
        hide(); // TODO prob don't wanna do these either. just hit callback and it will figure out if we should be hidden
        return true;
    }


    if ((event.type == GEventType::GUI_ACTION) &&
        (event.gui.control == m_eventSource) &&
        isNull(m_child)) { 
        m_actionCallback.execute(); 
        // Do not consume on a callback
        return false;
    }
    
    if (event.type == GEventType::MOUSE_BUTTON_DOWN) {
        // See what was clicked on
        const Point2 click(event.button.x, event.button.y);
        if (m_clientRect.contains(click)) {
            if (isNull(m_child)) {
                const int i = labelIndexUnderMouse(click);
                if (i >= 0) {
                    // There was a valid label index. Invoke 
                    // the menu's callback and then fire the action.
                    *m_indexValue = i;
                    fireMyEvent(GEventType::GUI_ACTION);
                    return true;
                }
                // (The click may have gone to the scroll bar in this case)
            } else {
                // If the user clicks on this and a child menu is active,
                // hide this menu's children.
                m_child->hide();
                return false;
            }             
        } else {
            // Clicked off the menu
            if (notNull(m_child)) {
                m_child->hide();
            } else if (isNull(m_parent)) {
                // Hide root menu
                hide();
            }
            return false;
        }
    } else if (event.type == GEventType::MOUSE_MOTION) {
        // Change the highlight
        const Point2 click(event.button.x, event.button.y);
        if (m_clientRect.contains(click)) {
            m_highlightIndex = labelIndexUnderMouse(click);
        }            
    }    

    bool handled = GuiWindow::onEvent(event);

    if (! (focused() || (notNull(m_innerScrollPane) && m_innerScrollPane->enabled())) && 
        notNull(m_child)) {
        hide();
    }

    return handled;
}


int GuiMenu::labelIndexUnderMouse(Vector2 click) const {
        
    click += pane()->clientRect().x0y0() - m_clientRect.x0y0();
    
    float width = pane()->clientRect().width();
    if (notNull(m_innerScrollPane)) {
        click += m_innerScrollPane->paneOffset();
        width = m_innerScrollPane->viewPane()->rect().width();
    }

    for (int i = 0; i < m_labelArray.size(); ++i) {
        // Extend the rect to the full width of the menu
        Rect2D rect = m_labelArray[i]->rect();
        rect = Rect2D::xywh(rect.x0(), rect.y0(), width, rect.height());
        if (rect.contains(click)) {
            return i;
        }
    }

    // Nothing found
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
    OSWindow* osWindow = notNull(superior) ? superior->window() : RenderDevice::current->window();
    const Vector2 high(osWindow->width() - m_rect.width(), osWindow->height() - m_rect.height());
    const Vector2 actualPos = position.min(high).max(Vector2(0, 0));

    moveTo(actualPos);
    manager->setFocusedWidget(dynamic_pointer_cast<Widget>(shared_from_this()));

    if (modal) {
        showModal(dynamic_pointer_cast<GuiWindow>(superior->shared_from_this()));
    } else {
        setVisible(true);
    }
}


void GuiMenu::hide() {
    if (isNull(m_manager)) return; // TODO remove
    debugAssert(notNull(m_manager));

    // First, recursively hide and remove all children
    if (m_child) {
        m_child->hide();
    }

    // Make this menu disappear and return focus to superior window
    setVisible(false);
    const shared_ptr<Widget>& widget = dynamic_pointer_cast<Widget>(shared_from_this());

    if (notNull(widget) && m_manager->contains(widget)) { // TODO perhaps this check should not be necessary
        m_manager->remove(widget);
    }
    m_manager->setFocusedWidget(dynamic_pointer_cast<GuiWindow>(m_superior->shared_from_this()));

    // Detach this child from its parent
    m_superior = nullptr;
    if (notNull(m_parent)) {
        m_parent->m_child = nullptr;
        m_parent = nullptr;
    }
}


void GuiMenu::render(RenderDevice* rd) const {
    if (m_morph.active) {
        GuiMenu* me = const_cast<GuiMenu*>(this);
        me->m_morph.update(me);
    }
    
    m_theme->beginRendering(rd);
    {
        m_theme->renderWindow(m_rect, focused(), false, false, false, m_text, GuiTheme::WindowStyle(m_style));
        m_theme->pushClientRect(m_clientRect); {
            renderDecorations(rd);                        
            m_rootPane->render(rd, m_theme, m_enabled);
        } m_theme->popClientRect();
    }
    m_theme->endRendering();
}


void GuiMenu::renderDecorations(RenderDevice* rd) const {
    // Draw the highlight (the root pane is invisible, so it will not overwrite)
    if ((m_highlightIndex >= 0) && (m_highlightIndex < m_labelArray.size())) {
        const Rect2D& r = m_labelArray[m_highlightIndex]->rect();
        const float verticalOffset = notNull(m_innerScrollPane) ? -m_innerScrollPane->verticalOffset() : 0.0f;
        m_theme->renderSelection(Rect2D::xywh(0, r.y0() + verticalOffset, m_clientRect.width(), r.height()));
    }
}

}
