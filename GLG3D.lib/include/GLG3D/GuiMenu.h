/**
 @file GLG3D/GuiMenu.h

 @created 2008-07-14
 @edited  20104-01-11

 G3D Library http://g3d.cs.williams.edu
 Copyright 2000-2017, Morgan McGuire morgan@casual-effects.com
 All rights reserved.
*/
#pragma once

#include "G3D/Pointer.h"
#include "G3D/Array.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiControl.h"

namespace G3D {

class PrefixTree;
class GuiPane;
class GuiScrollPane;

/**A special "popup" window that hides itself when it loses focus.
   Used by GuiDropDownList for the popup and can be used to build context menus. */
class GuiMenu : public GuiWindow {
friend class GuiDropDownList;
protected:

    GuiControl::Callback            m_actionCallback;
    GuiControl*                     m_eventSource;

    Array<String>*                  m_stringListValue;
    Array<GuiText>*                 m_captionListValue;

    /** The created labels for each menu item */
    Array<GuiControl*>              m_labelArray;
    /** If m_labelArray[i] corresponds to a prefix tree internal node, m_hasChildren[i] is true */
    Array<bool>                     m_hasChildren;
    Pointer<int>                    m_indexValue;

    /** Which of the two list values to use */
    bool                            m_useStringList;

    /** Window to select when the menu is closed */
    GuiWindow*                      m_superior;

    /** Scroll pane to stick the menu options in if there are too many. Null if no need for scroll */
    GuiScrollPane*                  m_innerScrollPane;

    /** Mouse is over this option */
    int                             m_highlightIndex;

    bool                            m_usePrefixTreeMenus;

    /** Non-null only if m_usePrefixTreeMenus == true */
    shared_ptr<PrefixTree>          m_prefixTree;
    
    GuiMenu(const shared_ptr<GuiTheme>& theme, const Rect2D& rect, Array<GuiText>* listPtr, const Pointer<int>& indexValue, bool usePrefixTreeMenus);
    GuiMenu(const shared_ptr<GuiTheme>& theme, const Rect2D& rect, Array<String>* listPtr, const Pointer<int>& indexValue, bool usePrefixTreeMenus);

    /** Returns -1 if none */
    int labelIndexUnderMouse(Vector2 click) const;

    /** Fires an action event */
    void fireMyEvent(GEventType type);

    void init(const shared_ptr<GuiTheme>& skin, const Rect2D& rect, const Array<GuiText>& listPtr, const Pointer<int>& indexValue);

    /** Called from render to draw chevrons and highlighting before child content. */
    virtual void renderDecorations(RenderDevice* rd) const;

public:

    /** A submenu. If the child is not null, then this window will not close. TODO: improve documentation */
    shared_ptr<GuiMenu>             m_child; // TODO make protected
    shared_ptr<GuiMenu>             m_parent; // TODO make protected

    static shared_ptr<GuiMenu> create(const shared_ptr<GuiTheme>& theme, Array<GuiText>* listPtr, const Pointer<int>& indexValue, bool usePrefixTreeMenus = false);
    
    static shared_ptr<GuiMenu> create(const shared_ptr<GuiTheme>& theme, Array<String>* listPtr, const Pointer<int>& indexValue, bool usePrefixTreeMenus = false);

    virtual bool onEvent(const GEvent& event) override;

    virtual void render(RenderDevice* rd) const override;
   
    void hide();

    const Rect2D& labelRect(int i) { return m_labelArray[i]->clickRect(); }

    /** \param superior The window from which the menu is being created. */
    virtual void show(WidgetManager* manager, GuiWindow* superior, GuiControl* eventSource, const Vector2& position, bool modal = false, const GuiControl::Callback& actionCallback = GuiControl::Callback());
};

}
