/**
 @file GLG3D/GuiMenu.h

 @created 2008-07-14
 @edited  20104-01-11

 G3D Library http://g3d.codeplex.com
 Copyright 2001-2014, Morgan McGuire, http://graphics.cs.williams.edu
 All rights reserved.
*/
#ifndef G3D_GuiMenu_h
#define G3D_GuiMenu_h

#include "G3D/Pointer.h"
#include "G3D/Array.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiControl.h"

namespace G3D {

class GuiPane;
class GuiScrollPane;

/**A special "popup" window that hides itself when it loses focus.
   Used by GuiDropDownList for the popup and can be used to build context menus. */
class GuiMenu : public GuiWindow {
protected:

    GuiControl::Callback            m_actionCallback;
    GuiControl*                     m_eventSource;

    Array<String>*                  m_stringListValue;
    Array<GuiText>*                 m_captionListValue;
    /** The created labels */
    Array<GuiControl*>              m_labelArray;
    Pointer<int>                    m_indexValue;

    /** Which of the two list values to use */
    bool                            m_useStringList;

    /** Window to select when the menu is closed */
    GuiWindow*                      m_superior;

    /** Scroll pane to stick the menu options in if there are too many. Null if no need for scroll */
    GuiScrollPane*                  m_innerScrollPane;

    /** Mouse is over this option */
    int                             m_highlightIndex;

    GuiMenu(const shared_ptr<GuiTheme>& theme, const Rect2D& rect, Array<GuiText>* listPtr, const Pointer<int>& indexValue);
    GuiMenu(const shared_ptr<GuiTheme>& theme, const Rect2D& rect, Array<String>* listPtr, const Pointer<int>& indexValue);

    /** Returns -1 if none */
    int labelIndexUnderMouse(Vector2 click) const;

    /** Fires an action event */
    void fireMyEvent(GEventType type);

    void init(const shared_ptr<GuiTheme>& skin, const Rect2D& rect, const Array<GuiText>& listPtr, const Pointer<int>& indexValue);

public:

    static shared_ptr<GuiMenu> create(const shared_ptr<GuiTheme>& theme, Array<GuiText>* listPtr, const Pointer<int>& indexValue);
    
    static shared_ptr<GuiMenu> create(const shared_ptr<GuiTheme>& theme, Array<String>* listPtr, const Pointer<int>& indexValue);

    virtual bool onEvent(const GEvent& event) override;

    virtual void render(RenderDevice* rd) const override;

    void hide();

    /** \param superior The window from which the menu is being created. */
    virtual void show(WidgetManager* manager, GuiWindow* superior, GuiControl* eventSource, const Vector2& position, bool modal = false, const GuiControl::Callback& actionCallback = GuiControl::Callback());
};

}
#endif
