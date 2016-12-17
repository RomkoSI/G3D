/**
 \file GuiPrefixDropDownList.cpp
 
 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2007-06-02
 \edited  2014-08-16

 Copyright 2000-2017, Morgan McGuire morgan@casual-effects.com
 All rights reserved.
 */
#include "G3D/platform.h"
#include "GLG3D/GuiPrefixDropDownList.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiPane.h"

namespace G3D {

GuiPrefixDropDownList::GuiPrefixDropDownList
(GuiContainer*                         parent, 
 const GuiText&                        caption,
 const Array<String>&                  items,
 const GuiControl::Callback&           actionCallback) :
 GuiControl(parent, caption),
    m_actionCallback(actionCallback),
    m_selectedValue(items.size() > 0 ? items[0] : ""),
    m_prefixTree(PrefixTree::create(items)) {
}


GuiPrefixDropDownList::GuiPrefixDropDownList
(GuiContainer*                         parent, 
 const GuiText&                        caption,
 const Array<GuiText>&                 items,
 const GuiControl::Callback&           actionCallback) :
 GuiControl(parent, caption),
    m_actionCallback(actionCallback),
    m_selectedValue(items.size() > 0 ? items[0] : ""),
    m_prefixTree(PrefixTree::create(items)) {
}


bool GuiPrefixDropDownList::containsValue(const String& s) const {
    // TODO test
    return m_prefixTree->contains(s);
}


void GuiPrefixDropDownList::render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const {
    (void)rd;
    if (m_visible) {
        // TODO what is the correct value for the selecting/focused Boolean?
        theme->renderDropDownList(m_rect, m_enabled && ancestorsEnabled, focused() || mouseOver(), 
                                 false, selectedValue(), m_caption, m_captionWidth);
    }
}


void GuiPrefixDropDownList::setSelectedValue(const String& s) {
    if (containsValue(s)) {
        m_selectedValue = s;
    }
}


/** Used for storing state needed by the hierarchical GuiMenus */
class GuiMenuNode {
protected:
    int                             m_selectedIndex = -1;
    Array<String>                   m_items;
    Array<shared_ptr<PrefixTree>>   m_nodes;

public:
    const shared_ptr<PrefixTree>& selectedNode() {
        return m_nodes[m_selectedIndex];
    }

    int selectedIndex() {
        return m_selectedIndex;
    }

    int itemCount() {
        return m_items.size();
    }

    GuiMenuNode(const shared_ptr<PrefixTree>& node) {
        for (const shared_ptr<PrefixTree>& child : node->children()) {
            shared_ptr<PrefixTree> branchPoint;
            const String& menuItemName = child->getPathToBranch(branchPoint);
            m_items.append(menuItemName);
            m_nodes.append(branchPoint);
        }
    }

    shared_ptr<GuiMenu> createMenu(const shared_ptr<GuiTheme>& theme) {
        return GuiMenu::create(theme, &m_items, &m_selectedIndex);
    }
};


void GuiPrefixDropDownList::appendMenu(const shared_ptr<GuiMenu>& menu) {
    if (isNull(m_menuHead)) {
        m_menuHead = menu;
    } else {
        // Linear traversal of menu item list to find tail. Makes it so we don't have to
        // keep a tail pointer.
        shared_ptr<GuiMenu> finger = m_menuHead;
        while (notNull(finger->m_child)) {
            finger = finger->m_child;
        }

        finger->m_child = menu;
        menu->m_parent = finger;
    } 
}


void GuiPrefixDropDownList::close() {
    if (notNull(m_menuHead)) {
        m_menuHead->hide();
        m_menuHead.reset();
    }
}


void GuiPrefixDropDownList::showMenu() {
    close();
    
    const Rect2D& clickRect = theme()->dropDownListToClickBounds(rect(), m_captionWidth);
    const Vector2& clickOffset = clickRect.x0y0() - rect().x0y0() + Vector2(GuiContainer::CONTROL_PADDING, 0);
    const Vector2 menuOffset(1, clickRect.height() + 10);
    
    showMenuHelper(m_prefixTree, Rect2D::xywh(clickOffset + menuOffset, Vector2::zero()));
}


void GuiPrefixDropDownList::showMenuHelper(const shared_ptr<PrefixTree>& node, const Rect2D& parentRect) {
    const shared_ptr<GuiMenuNode>& menuNode = std::make_shared<GuiMenuNode>(node);
        
    const shared_ptr<GuiMenu>& menu = menuNode->createMenu(theme());
    appendMenu(menu);    

    const Point2& menuPosition = parentRect.x1y0();

    menu->show(
        m_gui->manager(),
        window(),
        this,
        toOSWindowCoords(menuPosition),
        false,
        GuiControl::Callback([this, menuNode, menu]() {
            // If at a terminal node in the prefix tree, clean up and call callback.
            // Otherwise, recursively create a child menu.
            const shared_ptr<PrefixTree>& node = menuNode->selectedNode();
            
            if (node->isLeaf()) {
                m_selectedValue = node->value();
                m_actionCallback.execute();

                close();
            } else {
                Rect2D parentRect = menu->labelRect(menuNode->selectedIndex());

                // Offset to world space
                parentRect = Rect2D::xywh(menu->rect().x0(), parentRect.y0() + menu->rect().y0(), menu->rect().width(), parentRect.height());               

                showMenuHelper(node, parentRect);
            }
        })
    );
} 


bool GuiPrefixDropDownList::onEvent(const GEvent& event) {
    if (! m_visible) {
        return false;
    }

    // TODO make this block less verbose
    if (event.type == GEventType::MOUSE_BUTTON_DOWN) {
        showMenu();
        return true;
    } else if (event.type == GEventType::KEY_DOWN) {
        switch (event.key.keysym.sym) {
        case GKey::DOWN:
            showMenu();
            return true;
        case GKey::UP:
            showMenu();
            return true;
        default:;
        }
    }

   return false;
}


void GuiPrefixDropDownList::setRect(const Rect2D& rect) {
     m_rect = rect;
     m_clickRect = theme()->dropDownListToClickBounds(rect, m_captionWidth);
}

const GuiText& GuiPrefixDropDownList::selectedValue() const {
    return m_selectedValue;
}

void GuiPrefixDropDownList::setList(const Array<GuiText>& c) {
    m_menuHead.reset();
    
    m_prefixTree = PrefixTree::create();
    for (const GuiText& s : c) {
        m_prefixTree->insert(s); 
    }
}

void GuiPrefixDropDownList::setList(const Array<String>& c) {
    m_menuHead.reset();
    
    m_prefixTree = PrefixTree::create();
    for (const String& s : c) {
        m_prefixTree->insert(s); 
    }
}

void GuiPrefixDropDownList::clear() {
    // TODO test
    m_menuHead.reset();
    m_prefixTree = PrefixTree::create();
}

void GuiPrefixDropDownList::appendValue(const GuiText& c) {
    m_prefixTree->insert(c);
}

}
